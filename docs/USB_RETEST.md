# Ponowny test USB z kablem danych

Cel: enumeracja DualSense, odczyt standardowych deskryptorów urządzenia,
konfiguracji, interfejsów i endpointów, następnie raportów interrupt IN.
Nie jest to końcowe rozwiązanie Bluetooth ani test sterowania menu/grą.
Docelowy sprzęt: PS4 Slim, 12.52, GoldHEN v2.4b18.12.

## Ocena poprzedniego ELF

Stary ELF miał ograniczony czas odbioru i zwalniał interfejs, ale późniejszy
audyt ujawnił niepełną sygnaturę FreeDeviceList w użytym nagłówku OpenOrbis.
Do kolejnych prób używaj rewizji 6 z jawnym drugim argumentem zwalniania listy.
Ta korekta zastępuje wcześniejszą ocenę możliwości ponownego użycia bez zmian.
Enumeracja pozostaje jednorazowa: podłącz kontroler przed wysłaniem ELF.

Do nowego testu zalecana jest przygotowana **rewizja 6**, w tym samym pliku
`dist/diagnostics/dualsense4hen-usb-probe.elf`. Poprzedni plik pozostaje w
historii Git (`53facea`, SHA-256
`a241fb4f2a92d859a469786e7378a32690ea57070bfea68152d746fed835d980`).

Rewizja 2 loguje wszystkie odczytane VID/PID i statusy odczytu deskryptorów,
konfigurację celu oraz interfejsy i endpointy. Dekoduje i mapuje każdy poprawny
raport do 120-bajtowego stanu PS4; zapisuje do 120 próbek rozmieszczonych co
najmniej 250 ms w całym oknie. Nie wywołuje VDA, nie zmienia uprawnień i nie
tworzy DS4. Rewizja 5 dodała odczyt deskryptora raportów HID i pojedynczą
próbę GET_REPORT(Input, ID 1), ze snapshotem errno. Rewizja 6 poprawia ABI
zwalniania listy oraz obsługę natywnego timeoutu.
Próbuje przejąć wyłącznie standardowy DualSense `054c:0ce6`, HID alt 0 z
interrupt IN o rozmiarze pakietu co najmniej 64 B; inne urządzenia tylko opisuje.

## Dotychczasowy wynik nowego kabla

Testy wykonano zdalnie za zgodą i po potwierdzeniu gotowości właściciela;
nie trzeba uruchamiać dodatkowej kopii w PowerShell. API wykryło jeden
DualSense 054c:0ce6, odczytało konfigurację i 289-bajtowy deskryptor raportów
HID, Open/Claim/Release zwróciły 0. Nadal nie odebrano raportów wejściowych:
interrupt 128/64 B zwraca 0x802400ff. GET_REPORT zwraca 0x80240004.
Rewizja 5 zapisała przy tych wywołaniach odpowiednio errno=35 i errno=5.
Sam errno po wieloetapowej operacji nie rozstrzyga, które wewnętrzne wywołanie
było przyczyną. Szczegóły i wariant alt 0: `HARDWARE_RESULTS.md`.

## Przygotowanie i uruchomienie

1. Pozostaw działający DualShock 4 do obsługi konsoli. Podłącz **DualSense z
   PS5** nowym, sprawdzonym na komputerze kablem bezpośrednio do portu USB PS4.
   Kabel musi być teraz przy konsoli, nie przy komputerze. Odczekaj 5 sekund.
2. GoldHEN: włącz FTP Server (2121) i PayLoader Server (9090). Zostań w menu
   systemowym; nie uruchamiaj równocześnie innych payloadów. Nie dodawaj ELF do
   AutoRun ani plugins.ini. Zwykły test USB nie wymaga observera Bluetooth.
3. Z katalogu repozytorium wykonaj w PowerShell poniższe polecenia. Ścieżka
   Python dotyczy środowiska właściciela; poza nim użyj swojego Python 3.

```powershell
$ps4Ip = "WPISZ_ADRES_IP_PS4"
$python = "$env:USERPROFILE/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe"
& $python tools/console-probe.py --host $ps4Ip --ftp-port 2121 --loader-port 9090 --elf dist/diagnostics/dualsense4hen-usb-probe.elf --wait 35 --output build/hardware/usb-new-cable-01.log --print-lines 30
```

Wstaw aktualny lokalny adres konsoli w zmiennej `$ps4Ip`. Skrypt najpierw czyta
stan logu, sprawdza brak pliku STOP, wysyła ELF i po 35 s pobiera wynik.
Samo wysłanie TCP nie potwierdza wykonania. Nie uruchamiaj ponownie podczas
trwającego odczytu; po zakończeniu można powtórzyć test z nazwą `02`.

