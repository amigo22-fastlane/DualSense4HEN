# Istniejące połączenie Bluetooth — PS4 12.52

Stan na 2026-10-20, GoldHEN v2.4b18.12. Zachowujemy bieżącą sesję.
Nie używać Create+PS, usuwania urządzenia, ponownego parowania ani restartu
w celu powtórzenia observera. Starsze instrukcje parowania nie dotyczą tej próby.

## Dowody i granice

| Etap | Dowód | Status |
|---|---|---|
| Rozpoznanie w UI | Właściciel: nazwa DualSense, typ Kontroler, zielone Połączono | Potwierdzone przez właściciela |
| Zachowanie bez USB | Właściciel: połączenie i podświetlenie trwają bez kabla; lokalny klog zawiera wcześniejsze odłączenie USB DualSense | Potwierdzona obserwacja użytkowa |
| ACL, uwierzytelnienie, szyfrowanie, trwały bond | Brak śladu protokołu lub zweryfikowanego zapytania stanu | Nieustalone; UI nie zastępuje tych dowodów |
| L2CAP HID Control PSM 0x11 | Brak informacji o uzgodnionym CID i stanie kanału | Nieustalone |
| L2CAP HID Interrupt PSM 0x13 | Brak informacji o uzgodnionym CID i stanie kanału | Nieustalone |
| Odbiór HID | Pierwsze istniejące połączenie: zero zdarzeń naszego callbacku | Nie zaobserwowano; nie dowodzi braku pakietów |
| Rejestracja scePad | Pierwszy pomiar nie otworzył klienta Pad | Nieustalone |
| Menu i przycisk PS | Właściciel: DualSense nimi nie steruje | Potwierdzony objaw, nie lokalizacja przyczyny |

Nie ma podstaw, aby uznać urządzenie za wyłącznie „ogólne Bluetooth” albo
stwierdzić, że kanał HID działa. Nie ma też podstaw, aby uznać brak działania
za odmowę parowania: interfejs systemowy już pokazuje aktywne połączenie.

## Co mierzy narzędzie

`connected_bt_probe.c` i wariant `bt_observer.c` z D4H_BT_CONNECTED oraz
D4H_BT_PASSIVE używają istniejącej usługi, bez RegisterDevice,
UnregisterDevice, DisconnectDevice, SetReport, VDA lub zmiany przywilejów.
Nie linkują Control4Free, USB ani własnego stosu Bluetooth.

To subskrypcja klienta HID, **nie sniffer HCI**. Zatwierdzona lokalna analiza
libSceBluetoothHid pokazuje, że RegisterCallback zapisuje komendę subskrypcji
do własnego deskryptora /dev/bluetooth_hid i tworzy watcher. Nie jest to
dosłownie operacja bez żadnego zapisu. Nie wysyłamy tej komendy bezpośrednio.
Watcher zaczyna z pustym czteroelementowym cache; nie wykazano, że istniejące
połączenia są odtwarzane nowemu subskrybentowi. Brak callbacków nie wyklucza
raportów, kanałów ani dostarczania zdarzeń innemu klientowi systemowemu.

Weryfikacja pełnego segmentu kodu poprzedza użycie znanego, odczytywanego
slotu uchwytu watchera; szczegóły w [ABI](../BLUETOOTH_ABI_1252.md).
UnregisterCallback i join kończą własny watcher. Biblioteka pozostaje w
gospodarzu z powodu braku potwierdzonego Term. Narzędzie odmawia ponownego
przejęcia obecnego kontekstu. Nie restartować konsoli, by obejść tę odmowę.

Parser nie uznaje samego formatu za tożsamość DualSense. Rozdziela poprawny
raport z metadanymi 054c:0ce6, raport 0x31 z CRC bez potwierdzonej tożsamości
i wspólny minimalny format 0x01. Zachowuje ograniczoną liczbę próbek także
wtedy, gdy parser je odrzuca. Pola typu i statusu nadal są zapisywane surowo.

`pad_snapshot.c` używa rzeczywistych typów OpenOrbis i ReadState zamiast
opróżniania kolejki Read. Najpierw GetHandle, potem ewentualnie Open normalnego
klienta standardowego portu indeks 0, dla zalogowanych użytkowników. Zamyka
wyłącznie własne uchwyty, zachowuje pożyczone. Nie jest to globalny spis
kontrolerów, a PadInformation nie daje tu identyfikacji VID/PID.

