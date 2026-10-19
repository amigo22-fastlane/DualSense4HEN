# Niezależna rewizja architektury — 2026-10-19

**Aktualizacja 2026-10-20:** potwierdzona przez właściciela sesja DualSense
bez USB zmienia priorytet architektury A: obserwujemy istniejącą ścieżkę,
bez nowych prób parowania. UI nie dowodzi kanałów HID. Nowy pomiar i granice
wniosków: [EXISTING_BT_SESSION.md](EXISTING_BT_SESSION.md). Opis bazy poniżej
zachowuje stan wiedzy sprzed tej obserwacji.

Baza: **13aa707**, gałąź `codex/architecture-research-1252`. Main i wcześniejsze
artefakty zachowano. Cel pozostaje: DualSense → wewnętrzny Bluetooth PS4 →
menu i gry Orbis, bez urządzenia pośredniczącego. Nie uruchamiano patchy ani
zmian ustawień; nowy pomiar tworzy i zamyka trzy sockety.

## Co rzeczywiście wynika z dotychczasowych dowodów

Przeczytano dokumentację, parser, adapter VDA, bootstrap, sondy i dostępne
lokalne logi prób do bazy. Starsze pliki częściowo zawierają historię wielu
startów, więc ich końcowy exit nie jest osobnym dowodem każdego etapu.

- PayLoader wylicza 054c:0ce6 i odbiera deskryptor HID 289 B; nie otrzymał
  ani jednego poprawnego raportu wejścia. 0x802400ff nie wskazuje sam miejsca odmowy.
- Aplikacja widzi count=0 mimo udanego O_RDONLY na /dev/usbctl. Backend może
  pomijać nieudane urządzenia; końcowe errno nie jest śladem konkretnego ioctl.
- BluetoothHid Init i callback działały, RegisterDevice odrzucono. Nie ma
  dowodu bondingu DualSense, kanałów L2CAP, danych BT lub wymogu patcha jądra.
- VDA sprawdzono na atrapach; na konsoli wykazano eksporty, nie globalne wejście.
  Żaden pomiar transportu nie wymagał kodu Control4Free.

Szczegóły bazy: [HARDWARE_RESULTS.md](../HARDWARE_RESULTS.md),
[USBD_ABI_EXPERIMENT.md](../USBD_ABI_EXPERIMENT.md),
[BLUETOOTH_ABI_1252.md](../BLUETOOTH_ABI_1252.md).

## Cztery oddzielne problemy

| Warstwa | Potrzebny dowód | Co go nie zastępuje |
|---|---|---|
| USB | Raport IN z realnym naciśnięciem i zidentyfikowana ścieżka transferu | Deskryptor, claim=0, dioda pada |
| Parowanie BT | Discovery, uwierzytelnienie/bond, zgodny wynik usługi systemowej | RegisterDevice lub socket HCI |
| Odbiór BT | Rzeczywisty raport z uzgodnionego kanału HID | Sama obecność urządzenia na liście |
| Wejście systemowe | PS, menu, gra, odłączenie i brak wymogu fizycznego DS4 | Hook tylko w grze lub parser na PC |

USB jest narzędziem diagnostycznym, nie warunkiem logicznym ukończenia BT.
Parowanie i odbiór mogą należeć do innych usług niż klient BluetoothHid.

## Technicznie różne architektury

| Architektura | Odbiór i dostarczanie wejścia | Warunek powodzenia | Eksperyment rozstrzygający / odrzucający |
|---|---|---|---|
| A. Rozszerzenie systemowej ścieżki DS4 | Systemowy bond i transport; rozpoznanie DualSense oraz normalizacja raportu przed wspólnym wejściem DS4. Bez nowego VDA. | Natywna usługa musi przyjąć połączenie i mieć uchwytną granicę klasyfikacji/raportu; kompletne ABI i własność zasobów. | Pasywna korelacja standardowej próby parowania z etapami usługi. Jeżeli odmowa następuje przed ACL/HID, sam translator raportu nie wystarczy. Modyfikacje dopiero po audycie i odrębnej zgodzie. |
| B. Własny klient L2CAP nad stosem Orbis | System obsługuje radio/uwierzytelnienie; klient otwiera HID Control/Interrupt, parsuje raporty i używa niezależnie zweryfikowanego wejścia globalnego. | Publiczny lub odtworzony współdzielony interfejs L2CAP, obsługa bezpieczeństwa i brak konfliktu HID. | Nowy gate sprawdził kontrakt socketów FreeBSD: **odmowa errno=43**. Ten konkretny wariant odpada w badanym procesie; inne API Orbis pozostają nierozpoznane. |
| C. Własny stos nad HCI/ACL | Własne discovery/SSP/SDP/L2CAP/HIDP, parser i wejście globalne; nie używa RegisterDevice. | Zweryfikowany transport do wewnętrznego radia, reguły współdzielenia i uprawnienia. Bez przejęcia radia w ciemno. | Raw HCI przez socket FreeBSD zwrócił errno=43. Port BTstack bez nowego backendu nie wystarczy. Sukces utworzenia socketu też nie dowodziłby podłączenia radia. |
| D. Podmiana u konsumentów scePad | Stan DualSense wczytuje hook Read/ReadState zamiast nowego urządzenia. | Dostęp do transportu oraz pokrycie Shell, gier, przycisku PS i cyklu połączenia. | Hook w jednej grze jest kontrolą ograniczenia, nie globalnym rozwiązaniem. Jeśli potrzebny jest stale fizyczny DS4 lub nie obejmuje Shell, wariant nie spełnia celu. Nie instalowano hooków. |

