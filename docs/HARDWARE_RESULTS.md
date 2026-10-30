# Pomiary sprzętowe — 2026-10-19

Konsola właściciela: PS4 Slim. Systemowy odczyt z własnego ELF:
`sceKernelGetSystemSwVersion -> 0`, `0x12520001`, `12.520.001`.
GoldHEN zgłoszony przez właściciela: v2.4b18.12; banner FTP: GoldHEN FTP v2.2.
Numer serwera FTP nie jest numerem GoldHEN. Adres LAN, pełne logi i binaria
Sony pozostają poza repozytorium.

| Eksperyment | Wynik | Co faktycznie potwierdza |
|---|---|---|
| Pierwsze ELF-y | loader potwierdza start, wczesny runtime odmawia relokacji | Nie był to jeszcze pomiar Bluetooth. |
| Poprawiony runtime | wejście ELF, relokacje, plik logu, odczyt wersji; exit=0 | Działający bootstrap w ScePartyDaemon na tej konsoli. |
| Biblioteka Bluetooth HID | załadowanie i odładowanie=0, obecne 17 eksportów | Dostęp do biblioteki z payloadu; nie dowód parowania. |
| MBus i Pad | załadowanie=0; obecne Init, SetProcessPrivilege, AddDevice, InsertData, DeleteDevice | Dostępność eksportów VDA; nie utworzono DS4. |
| Audyt ABI | 3 segmenty po 16384 B, lokalny odczyt za zgodą właściciela | Ustalenie sygnatur i struktury callbacku dla konkretnej biblioteki. |
| Observer: Init | 0 | Inicjalizacja libSceBluetoothHid działa w procesie payloadu. |
| Observer: RegisterCallback | 0 | Własny callback i watcher zostały uruchomione. |
| Observer: RegisterDevice(054c,0ce6) | **0x81540002** | Próba rejestracji została odrzucona; symboliczne znaczenie błędu nieustalone. |
| Observer: UnregisterCallback / scePthreadJoin | 0 / 0 | Callback wyrejestrowany i wątek zakończony, bez uwalniania kodu spod wątku. |
| Observer: raporty | callbacks=0, events=0, parsed=0 | Brak dowodu odbioru DualSense przez Bluetooth. |
| USB, wcześniejszy kabel | Init=0; GetDeviceList=0; lista ma pierwszy element NULL | Pomiar obciążony problemem kabla: późniejszy test na PC nie wykrywał przez niego DualSense. Nie dowodzi ograniczenia API PS4. |
| Nowy kabel USB-C, test PC zgłoszony przez właściciela | PC prawidłowo wykrywa DualSense | Kabel obsługuje dane USB na PC. Nie jest to jeszcze wynik PS4. |
| Nowy kabel, ponowny test PS4 | count=1, VID=054c PID=0ce6 | Enumeracja działa w procesie PayLoadera na tej konsoli. |
| HID z deskryptorów | interfejs 3 alt 0, IN=0x84, OUT=0x03, max packet=64 | Endpoint ustalony z rzeczywistej konfiguracji. |
| Open / Claim / Release | 0 / 0 / 0 | Cykl API poprawny; claim jest rezerwacją w procesie, nie dowodem dostępu sterownika do endpointu. |
| GET_DESCRIPTOR(Report) | zwrócono 289 B z żądanych 289, errno=0 | Odebrano deskryptor raportów HID, nie raport wejścia. |
| InterruptTransfer 128 B i 64 B | 0x802400ff, bytes=0, good=0 | Nie odebrano wejścia; sam rozmiar bufora nie wyjaśnia błędu. |
| GET_REPORT(Input, ID 1) | 0x80240004, errno=5 | Brak raportu; wrapper może tak tłumaczyć inne błędy backendu, nie dowodzi fizycznego odłączenia. |
| Jawny wybór HID alt 0 | 0x802400ff, errno=5 | Osobny wariant odmówił operacji; interfejs zwolniono. |
| InterruptTransfer, snapshot errno | 0x802400ff, errno=35 | Odczyt natychmiast po API; wewnętrzne operacje mogą nadpisywać errno. |

Pierwotny prototyp niewłaściwie traktował symbol bazy ELF jako ABS i nie
deklarował wszystkich rejestrów zmienianych przez syscall. Log startu pozwolił
to wykryć przed operacjami Bluetooth. Poprawiono symbole sekcyjne i constraints
asm; `check-elf.py` sprawdza teraz także symbole bootstrapu i odmawia ABS.
Nie zastosowano obchodzenia błędów relokacji ani patchy pamięci systemowej.