Wariant **dualsense4hen-pad-view.elf** w ogóle nie inicjuje subskrypcji BT.
Może kontrolować widok Pad po pierwszym observerze, zachowując jego moduł
i fizyczne połączenie. Zainicjalizowany własny Pad i zależność Mbus pozostają
w procesie, gdyż nie mamy zweryfikowanego PadTerm. Nie zmienia to konfiguracji
autostartu. W razie nieudowodnionego końca callbacku kod pozostaje rezydentny;
nie zwalniamy go spod działającego wątku ani nie wymuszamy restartu.

## Pomiar 1: 2026-10-20 00:04:30 czasu Polski

Lokalny log `build/hardware/bt-connected-01.log`, tylko przyrost nowej próby:

- firmware 0x12520001, wersja 12.520.001;
- UserService Init zwrócił 0x80960003; sonda rev1 przerwała przygotowanie Pad;
- BluetoothHid Init=0, RegisterCallback=0;
- fazy 0/1/2 w 0/5045/20087 ms, powiadomienia=0;
- UnregisterCallback=0, join=0;
- callbacks/events/parsed/invalid = 0/0/0/0, complete=0, exit=0.

Właściciel potwierdził wykonanie X na DualSense i L2 na DS4 oraz dalsze
działanie menu i istniejącego połączenia Bluetooth bez kabla.

ELF rev1: SHA256 `21ad39a0618a3506e0c19718620a15a5b36d4fb0d9a6c8e8007bcd9addef5a8f`.
Log: SHA256 `5bcb72de1a7f28992378dcb236e777b5ffa1eec0f5f50526a6ba7b29f568c55e`.
Pełny klog pozostaje lokalnie; nie zawiera rozstrzygającego śladu kanałów HID.

OpenOrbis `orbis/_types/errors.h` oznacza 0x80960003 jako
ORBIS_USER_SERVICE_ERROR_ALREADY_INITIALIZED. Rev1 błędnie wymagała nowego
Init przed zapytaniem i nie wykonała porównania Pad. Rev2 najpierw wykonuje
odczyt listy zalogowanych użytkowników, zachowując istniejący stan. Tylko po
niepowodzeniu zapytania we własnym module próbuje Init ze strukturą SDK;
Terminate wywołuje wyłącznie po własnym Init=0. Nie terminujemy stanu hosta.

## Pomiar 2: 2026-10-20 00:07:29 czasu Polski

Uruchomiono wariant Pad-only rev2, bez ponownej subskrypcji Bluetooth.
Zapytanie login-list **przed Init** zwróciło 0: poprawka usuwa błąd sondy,
nie wymaga ponownej inicjalizacji ani Term istniejącego stanu UserService.

- scePadInit=0; otwarto jeden własny standardowy uchwyt zalogowanego użytkownika;
- GetControllerInformation=0, connected=1, class_raw=0, connection_type_raw=0,
  count_raw=1; nie jest to identyfikacja DualSense;
- ReadState: 347 odczytów, 0 błędów, 347 razy connected=1;
- idle_X=0, DualSense_phase_X=0, **DS4_control_L2=0**;
- Close własnego uchwytu=0, complete=0, exit=0.

Właściciel potwierdził poprawne wykonanie obu faz, działanie DS4 w menu i
dalsze połączenie DualSense. Otwarty klient Pad **nie przeszedł kontroli
dodatniej DS4**. Nie wolno uznać tej próby za dowód, że DualSense nie jest
zarejestrowany globalnie. Możliwy wpływ kontekstu/fokusu/ograniczenia klienta
pozostaje hipotezą; nie wywołano SetProcessPrivilege ani SetApplicationFocus.
Nie powtarzamy tej samej próby ze zgadywanymi flagami.

ELF: SHA256 `6f2b5536d32339480f011b480a60680a29e0b343c1012102163c183a8cd4eefb`.
Log `build/hardware/bt-pad-view-01.log`:
SHA256 `1d7dc6d0468099d1add7eff74a34460211ad428e72610ec1888c6de685967880`.
Wyniki liczbowe bez kodu Sony: [existing-bt-results.json](existing-bt-results.json).

## Instalacja i wykonanie