Po komunikacie **ELF sent** zacznij sekwencję. Każdy przycisk trzymaj około
sekundy, potem puszczaj. Orientacyjnie: 0–3 s spoczynek; 3–10 s krzyżak i
cztery przyciski akcji; 10–16 s oba analogi i L3/R3; 16–21 s L1/R1 i płynne
L2/R2; 21–26 s Create, Options, krótko PS (bez przytrzymywania); 26–30 s ruch
palcem po touchpadzie i klik. Ponieważ interfejs może odmówić przejęcia albo
urządzenie może nie zostać znalezione, ta sekwencja sama w sobie nie jest
potwierdzeniem odbioru. Odnotuj reakcję menu lub komunikaty konsoli.

## Jakie logi odesłać

- `build/hardware/usb-new-cable-01.log`: tylko dopisany fragment tego testu.
- `build/hardware/usb-new-cable-01.json`: SHA-256 ELF, czas UTC, rozmiary logu.
- Informację: który port USB, czy DualSense świecił, czy menu działało i jakie
  przyciski wykonano. Próbki kontrolera nie są jeszcze dowodem działania VDA.

Skrypt zachowuje też `.cumulative.log` lokalnie. Nie trzeba publikować całego
skumulowanego logu ani klog, które mogą zawierać dane innych sesji. Jeśli nie
ma nowego fragmentu lub końcowego `probe exit=`, wykonanie/zakończenie jest
niepotwierdzone. Po dalszych 10 s pobierz pełny log bez ponownego startu ELF:

```powershell
& $python tools/console-probe.py --host $ps4Ip --fetch-only --wait 0 --output build/hardware/usb-new-cable-fetch.log
```

Przy braku logu włącz również Klog Server (3232). W drugim terminalu ustaw również `$ps4Ip`, a **przed**
wysłaniem ELF, można zapisać rozruch:

```powershell
& "$env:USERPROFILE/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe" tools/capture-klog.py --host $ps4Ip --seconds 40 --output build/hardware/usb-new-cable-01.klog
```

Wystarczy jeden odbiornik klog. Odesłać najpierw linie `[d4h]` i komunikaty
loadera dotyczące tego ELF; pełny log systemowy pozostaje lokalnie.

## Interpretacja i kolejny krok

| Wynik w nowym fragmencie | Znaczenie i następny eksperyment |
|---|---|
| `count>0`, `VID=054c PID=0ce6` | DualSense wyliczony przez API; sprawdzić konfigurację i claim. |
| `count>0`, inne VID/PID | Enumeracja działa częściowo; sprawdzić port, kabel, faktyczną odmianę kontrolera. Nie zmieniać PID na ślepo. |
| `ClaimInterface` niezerowe | Nie odłączać sterownika siłą. Zbadać własność interfejsu i kontekst procesu. |
| `usb-raw`, `usb-parse OK`, `ds4-dry-run bytes=120` | Odbiór, parser i mapowanie są w jednym potoku; porównać ze zgłoszonymi czynnościami. |
| `USB summary good>0`, `ReleaseInterface=0`, `probe exit=0` | Sonda zgłasza odbiór i poprawne zakończenie; wymagana ocena logu i potwierdzenie użytkownika. |
| Nadal `count=0` | Powtórzyć na drugim porcie (po zakończeniu pierwszej sondy). Porównać z testem bez DualSense. Ewentualnie użyć znanej pamięci USB tylko jako kontroli enumeracji, bez otwierania i zapisu. |
| Odbiór po czym `io_error` i exit=-6 | Częściowy odbiór zakończony błędem, np. odłączeniem. Nie oznaczać jako pełny sukces. |

Jeśli obydwa porty i urządzenie kontrolne pozostają niewidoczne, kolejnym
eksperymentem jest porównanie PayLoader/ScePartyDaemon z osobną aplikacją
homebrew i audyt ABI/backendu sceUsbd oraz istniejących uprawnień. To hipotezy
do sprawdzenia, nie dowód konieczności patcha jądra. Nie przenosimy do Orbis
linuksowych ioctl ani /dev/hidraw. Odczyt innej biblioteki Sony wymaga osobnej
zgody; dotychczasowa zgoda dotyczyła libSceBluetoothHid.

## Zatrzymanie i wycofanie

Sonda kończy się po maksymalnie 30 s lub 30000 odczytów. Nie odłączaj kabla
w trakcie pomiaru podstawowego. Awaryjne żądanie zatrzymania:

```powershell
& $python tools/console-probe.py --host $ps4Ip --stop
```

Tworzy wyłącznie `/data/dualsense4hen.stop`; kolejny start będzie celowo
zablokowany. Przed świadomym powtórzeniem usuń ten konkretny plik przez FTP.
Nie usuwaj pliku blokady podczas działającej sondy. USB nie zapisuje konfiguracji
parowania. Zasady wcześniejszego observera, którego moduł BT może pozostać w RAM,
opisuje `INSTALL_ROLLBACK.md`; nie uruchamiaj go ponownie w ramach testu USB.