Końcowy `dist/diagnostics/dualsense4hen-bt-probe.elf` (SHA-256
`0ccc616482cd280f9fd6c61a3ba63deffb267be7e3e66fdb428e24789a3e2a0f`)
również uruchomiono na tej konsoli: zakończył się z `probe exit=0`.
Warianty USB i observer z dystrybucji powstały z tego samego kodu co testy
deweloperskie, po usunięciu sekcji debug; sprawdzono ich strukturę ELF.
Tymczasowe trzy pliki segmentów biblioteki usunięto następnie z konsoli;
lokalne kopie badawcze pozostają w ignorowanym katalogu.

`sceUsbdExit` miał w nagłówku deklarację zwracającą int; obserwowana wartość
nie nadawała się do interpretowania jako status. Prototyp wywołuje zakończenie
i ignoruje wynik. To kolejny powód, aby nie uznawać samych nagłówków za test ABI.

Po observerze biblioteka HID pozostaje załadowana bez callbacku i watchera.
Nie ma publicznego eksportu Term; konserwatywne wycofanie wymaga restartu
konsoli. Żadnego ELF nie dodano do AutoRun, żadnych ustawień parowania nie
zapisano. Nie zmieniano kodu jądra, firmware, plugins.ini ani authid procesu.

**Nie potwierdzono:** bezpośredniego parowania i reconnect, raportów wejściowych BT/USB,
wirtualnego DS4 na sprzęcie, PS/Options/Create w menu i grach, touchpada w
grach, rest mode ani pełnej kompatybilności. Nie deklarujemy ich działania.

## Korekta interpretacji USB i nowy ELF

Właściciel zidentyfikował poprzedni kabel jako niepozwalający wykryć kontrolera
na komputerze, a inny kabel jako poprawnie przesyłający dane. Poprzednia pusta
lista pozostaje autentycznym wynikiem API, ale nie izoluje wpływu kabla od
uprawnień, backendu lub kontekstu procesu. Najpierw należy powtórzyć pomiar.

Na etapie przygotowania rewizja 2 poprawiła obserwowalność:
loguje wszystkie VID/PID, konfigurację celu i próbki przez całe okno, następnie
mapuje odczytany stan do DS4 wyłącznie w podglądzie. Nie wywołuje VDA.
Hostowe testy nowych ścieżek nie stanowią pomiaru na konsoli.
Późniejszy audyt wykazał brakujący argument FreeDeviceList; do kolejnych
prób zalecana jest rewizja 6, zamiast wcześniejszego ELF bez zmian.
Szczegóły i wymagane logi: [USB_RETEST.md](USB_RETEST.md).

## Wykonane próby z nowym kablem

Właściciel potwierdził gotowość i podłączenie nowego kabla przed startem.
Po pierwszej próbie potwierdził normalne działanie menu i zgłosił zgaszenie
diody DualSense. Sondy nie wysyłają poleceń podświetlenia; dioda nie jest testem
dostępności USB. Kolejne próby nadal wyliczały kontroler. Nie potwierdzono
sterowania menu lub grą za pomocą DualSense.

| Próba lokalna | SHA-256 własnego ELF | Wynik |
|---|---|---|
| usb-new-cable-01, rev 2 | ca51d304e4d128eb8e33135934de9112e77e1a0eeeb1aeeb9eb0e11bc07940b7 | Enumeracja/config/claim OK; interrupt 128 B odrzucony. |
| usb-new-cable-02, rev 3 | d05ed0d287a6ae102ef949a82c8fe27f59f6acf8ac24be983629b40f57986ead | Interrupt 64 B odrzucony tak samo. |
| usb-new-cable-03, rev 4 | 12d44aeb418601e6f1da70053fec4bb99f1e35996d3647bf46baa192f96d4a0d | Deskryptor HID 289 B OK; GET_REPORT i interrupt bez wejścia. |
| usb-new-cable-04, alt 0 | 6a7d8000dbb175ec00979c08ebe657fd32b6129aae8b58bb2ff904d76b52a6e8 | SetInterfaceAltSetting odrzucone, release=0, exit=-7. |
| usb-new-cable-05, rev 5 | e65bb5ea14828b8a98eaeaf350ae78b46fc2e6de0013596ce118a4a98250d79c | Deskryptor errno=0; GET_REPORT errno=5; interrupt errno=35; exit=-6. |
| usb-new-cable-06, alt 0 + errno | b75dfe704a9a1b0b277dad739b3bd33f3ad909bcf85c2f1fc58ad7ead9828f37 | SetInterfaceAltSetting errno=5, release=0, exit=-7. |
| usb-new-cable-07, natywny timeout | a9818e53c4f32d1995cca07ed90b48c35d6a954e284e61b816a6d85d40cf05b8 | Wynik jak w próbie 05; timeout nie wystąpił. |
| usb-new-cable-08, końcowa rev 6 | 5a3fd7ff571de753792ac6a93ea251b12d7ac709989e932658e374a17beaf88d | Poprawione ABI zwalniania listy; nadal deskryptor OK, zero raportów, release=0, exit=-6. |