Nic nie instalować i nie podłączać DualSense przez USB. ELF przesyłamy
jednorazowo do istniejącego GoldHEN PayLoader 9090, po uzgodnieniu gotowości.
Nie włączać AutoRun. Homebrew PKG nie jest wymagany.

1. DualShockiem 4 wrócić na ekran główny PS4. Oba pady pod ręką.
2. Poczekać na powiadomienia: 5 sekund bez naciskania obu padów.
3. Przez kolejne 15 sekund naciskać i puszczać X na **DualSense z PS5**.
4. Przez kolejne 15 sekund wciskać i puszczać L2 na **DualShocku 4 z PS4**.
   Nie naciskać X na DS4, żeby nie uruchamiać zaznaczonego elementu menu.
5. Pozostawić bieżące połączenie i zgłosić wykonanie oraz ewentualny błąd.

Zbieramy przyrost /data/dualsense4hen-probe.log przez FTP oraz opcjonalnie
45 s klog. Właściciel nie musi uruchamiać PowerShella ani ręcznie przesyłać
logu, jeżeli serwery 192.168.88.7 pozostają dostępne. Zapisujemy hash ELF,
czasy UTC oraz liczbę bazowych bajtów; stary końcowy exit nie potwierdza próby.

Budowanie: `powershell -File tools/build-research.ps1 -ConnectedBt` lub
`-PadOnly`. Wymaga lokalnego Zig, nagłówków payload SDK i typów OpenOrbis;
nie używa patchującego CRT. `tools/check-elf.py` sprawdza strukturę pliku,
a nie zgodność sprzętową. Analiza: `python tools/bt_session_log.py LOG`.

## Interpretacja i następny rozstrzygający krok

Kontrola DS4 L2 musi dać dodatni wynik bez błędów odczytu; inaczej brak X
nic nie mówi nawet o widoku Pad tego procesu. Poprawny odczyt BT z pasującymi
metadanymi i brak X przy udanej kontroli zawęziłby lukę między dostarczeniem
HID do klienta a obserwowanym Pad. Sam X w fazie DualSense daje korelację
czasową, jeszcze nie globalną identyfikację lub obsługę menu.

Jeśli callback nadal nic nie pokazuje, nie powtarzamy identycznej subskrypcji.
Potrzebne jest **zweryfikowane zapytanie do istniejącej sesji**: stan urządzenia,
stan obu kanałów/CID albo punkt wejścia raportu w usłudze natywnej. Obecnie
GetDeviceInfo otwiera /dev/bt, ale rozmiar i semantyka wskazywanego bufora
nie są ustalone. Nie wywołujemy go ze zgadniętym indeksem lub buforem.
GetDeviceName też nie ma potwierdzonego pełnego kontraktu. Następny audyt
musi znaleźć systemowego konsumenta i rzeczywiste struktury, zamiast zgadywać
ioctl, kody HCI lub dodawać własne parowanie. Odczyt nowych bibliotek Sony
wymaga osobnego określenia i zatwierdzenia zakresu, jeżeli okaże się potrzebny.
Po osobnej zgodzie wykonano audyt RX/R HidControl. Wykazał inny kontrakt
/dev/hid, nadal bez pełnego rozmiaru wyniku. Nie wykonano zgadywanego GetInfo:
[HID_CONTROL_ABI_EXPERIMENT.md](HID_CONTROL_ABI_EXPERIMENT.md).

Źródła interfejsu:
[OpenOrbis Pad](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/include/orbis/Pad.h),
[typy i błędy](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/include/orbis/_types/errors.h),
[UserService](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/include/orbis/UserService.h).

## Dostępna weryfikacja na komputerze

Zig `tools/test-host.ps1`: core 42403, ACL/kontrakty 21289, monitor BT 102,
Pad snapshot 34, observer registered/passive/connected 92/382/434,
USB 10911/11058/11233 — PASS. Osobno po uzupełnieniu propagacji błędów
audyt HidControl: 235 sprawdzeń PASS, także odmowa open, błąd write/close.
Python: 20 testów PASS, w tym siedem chroniących przed fałszywym wnioskiem
o kanałach, tożsamości, globalnym Pad lub użyciem starego sukcesu z logu.
Wszystkie trzy nowe ELF przeszły kontrolę struktury; bez importów i W+X.
Syntetyczne raporty i atrapy nie są dowodem działania DualSense na konsoli.