**Preferowana dalsza droga: A**, z ustaleniem stanu istniejącego połączenia,
kanałów HID i dostarczenia raportów przed translatorami. B pozostaje alternatywą
tylko po znalezieniu rzeczywistego interfejsu Orbis. C ma największy koszt i
ryzyko konfliktu z systemowym właścicielem radia. D nie rozwiązuje transportu.
Nie wykazano jeszcze wykonalności żadnej kompletnej drogi na 12.52.

Nie projektujemy obejścia uwierzytelnienia kontrolera ani stałego fikcyjnego
DS4. Zachowanie istniejącego slotu wymaga obsługi połączenia i tożsamości,
nie tylko zamiany tablicy przycisków. Sama zmiana VID/PID nie tłumaczy różnic
formatu, inicjalizacji, output i polityki parowania.

## Co można zastąpić

| Składnik | Ocena po rewizji |
|---|---|
| Parser i wspólny stan | Użyteczny niezależny komponent; konkretna implementacja wymienna. Format raportów musi rozumieć któraś warstwa. |
| Mapowanie do 120 B scePad | Potrzebne obecnemu VDA/hookom; zbędne przy natywnym translatorze niżej w potoku. Nie jest formatem surowego DS4 HID. |
| Control4Free/VDA | Opcjonalny sink. Nowy ELF i ACL/HIDP nie linkują vda.c ani ps4_pad.c. Wiedza o ABI VDA nie rozwiązuje transportu ani parowania. |
| sceUsbd | Opcjonalny backend USB. Własny backend nadal podlega polityce sterownika i własności endpointów. |
| sceBluetoothHidRegisterDevice | Wymagany tylko przez dotychczasowy wariant klienta; A/B/C mogą mieć inną drogę. Nie udowodniono, że funkcja wykonuje pairing. |
| GoldHEN | Dostarcza środowisko uruchamiania; nie stanowi dowodu dostępu HCI lub możliwości globalnego wejścia. |

## Co nowego wnosi audyt publicznych źródeł