Pełne logi, klog i JSON z czasami UTC pozostają w ignorowanym build/hardware/.
Wszystkie osiem sond odnotowało zwolnienie interfejsu i koniec własnego
modułu USB. Nie włączano VDA ani zmian uprawnień.

## Zatwierdzony audyt USBD

Po osobnej zgodzie właściciela odczytano tylko RX (65536 B) i R (16384 B)
libSceUsbd. RW pominięto. Sonda zakończyła się exit=0, moduł odładowano.
Kopie pobrano i sprawdzono lokalnie, po czym oba utworzone pliki usunięto
z konsoli. Nie odczytywano jądra. Zakres i własne wnioski:
[USBD_ABI_EXPERIMENT.md](USBD_ABI_EXPERIMENT.md).

## Samodzielna aplikacja OpenOrbis

Zbudowano rzeczywisty PKG z oficjalnym OpenOrbis v0.5.4. Poniższe próby
dotyczą wyłącznie własnej aplikacji diagnostycznej, nie globalnego DS4.

| Paczka | Wynik właściciela / lokalnej kolekcji | Wniosek |
|---|---|---|
| Pierwsza 1.00, błędny TITLE_ID D4HN00001 | CE-32957-6 podczas instalacji | Błąd własnych metadanych: prefiks tytułu musi mieć cztery litery. Dodano walidację SFO/GP4/PKG i test regresji. |
| Poprawiona 1.00, TITLE_ID DSHN00001 | Instalacja i start potwierdzone; kilka powiadomień, CE-34878-0 | Klog potwierdza wejście do runtime i SIGSYS przy powrocie main do CRT _exit. Nie uzyskano pomiaru USB z aplikacji. |
| 1.01 | Instalacja/start potwierdzone, powiadomienia, czarny ekran, samoczynny koniec bez zgłoszonego błędu | Klog potwierdza wersję 1.01 i żądanie LoadExec ze ścieżką exit; system zapisał błąd uruchomienia tej ścieżki 0x80aa001a i zamknął tytuł. Brak wpisów [d4h] oraz globalnego pliku logu. Wyniku USB nie można wywnioskować. |
| 1.02 | Właściciel wykonał test X; pobrano log 2349 B | Init=0, GetDeviceList count=0, exit=-22. Zwolniono własny moduł USB. Brak odczytu endpointu i brak raportów. |
| Porównanie PayLoader po zamknięciu 1.02 | Ten sam kabel i podłączenie pozostawione przez właściciela | count=1, 054c:0ce6, deskryptor HID 289 B; interrupt 0x802400ff, errno=35, good=0, release=0, exit=-6. |
| Dostęp backendu w PayLoaderze | open(/dev/usbctl, O_RDONLY)=25, errno=0, close=0 | Sam dostęp do węzła kontrolnego działa. Bez ioctl spoza biblioteki; transfer nadal nie działa. |
| 1.03 | Właściciel wykonał test; pobrano log 2516 B | open(/dev/usbctl,O_RDONLY)=16/errno=0, close=0; Init=0/errno=0; count=0, errno=1 po enumeracji; exit=-22. |

Hashe PKG odróżniające próby:

- Odrzucona 1.00: `fc15d7fbde142e5d55a7f6f4498544e6d753207581527ba0dfeacde04bebbe92`.
- Poprawiona 1.00: `303963a8ec9838752f942bd6810d92160a3efd59aaf046e27f5dc23b78cacc55`.
- 1.01: `6fb7efe61351cd5bfe15a4110f13294546c1af4104adac89d04ac7a0bc645c00`.
- 1.02: `3554433c801c5ce13e453986e03fad5a6d1e6f5a7ac7250f4c2ad349df24e474`.
- 1.03: `73eef54edafbec1c6abb0cd618451ecde81ddc9371c077793a5737da78da8e24`.

Crash 1.00 skorelowano z własnym ELF i jego importem _exit, nie z odczytem
jądra. Nie ustalono w tej próbie dokładnej przyczyny wcześniejszego powrotu
runtime ani kodu odmowy otwarcia logu. Brak globalnego /data nie dowodzi sam
w sobie, że sceUsbd jest niedostępne. Wersja 1.02 używa zadeklarowanych
sceKernelDlsym/Open/Write i osobnego obszaru własnej aplikacji, bez obchodzenia
sandboxu. Nie zakładamy, że bezpośrednie syscalle payloadu są równoważnym
kontekstem aplikacji. `USB_APP.md` opisuje instalację i wymagane dowody.

Log 1.02 powstał w /data jako uid=1, mode=0600. FTP widziało plik w LIST, ale
odmawiało RETR. Po odrzuconej automatycznie pierwszej próbie uzyskano osobną
zgodę właściciela i zmieniono **wyłącznie ten log** przez SITE CHMOD na 0644.
Następnie pobrano go z globalnego /data. SHA-256 logu:
`6fa99126b4009331cd4c0950c9f209318a12ec65845a6d1caf050ce89c8c0f1c`.
Nie zmieniano praw węzłów urządzeń ani konfiguracji konsoli.

Porównanie potwierdza różnicę enumeracji między kontekstami w tej próbie,
nie konkretną przyczynę po stronie polityki dostępu, sandboxu lub backendu.
Nie potwierdza, że podniesienie uprawnień rozwiązałoby odbiór. W PayLoaderze
po poprawnej enumeracji errno=2 mimo count=1: kolejny dowód, że samo errno
po całym API nie jest statusem pojedynczego ioctl. Nie odebrano X.

Nowa sonda dostępu ma SHA-256
`d8382856e7faa86f5ff5629bed1199225797b738831af9af32997151056c8d3e`.
Ścieżka /dev/usbctl i O_RDONLY pochodzą z zatwierdzonego audytu backendu
tej biblioteki, nie z założenia o zgodności FreeBSD. Nie skopiowano jego ioctl.
Wyniki: lokalnie app-usb-102.log/json, usb-app-comparison-01.log/json i
usb-access-01.log/json. Żadnego zdarzenia X z symulacji nie zaliczono do
pomiaru sprzętowego. Pełne logi są wyłącznie w ignorowanym build/hardware.

Log 1.03 (`app-usb-103.log/json`) ma SHA-256
`2b421e02c1c1d6d35ccfeab74b2efccc2613ec7ed3f4c73a31bc63c5157b1532`.
Pobrano go bez kolejnej zmiany praw. Pokazuje, że pusta lista nie wynika
z samej niemożności otwarcia /dev/usbctl. Errno=1 po enumeracji jest zgodne
z etykietą EPERM z nagłówka SDK, lecz nie identyfikuje konkretnego ioctl ani
warunku sterownika. Nie wykonano ioctl bezpośrednio, obejścia uprawnień,
zmiany authid, praw urządzeń lub patcha jądra. W tej konfiguracji zwykła
aplikacja nie dostarczyła wejścia USB, a PayLoader nadal napotyka błąd transferu.

## Niezależna gałąź architektury po 13aa707

Eksperyment native-bt-gate nie powtarza USB. W procesie PayLoadera na tej samej
12.52 kontrola IPv4 utworzyła fd=25 i zamknęła go z wynikiem 0. Kontrakty
FreeBSD HCI (36/3/134) i L2CAP (36/5/135) zwróciły -1/errno=43. ELF zakończył
się exit=0; nie rejestrował urządzenia ani nie wysyłał komend radia.
Nie dowodzi to fizycznej niezgodności Bluetooth lub braku innych API Orbis.
Pełna interpretacja, hash i czas: [NATIVE_BT_GATE.md](research/NATIVE_BT_GATE.md).

## Istniejące połączenie Bluetooth — 2026-10-20

