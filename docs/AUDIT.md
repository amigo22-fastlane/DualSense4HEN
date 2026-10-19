# Audyt źródeł — 2026-10-19

Stan sprzętu przekazany przez właściciela: PS4 Slim, **12.52**, GoldHEN
**v2.4b18.12**. Po pierwszym commicie wykonano testy na konsoli właściciela;
wyniki są w `HARDWARE_RESULTS.md`. Poniższy audyt repozytoriów jest oddzielony
od późniejszych ustaleń ABI z własnego modułu opisanych w `BLUETOOTH_ABI_1252.md`.
Dokładne commity klonów zapisuje `sources.lock.json`. Linki poniżej wskazują
publiczne źródła; do reprodukcji należy wybrać zapisany commit.

## Wyniki analizy wymaganych projektów

| Projekt | Implementacja i zależności | Licencja i decyzja |
|---|---|---|
| [Control4Free](https://github.com/MoHadiShibli/Control4Free) | `src/vda.c`, `src/main.c`, `src/klog_line.c`, `include/c4f_sce.h`: globalny pad przez VDA/MBus; payload SDK, launcher OpenOrbis. Wejście z WebSocket jest oddzielne od emulacji. | GPL-3.0; adaptujemy ABI i sekwencję VDA w projekcie GPL. Nie potrzebujemy klienta sieciowego. |
| [Enhanced-PS5-DualSense-Haptics-Bridge](https://github.com/ragnarok179/Enhanced-PS5-DualSense-Haptics-Bridge) | Go; `Source/dualsense_input.go`: wspólne ciało raportu od bajtu 1 USB lub 2 BT. `bluetooth_main_windows.go`, `bluetooth_radio_disconnect_windows.go`, `usb_hid_windows.go`: zależności Windows HID/audio, nie PS4. | MIT, Ragnarok179; przydatna dokumentacja i porównanie offsetów, bez kopiowania kodu ani uruchamiania EXE/updatera. |
| [ClutchBenefactor/dualsense-controller](https://github.com/ClutchBenefactor/dualsense-controller) | Klon zawiera losowo nazwane pliki, DLL/ENC, placeholdery i C z niepoprawnymi identyfikatorami (`utils/middleware_14.c`). `license.md` zawiera reklamowy opis programu, nie tekst licencji. Nie znaleziono spójnego parsera HID/PS4 ani weryfikowalnego procesu budowy. | Brak wykazanej licencji na użyteczny kod. Wyłączony z implementacji; nie wykonujemy jego binariów. Nie wyciągamy z tego wniosku o złośliwości. |
| [nondebug/dualsense](https://github.com/nondebug/dualsense) | HTML/WebHID, deskryptory USB/BT i identyfikatory Sony 054c:0ce6. Krótki BT ma inne pozycje przycisków/triggerów. Odczyt feature 0x05 włącza pełny tryb BT według projektu. API WebHID nie jest dostępne na Orbis. | Brak pliku licencji w klonie; wyłącznie odniesienie do protokołu. Nie kopiujemy JS/HTML/deskryptorów. |
| [xbox_controller_plugin](https://github.com/crucifix86/xbox_controller_plugin) | `src/usb_xbox.c`: `sceUsbdGetDeviceList/Open/ClaimInterface/InterruptTransfer`. Hooki `scePad*` działają w grze, nie tworzą globalnego MBus DS4. Na sztywno interfejs 0 i endpoint Xbox nie nadają się do kompozytowego DualSense. | README deklaruje MIT, ale brak osobnego LICENSE w klonie. Nie kopiujemy implementacji. Prototyp USB bazuje na zweryfikowanych deklaracjach, skanuje deskryptory. |
| [GoldHEN_Plugins_Repository](https://github.com/GoldHEN/GoldHEN_Plugins_Repository) | `plugin_src/gamepad_helper`, `plugin_src/button_swap` i przykłady hooków: użyteczne do wejścia w grze. Brak backendu parowania/surowego Bluetooth DualSense w analizowanym drzewie. | GPL-3.0; analiza, bez przenoszenia hooków. |
| [DualSenseBridge](https://github.com/spsokhi/DualSenseBridge) | C++20, `src/DualSenseDevice.cpp`: SetupAPI/HidD, ReadFile, pełne raporty i CRC. `CMakeLists.txt` wymaga Windows i ViGEmClient (pobierany z `master`), touchpad używa SendInput. Emulowany X360 na PC, nie DS4 na PS4. | MIT, Sukhpreet Singh. Analiza, bez kopiowania Windows backendu. |

## Dodatkowe projekty

- [remotePad-ng](https://github.com/MoHadiShibli/remotePad-ng), GPL-3.0:
  hooki odczytu pada w grze, dane z sieci. `src/pad/usb.c` jest pustym
  szkieletem; nie dowodzi odbioru HID z DualSense ani parowania na PS4.
- [OpenOrbis-PS4-Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain),
  GPL-3.0: `include/orbis/Usbd.h` ma rzeczywiste prototypy `sceUsbd*`.
  Część dalszych eksportów ma jedynie `void foo()` jako placeholder:
  **nie używamy** tak zadeklarowanych detach/attach/kernel-driver funkcji.
  Repozytorium źródłowe nie zawiera zbudowanych stubów w `lib/`.
- [GoldHEN_Plugins_SDK](https://github.com/GoldHEN/GoldHEN_Plugins_SDK), MIT:
  syscall 500 polecenia 0–7: wersja SDK, klog, jailbreak/restore, informacje
  procesu, RW, PRX load/unload. Brak udokumentowanego polecenia przechwytywania
  Bluetooth. Wersja SDK 0x100 **nie jest** numerem wydania GoldHEN 2.4b18.12.
- [ps4dev/ps4sdk](https://github.com/ps4dev/ps4sdk), Unlicense dla własnego kodu,
  BSD/MIT dla odziedziczonych komponentów: `sce/usbd.h` pokazuje podobne API USB, ale np.
  `FreeDeviceList` ma inną deklarację od OpenOrbis. Nie mieszamy ABI.
  `AF_BLUETOOTH` w odziedziczonym FreeBSD `sys/socket.h` nie potwierdza
  działającego socketu HCI/L2CAP na Orbis.
- [ps4-payload-dev/sdk](https://github.com/ps4-payload-dev/sdk), pliki CRT GPL:
  biblioteki i wrappery do ELF. Źródła `crt/kernel.c` rozróżniają firmware
  12.50 i 12.52. Nie ma wpisu 12.53. Standardowy CRT wywołuje `__patch_init`
  modyfikujący mmap oraz tymczasowo zmienia jail/root w `__rtld_init`.
  To trzeba uwzględnić przy deklarowaniu bezpieczeństwa payloadu; sam brak
  patchy we własnym `main` nie oznacza braku patchy w starcie SDK.
- [idc/ps4libdoc, gałąź 9.00](https://github.com/idc/ps4libdoc/tree/9.00):
  faktyczne nazwy i NID eksportów, **bez sygnatur i semantyki parametrów**.
  Nie ma licencji na kopiowanie bazy w klonie; zapisujemy własne wnioski i linki.
  Pierwsza próba błędnego adresu `ps4dev/ps4libdoc` nie powiodła się; właściwy
  właściciel to `idc`, gałąź ustalona przez `git ls-remote --heads`.
- [PadBridge-PS5](https://github.com/X-F1REBALL-X/PadBridge-PS5) pojawił się
  w indeksie wyszukiwania jako analogiczny projekt dla PS5. Bezpośredni
  `git ls-remote` zwrócił **Repository not found**, a klon nie uzyskał HEAD.
  Nie udajemy audytu jego źródeł ani nie traktujemy opisu jako dowodu ABI PS4.

## Mechanizm wirtualnego kontrolera

Control4Free ładuje `/system/common/lib/libSceMbus.sprx` przed wywołaniami
`libScePad`. Kolejność: `scePadInit()`, `scePadSetProcessPrivilege(1)`,
`scePadVirtualDeviceAddDevice(param,3)`. Param ma 32 bajty. W przykładzie
`userId=1` wywołuje systemowy wybór użytkownika. Następnie
`scePadVirtualDeviceInsertData(handle,&state)` dostaje 120 bajtów danych.
PS jest standardowym bitem 0x10000. DeleteDevice usuwa wirtualne urządzenie.

`AddDevice` nie zwraca handle. Control4Free koreluje nowy wpis
`SCE_MBUS_EVENT_DEVICE_ADDED ... [subType:2]` z `/dev/klog` po drenażu i markerze.
Czytnik jest wyłączny i może kolidować z klientem serwera klog GoldHEN.
Równoczesna inna sesja VDA/Remote Play może podważyć własność znalezionego ID;
nie wolno przejmować przypadkowych urządzeń ani traktować statusu jako handle.

Control4Free zmienia authid na `0x3800000000010003` i capabilities procesu
gospodarza przez SDK, zapisuje poprzedni stan i przywraca go przy wyjściu.
Zmiana dotyczy współdzielonego demona, nie osobnego procesu aplikacji.
Pierwszy adapter DualSense4HEN **nie implementuje** tych zapisów do jądra.
Przy niewystarczających uprawnieniach ma odmówić działania, a nie zgadywać offsety.

Według [tabeli Control4Free](https://github.com/MoHadiShibli/Control4Free/blob/main/docs/installation.md)
wymagane GoldHEN to co najmniej v2.4b18.10; 12.52 jest tam **nieprzetestowane**.
Zgłoszone v2.4b18.12 spełnia warunek wersji, nie stanowi testu ABI ani sprzętu.
Nie ma podstaw do deklarowania zgodności 12.53 ani do używania offsetów 12.52
na 12.53. Korekta użytkownika usuwa 12.53 z bieżącego celu.

## Stan Bluetooth

W `system/common/lib/libSceBluetoothHid.sprx.json` PS4LibDoc 9.00 występują
`sceBluetoothHidInit`, `ParamInitialize`, `ThreadParamInitialize`,
`RegisterCallback`, `UnregisterCallback`, `RegisterDevice`, `UnregisterDevice`,
`GetDeviceInfo`, `GetDeviceName`, `GetReportDescriptor`, `GetInputReport`,
`GetFeatureReport`, `SetFeatureReport`, `SetOutputReport`, `InterruptOutput`,
`DisconnectDevice`, `DebugGetVersion`. `SceSysCore` importuje Init,
GetDeviceInfo i DisconnectDevice. **To dowód istnienia interfejsu HID,
nie dowód obsługi DualSense, parowania ani dostępności dla payloadu.**

Nie znaleziono w analizowanych projektach zweryfikowanego ABI rejestracji
callbacku/urządzenia, discovery, bond keys ani sposobu obejścia odmowy
systemowego parowania DualSense. Nie zakładamy, że `RegisterDevice` paruje
radio: może tylko przyłączać klienta do już zarejestrowanego urządzenia.
Nie potwierdzono, że modyfikacja jądra jest potrzebna; najpierw należy ustalić
miejsce odmowy (UI, polityka systemowego demona, rozpoznawanie HID, czy sterownik).

Następny eksperyment: odczyt wersji systemu i obecności eksportów na 12.52,
próba systemowego wykrywania z Create+PS i zapis logów, następnie porównanie
wywołań biblioteki w legalnie pozyskanych modułach **tej samej** konsoli.
Nie wywoływać nieznanych funkcji z wymyśloną strukturą ani callbackiem.

Późniejszy lokalny audyt modułu rozstrzygnął część sygnatur i układ callbacku.
Nie dowiódł semantyki parowania. Test potwierdził Init i callback oraz błąd
rejestracji 0x81540002. Nie nadajemy temu kodowi wymyślonej nazwy błędu.

## OpenOrbis i diagnostyka w zwykłej aplikacji

Dodano oficjalną dystrybucję OpenOrbis v0.5.4 jako zewnętrzne narzędzie
(archiwum i hash w BUILD.md), lokalne źródła OpenOrbis/musl oraz LibOrbisPkg
z rewizjami w sources.lock.json. Normalny CRT aplikacji i statyczna musl są
oddzielne od patchującego CRT payload SDK, którego projekt nie linkuje.
COPYRIGHT musl, licencja compiler-rt i NOTICE trafiają do własnego PKG.
LibOrbisPkg jest narzędziem zewnętrznym; jego walidator potwierdza format
Content ID z czteroliterowym prefiksem tytułu, co ujawniło błąd pierwszej paczki.

Wykonane pomiary wykazały, że aplikacja OpenOrbis 1.02/1.03 widzi pustą listę
sceUsbd, choć PayLoader przy tym samym kablu wykrywa DualSense. Otworzenie
/dev/usbctl z O_RDONLY działa w obu kontekstach. To własne obserwacje Orbis,
nie przeniesienie modelu uprawnień Linux/FreeBSD. Nie zdiagnozowano dokładnej
odmowy backendu, nie wdrożono zmian authid, praw urządzeń ani hooków ioctl.