- [Linux hid-playstation](https://github.com/torvalds/linux/blob/cd4f494ba535e848e2d299707d363de84e61a518/drivers/hid/hid-playstation.c)
  obsługuje USB/BT nad transportami kernela Linux; rozróżnia raporty i CRC.
  Nie jest implementacją parowania ani przenośnym zamiennikiem Orbis. GPL-2.0-or-later;
  kod nie został skopiowany. Przypięta kopia: `linux-source.json`.
- [FreeBSD ps5dsense](https://github.com/freebsd/freebsd-src/blob/main/sys/dev/hid/ps5dsense.c)
  w audytowanej rewizji ma **BUS_USB**, hidbus/hidmap/evdev i licencję BSD-2-Clause.
  Jego własna instrukcja opisuje USB. Nie traktujemy go jako gotowego sterownika
  Bluetooth ani KLD możliwego do załadowania na PS4.
- [Bluepad32](https://github.com/ricardoquesada/bluepad32/blob/main/src/components/bluepad32/bt/uni_bt_bredr.c)
  rozdziela kanały HID Control/Interrupt i korzysta z BTstack. Wspiera DualSense
  przez BR/EDR; transport ESP32/Posix nie daje backendu radia PS4. Apache-2.0
  projektu nie usuwa osobnych warunków zależności.
- [BTstack](https://github.com/bluekitchen/btstack/blob/master/LICENSE)
  dostarcza host HCI/L2CAP/HID i przykłady. Audytowana licencja ma ograniczenie
  użycia komercyjnego; nie dołączamy jego kodu do projektu GPL. Nowy odbiornik
  ACL jest własną implementacją, bez bibliotek BTstack/Bluepad32.
- [FreeBSD Bluetooth sockets](https://man.freebsd.org/cgi/man.cgi?query=ng_btsocket&sektion=4&manpath=FreeBSD+13.5-RELEASE)
  opisują PF_BLUETOOTH i typy HCI/L2CAP. Nagłówki payload SDK odziedziczyły
  te stałe. Właśnie ich dostępność na Orbis została sprawdzona, nie założona.
- [PS4 Linux](https://github.com/sony-jaguar-devs/ps4-linux)
  ma modele z różnymi układami i transportami radia, w tym SDIO. Nie zakładamy,
  że „Slim” oznacza konkretny chip lub HCI po USB. Uruchomienie Linux zamiast
  Orbis nie zachowuje celu menu i natywnych gier, więc nie jest rozwiązaniem.
- [GoldHEN gamepad_helper](https://github.com/GoldHEN/GoldHEN_Plugins_Repository/tree/main/plugin_src/gamepad_helper)
  pokazuje modyfikację danych na poziomie Read/ReadState w procesie gry.
  Publiczne nazwy PS4LibDoc, np. DeviceClassParseData/GetDataInternal/HidControl,
  nie dostarczają pełnego ABI. Nie wywołujemy ich placeholderów void().

Rewizje nowych klonów są w `../sources.lock.json`. Oprócz wymaganych źródeł
przejrzano BTstack, Bluepad32, FreeBSD HID/libusb i przypięty sterownik Linux.
Żadne moduły Sony, kody kernela ani klucze parowania nie zostały opublikowane.

## Root cause USB: uczciwy stan i eksperymenty

| Hipoteza | Obecny dowód | Warunek rozstrzygnięcia |
|---|---|---|
| Złe publiczne ABI InterruptTransfer | Przepływ argumentów sprawdzono w tej samej bibliotece. Naprawa sprzątania listy nie zmieniła błędu odbioru. | Znaleźć konkretną niezgodność argumentu/struktury przed jego użyciem. Bez tego brak podstaw do następnej zmiany sygnatur. |
| Uprawnienia/kontekst | count=1 w PayLoaderze, count=0 w aplikacji; O_RDONLY usbctl działa w obu. | Zlokalizować dokładny nieudany open/query, zachowując jego errno przed kolejną operacją. Nie zmieniać authid w celu „sprawdzenia”. |
| Własność endpointu/systemowy sterownik | Claim jest lokalnym bitem; nie dowodzi przejęcia endpointu. | Zidentyfikować odmowę w ścieżce konfiguracji/otwierania transferu i status backendu. Nie odłączać systemowego sterownika na próbę. |
| Błąd backendu/event loop | Sync API już obsługuje zdarzenia; -99 jest ogólnym wynikiem. | Zweryfikować pierwszy etap awarii, nie ostatnie errno. Nie dodawać drugiego HandleEvents bez dowodu. |
| Timeout/rozmiar | 64/128 B i natywny timeout nie zmieniły wyniku; błąd był natychmiastowy. | Nie powtarzać tych samych wariantów bez nowej przesłanki. |

Upstream libusb20 używa osobnych deskryptorów do control i transferów, a
niektóre błędy ioctl sprowadza do OTHER. To wskazówka do analizy, **nie dowód
zgodności ioctl, struktur czy zabezpieczeń Orbis**. Nie implementowano nowego
sterownika USB przez wklejenie FreeBSD. Najpierw potrzebny jest potwierdzony
punkt obserwacji błędu; nowe aktywne hooki wymagają osobnego przeglądu ryzyka
i zgody przed uruchomieniem. Obecny eksperyment żadnego hooka nie instaluje.

## Wybrany eksperyment i wynik

Wybrano bramkę dostępności HCI/L2CAP: rozstrzyga warunek dwóch alternatywnych
architektur, nie powtarza odczytu 0x84. Pełny protokół, wynik i instrukcja:
[NATIVE_BT_GATE.md](NATIVE_BT_GATE.md). Kontrola IPv4 działała, oba kontrakty
BT zwróciły **errno=43**. To potwierdzony logiem warunek blokujący prosty port
stosu opartego na tych socketach w procesie PayLoadera tej konsoli.

Następny eksperyment o najwyższym priorytecie po tym wyniku: pasywna korelacja
standardowej próby parowania z natywną ścieżką DS4, oddzielająca discovery,
ACL/uwierzytelnienie, akceptację HID i dostarczenie raportu. Do rozpoznania
pozostają ABI usługi parowania oraz natywna granica klasyfikacji. Nie ma jeszcze
podstaw do napisania rozszerzenia tej usługi lub patcha jądra. Bezpośredni
Bluetooth pozostaje celem; nie zastępujemy go USB lub urządzeniem zewnętrznym.