Właściciel potwierdził DualSense jako Kontroler z zielonym Połączono, także
bez kabla USB; podświetlenie trwa, lecz X i PS nie sterują menu. Nie zmieniano
tego połączenia, parowania ani konfiguracji.

Wykonano 35-sekundowy observer **bez RegisterDevice**. Init=0, callback=0,
trzy fazy i powiadomienia wykonane, unregister=0, join=0; callbacks/events/
parsed/invalid=0/0/0/0. Sonda rev1 nie wykonała kontroli Pad: own UserService
Init zwrócił 0x80960003 (ALREADY_INITIALIZED według OpenOrbis). To błąd
warunku wejścia sondy, nie wynik transportu DualSense.

Rev2 najpierw odczytała istniejącą listę użytkowników: ret=0 bez Init.
W osobnym wariancie Pad-only, bez nowej subskrypcji BT, otwarto normalnego
klienta scePad: Init=0, info=0/connected=1, 347 odczytów/0 błędów. Jednak X
w fazie DualSense=0 i kontrolny L2 na DS4=0. Właściciel potwierdził wykonanie
obu faz i dalsze działanie DS4 w menu oraz połączenia DualSense. **Kontrola
dodatnia nie przeszła**: brak X nie dowodzi braku globalnej rejestracji pada.
Własny uchwyt zamknięto z wynikiem 0. Nie zmieniano przywilejów ani fokusu.

Stan ACL, uwierzytelnienia/bondingu, HID Control/Interrupt oraz globalnego
scePad pozostaje nieustalony. Nie zrównujemy UI z kanałami HID ani braku
callbacków z brakiem raportów. Moduł Bluetooth zachowano po zakończeniu
własnego watchera. Nie restartować PS4 dla ponowienia tego eksperymentu.
Szczegóły, hashe, instrukcja i następny audyt:
[EXISTING_BT_SESSION.md](research/EXISTING_BT_SESSION.md).

Po osobnej zgodzie wykonano lokalny audyt HidControl: RX/R po 16384 B,
RW pominięto, moduł unload=0, exit=0. Kopie pobrano/sprawdzono, oba pliki
segmentów usunięto z konsoli. Nie wywołano funkcji HID Control ani nowych
prywatnych ioctl. Wrapper /dev/hid nie określa rozmiaru wskazywanego wyniku
GetDeviceInfo, więc nie wykonano zapytania i kanały L2CAP pozostają UNKNOWN.
[Szczegóły](research/HID_CONTROL_ABI_EXPERIMENT.md).

Weryfikacja raportu Deep Research wykazała niezgodne numery komend ACTIVE/
DETACH (raport 144/145, kod Usbd 12.52 124/125). Nie wykonywano komend raportu.
Przygotowano nową sondę własności USB bez detach/transferów; **nie uruchomiono
jej na sprzęcie**, gdy trwa istniejąca sesja BT bez kabla. Nie potwierdzono
uhid jako przyczyny wcześniejszych błędów. Tabela twierdzeń i eksperymenty:
[DEEP_RESEARCH_REVIEW.md](research/DEEP_RESEARCH_REVIEW.md).

## Wznowienie Bluetooth — 2026-10-30

Relacja właściciela: po wyłączeniu i ponownym włączeniu PS4 DualSense nie
połączył się sam. Aby połączyć ponownie, właściciel użył „Zapomnij urządzenie”
i ponownie sparował kontroler; obecnie zgłasza aktywne połączenie Bluetooth.
Nie ma informacji o próbie reconnect bez usunięcia wpisu ani o jej wyniku.
Nie ustalono stanu kluczy, uwierzytelnienia, kanałów HID lub raportów.
Nie przypisujemy braku reconnect do konkretnej przyczyny.

Jest to **nowa sesja po restarcie**, więc wcześniejsze zera callbacków/Pad
pozostają wynikami historycznymi, nie pomiarem bieżącego połączenia.
Nie usuwaliśmy wpisu i nie zlecaliśmy kolejnej zmiany parowania.

Pasywna próba połączenia z dotychczasowym klog:3232 zakończyła się timeout
(8 s), bez logu. Nie wysłano payloadu i nie uruchomiono sondy USB.
Przygotowano kod audytu jednego natywnego konsumenta oraz lokalne testy;
nowy odczyt pliku systemowego wymaga odrębnej zgody, nadal oczekuje.
[Zakres i kryteria](research/NATIVE_CONSUMER_EXPERIMENT.md).
