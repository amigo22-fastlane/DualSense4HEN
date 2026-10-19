# Weryfikacja raportu Deep Research na firmware 12.52

Raport trafnie rozdziela transport i wejście globalne, ale nie wykazuje
przyczyny błędu USB. Jego propozycja bezpośrednich ioctl zawiera **błędne
numery komend**, w tym komend mogących zmieniać stan portu/zasilania.
Nie wdrożono instrukcji detach, zapisu parowania, patcha ani zmiany uprawnień.
Potwierdzone przez właściciela istniejące połączenie Bluetooth przesuwa
priorytet na klasyfikację i dostarczenie wejścia, zamiast nowego parowania.

Przeczytano cały lokalny `raport.docx`: 345 niepustych akapitów, także tabele,
33 hiperłącza i wszystkie trzy obrazy (dwa diagramy i małą ikonę). OOXML nie
zawiera osobnych footnotes/endnotes/comments. SHA256 pliku:
`3dcaf8ca1f170abcdbb59e5a2ef69d89c6b6cbcd09b91637a4f57ac8a2c96551`.
Oznaczenia Pxxxx poniżej odnoszą się do kolejnych niepustych akapitów
word/document.xml, z zachowaniem kolejności akapitów w komórkach tabel.
Nie są numerami stron. Pełny tekst i obrazy do analizy pozostają w ignorowanym
build/report-review; oryginału nie zmieniono ani nie dołączono do commita.

## Tabela twierdzeń

„Potwierdzone” wymaga wskazania zakresu: kod źródłowy, lokalny audyt biblioteki
albo własny pomiar sprzętowy. Nie przenosimy testu PS5 lub innego firmware
na tę PS4. „Sprzeczne” obejmuje też sprzeczność ze zweryfikowanym kodem/ABI.

| Twierdzenie raportu | Ocena | Dowód i korekta |
|---|---|---|
| 054c:0ce6, HID 3, IN 0x84, 64 B, deskryptor 289 B (P0005) | **potwierdzone** | Własne logi USB z nowym kablem; to enumeracja i deskryptor, jeszcze nie raport przycisku. |
| Błąd 0x802400ff jest bezpośrednim skutkiem przypisania uhid do interfejsu 3 (P0182, P0245) | **niezweryfikowane** | Nie wykonano dotąd zapytania o właściciela/nazwę sterownika. Lokalny backend redukuje wiele błędów do -99/OTHER; ten sam kod nie rozróżnia zajęcia, praw i błędu FS. |
| Stan interfejsu można odczytać bez detach (sens propozycji P0301–0303) | **potwierdzone** w kodzie interfejsów | W RX 12.52 jest odrębna ścieżka ACTIVE, a także GET_IFACE_DRIVER. Przygotowano sondę RO. Nie jest jeszcze wynikiem odczytu właściciela na konsoli. |
| ACTIVE to _IOW('U',144,int), DETACH to 145 (P0299) | **sprzeczne z naszymi wynikami** | RX 12.52: ACTIVE 0x8004557c (124), DETACH 0x8004557d (125). FreeBSD 9.3 przypisuje 144 do SET_PORT_DISABLE, a 145 do SET_POWER_MODE; ten ostatni numer występuje też w kodzie Sony jako operacja zasilania. Nie wysłano ich. [S1][S3] |
| USB_DO_REQUEST ma numer 1 na ugen, USB_SET_IMMED 110, USB_SET_TIMEOUT 113 (P0299) | **sprzeczne z naszymi wynikami** | Kod Sony używa DO_REQUEST=0xc018556f, czyli numer 111/struktura 24 B. FreeBSD numer 1 to USB_REQUEST kontrolera, 110 to GET_STRING_DESC, 113 to SET_RX_SHORT_XFER; generyczne HID SET_IMMED ma 22. Tych przestrzeni nie wolno mieszać. [S1] |
| USB_FS_INIT i USB_FS_OPEN da się przepisać z FreeBSD bez dalszego audytu (P0193, P0305) | **niezweryfikowane** | Numery i użyte pola częściowo zgodne z RX Sony: INIT=0x801055c3, OPEN=0xc01055c5. Nie zweryfikowano całej implementacji jądra, uprawnień i cyklu FS. Nasza sonda nie wykonuje tych komend. |
| ep_index=0x84 oznacza otwierany endpoint USB FS (P0305) | **sprzeczne z naszymi wynikami** | ep_index to slot tablicy transferów, ep_no to adres endpointu. Backend Sony zapisuje je w osobnych polach. W FreeBSD FS_INIT dopuszcza najwyżej 127 slotów, więc 0x84 nie jest prawidłowym slotem takiej tablicy. [S1][S2] |
| Ominięcie sceUsbd przez ugen samo zapewni działające raporty (P0310) | **niezweryfikowane** | sceUsbd już używa tego backendu. Własny klient może ujawnić dokładny błąd ioctl, ale nie usuwa polityki lub własności urządzenia. Ścieżki endpointów ugenX.Y.4 nie potwierdzono na tej PS4. |
| Bezpośrednie zapytania do jednego ugen poprawią rozdzielczość diagnostyki | **prawdopodobne** | Pozwolą zmierzyć konkretny open i ioctl wraz z errno zamiast ostatniego errno po całym API. Przygotowany kod ogranicza się do RO oraz trzech sprawdzonych zapytań. |
| 0x802400ff oznacza wyłącznie brak otwartego potoku USB FS (P0180–0182) | **sprzeczne z naszymi wynikami** | Audyt wrappera wykazał ogólną translację -99 i innych nieobsłużonych ujemnych wartości do FF; nie jest to unikalna diagnoza. |
| 0x80240004 po GET_REPORT dowodzi odłączenia/ENXIO errno=6 (P0183) | **sprzeczne z naszymi wynikami** jako diagnoza tej próby | Własny odczyt po API dał errno=5, a urządzenie i deskryptor nadal działały w innych próbach. Backend potrafi zamaskować przyczynę w -4. Nie mamy śladu źródłowego ioctl potwierdzającego ENXIO lub STALL. |
| -99 jest tym samym co jednobajtowe -1 i wystarczy nałożenie maski (P0177, P0183) | **sprzeczne z naszymi wynikami** | -99 to 0x9d w dolnym bajcie, nie 0xff; Sony używa jawnej tabeli i ścieżki domyślnej. Nie odtwarzać translacji przez proste OR. |
| Pusta lista w aplikacji wynika dokładnie z chroot/AuthID/caps (P0184, P0246) | **niezweryfikowane** | W obu kontekstach działa open RO /dev/usbctl; aplikacja count=0, PayLoader count=1 przy tym samym podłączeniu. Biblioteka może pomijać błędy otwarcia/metadanych ugen. Nie ustalono konkretnego warunku odmowy. Nie zmieniono poświadczeń. |
| 0x81540002 ma udowodnioną nazwę INVALID_PARAM/INVALID_DEVICE (P0186) | **niezweryfikowane** | Własny audyt: gałąź translacji 0x802f3701 → 0x81540002. Brak potwierdzonej nazwy symbolicznej i warunku w sterowniku. [S7] |
| RegisterDevice oczekuje struktury zamiast dwóch skalarów (P0188) | **sprzeczne z naszymi wynikami** | Kod 12.52 używa dwóch wartości 16-bitowych i pakuje je w 4 B. Znaczenie/polityka pary po stronie sterownika nie są w pełni rozpoznane. Nie zgadujemy alternatywnego wskaźnika. |
| Biblioteka jest wyłącznie dla gitar/perkusji i ma statyczną allowlistę Sony (P0187–0188) | **niezweryfikowane** | Sama lista eksportów lub odmowa jednej pary nie dowodzi grup obsługiwanych urządzeń, umiejscowienia allowlisty ani konieczności patcha. |
| DualSense w ogóle nie może zostać połączony bez kabla (P0004, P0189, P0248) | **sprzeczne z naszymi wynikami** | Właściciel potwierdził zielone Połączono, typ Kontroler, podświetlenie i utrzymanie połączenia bez USB. To nie dowód kanałów HID lub pełnego bondingu. |
| PS4 na pewno nie otworzyła HID Control/Interrupt i nie zapisała Link Key (P0189) | **niezweryfikowane** | Nie mamy śladu ACL/L2CAP, stanu bezpieczeństwa ani kluczy. Zero callbacków observera nie dostarcza takiego dowodu. Nie czytamy i nie zapisujemy kluczy. |
| Usługa natywna i translator mogą zastąpić własne parowanie | **prawdopodobne** jako kierunek | Istniejąca sesja jest mocniejszym punktem startu. Wymaga potwierdzenia, gdzie system odbiera/klasyfikuje raporty; odbiór HID i globalny Pad nadal są nieustalone. |
| Klog pomaga odzyskać DeviceId po AddDevice (P0240, P0307) | **potwierdzone** w kodzie referencyjnym | Control4Free i SplashDown mają ten mechanizm. Nasz vda.h już ostrzega, że AddDevice zwraca status, nie ID. Jest to brakujący element adaptera do wykonania na sprzęcie, ale nie wyjaśnienie błędów transportu. [S4][S5] |
| Wystarczy dowolna świeża linia DEVICE_ADDED/subType:2 | **niezweryfikowane** jako dowód własności | W klog tej PS4 jest historyczne ADDED 0x50300/subType:2 i model DUALSHOCK4. Nie wykonano wtedy AddDevice naszej sondy. Taki wpis sam nie daje prawa adoptowania/usunięcia urządzenia ani nie identyfikuje DualSense. Potrzebne marker, jednoznaczna korelacja i brak konkurencyjnych zmian. |
| VDA jest „całkowicie rozwiązane”, działa na 12.52 bez zmian przywilejów (P0054, P0269–0271) | **niezweryfikowane** na 12.52; twierdzenie o przywilejach **sprzeczne z audytem kodu** | Kod Control4Free/SplashDown zmienia authid/caps, a także używa SetProcessPrivilege. SplashDown zawiera dodatkowo kasowanie zakresu 0–63 i fallback słabego ID. Tych operacji nie przenosimy. Nie mamy własnego sprzętowego potwierdzenia VDA. [S4][S5] |
| Zmiana VID/PID wystarczy pośrednio do pełnej obsługi DS4 (P0311) | **niezweryfikowane** | Inne raporty/CRC/output/cykl połączenia i autoryzacja mogą wymagać osobnej obsługi. Nie zlokalizowano takiej tabeli na 12.52. |
| Raport 0x05/0x20 to kalibracja, wystarcza do pełnego BT (P0123, P0242) | **niezweryfikowane** w tej postaci | Linux rozróżnia kalibrację 0x05/41 B i informacje firmware 0x20/64 B. Projekt nondebug opisuje aktywację pełnego trybu; nie dowodzi to stanu tej sesji. X istnieje już w podstawowym formacie, więc brak pełnego trybu nie wyjaśnia sam braku X. [S6][S8] |
| Nie trzeba rozróżniać CRC poza „ziarnem A1/A2” | **potwierdzone częściowo** jako format protokołu | Raport 0x31 ma CRC z bajtem prefiksu A1 dla wejścia/A2 dla wyjścia; parser już to sprawdza. Nie jest to źródło transportu. Implementacji Linux nie przeniesiono do Orbis. [S6] |
| nondebug ma MIT, xbox_controller_plugin GPL, ClutchBenefactor jest wskazanym działającym pakietem Python (grafika/P0242) | **sprzeczne z audytem repozytoriów** | W klonie nondebug brak licencji; README Xbox deklaruje MIT, nie GPL; wskazany klon ClutchBenefactor nie jest zweryfikowaną biblioteką PyPI. Nie utożsamiamy nazwy pakietu z tym repozytorium. Zob. AUDIT.md i przypięte rewizje. |
| Most PC/WebHID i projekty PS5 dostarczają działającego backendu Orbis BT | **sprzeczne z kodem** | Mogą dostarczyć wiedzę o formacie, ale nie dostęp do wewnętrznego radia PS4. Test PS5 13.60 nie jest testem PS4 12.52. [S9] |
| Natychmiastowe działanie/„pierwsze na świecie” i ryzyko detach „bardzo niskie” (P0262–0264, P0310) | **niezweryfikowane** | Brak testów właściciela interfejsu i ABI jądra. Wywołanie ioctl z userspace nadal uruchamia kod sterownika i może zmieniać urządzenie. Nie deklarujemy terminów ani pierwszeństwa. |

## Najmocniejsze nowe dowody

Zatwierdzony wcześniej odczyt **tej samej 12.52** pozwolił wykazać numery
komend w RX libSceUsbd, zamiast przenosić je z FreeBSD. Offline:

```
python tools/research_abi.py usbd build/hardware/usbd-seg0.bin
python tools/research_abi.py hidcontrol build/hardware/hidcontrol-seg0.bin
python tools/native_bt_evidence.py build/hardware/bt-connected-baseline-01.klog
```

Skrypt wymaga pełnego SHA256/rozmiaru modułu przed użyciem lokalnych punktów
analizy. Zwraca własne dane liczbowe, nie kod Sony; kopii nie ma w repozytorium.
Ustalono zgodność **żądania wrappera i części pól**, nie wszystkich semantyk
sterownika. W szczególności poznanie FS_OPEN nie jest zgodą na jego wykonanie.

Profil Usbd RX: SHA256 dc8aa9f70baa6f6e32d1ea938cdd51761f6601aa4c7e63ff11f6509b848b5dff,
65536 B. Sonda dodatkowo wymaga zgodności CRC(A1)=0x3efeb3ed i tego, by
dynamicznie rozwiązane GetBusNumber/GetDeviceAddress wskazywały odpowiednio
lokalnie analizowane liściowe wejścia +0x420/+0x430. Są to punkty w module
użytkownika, nie offsety jądra lub adresy patcha. Zgodność tych powiązań
dlsym będzie osobnym warunkiem pierwszej próby sprzętowej nowej sondy;
przy różnicy sonda kończy się przed Init/enumeracją. Nie wykonuje skoku
pod stały offset zamiast eksportu.

W nowym, zatwierdzonym pomiarze HidControl pobrano RX i R po 16384 B,
pominęto RW; moduł zwolniono, pliki usunięto po sprawdzeniu kopii. Funkcje
GetDeviceInfo/Name nadal tylko pakują 32-bitowy selektor i wskaźnik w 16 B
dla /dev/hid. **16 B to rozmiar opakowania, nie rozmiar bufora wynikowego.**
Nie wykonano tych funkcji. Nie uzyskaliśmy jeszcze struktury urządzenia,
listy CID ani potwierdzenia odbioru HID. Szczegóły: [audyt](HID_CONTROL_ABI_EXPERIMENT.md).

Analiza klog dodała rozróżnienie: system historycznie obsłużył urządzenie
0x50300 z etykietą DUALSHOCK4, ale bez związku potwierdzającego tożsamość
DualSense. Identyfikator użytkownika nie trafia do podsumowania. Nawet udana
kontrola natywnego DS4 w innym kontekście nie naprawia nieudanej kontroli
ReadState w PayLoaderze (347 odczytów, zero L2).

## Dwa eksperymenty o największej wartości

### Bluetooth — stan istniejącej sesji u jej natywnego konsumenta

Pytanie: czy istniejąca sesja kończy się przed HID, czy raport dociera do
systemowego konsumenta i odpada przy klasyfikacji/eksporcie do Pad?

Etap już wykonany: pasywny callback bez RegisterDevice oraz audyt RX/R
HidControl. Pierwszy nie daje zdarzeń, drugi ujawnia inny kontrakt niż /dev/bt,
ale nie rozmiar wyniku. **Nie powtarzamy callbacku i nie tworzymy parowania.**

Następna część: ustalić rzeczywisty call-site GetDeviceInfo w systemowym
konsumencie (publiczna lista importów PS4LibDoc 9.00 wskazuje
`system/sys/SceSysCore.elf`, nie bibliotekę libSceSysCore.sprx; to kandydat
do osobnej weryfikacji na 12.52), w tym pochodzenie
selektora, pełne bufory oraz pola statusu. Porównać istniejący wpis DualSense
z natywnym wpisem DS4. Dopiero po dowodzie obserwacyjnego ABI wykonać jedno
zapytanie istniejącego urządzenia. Dla kanałów wymagany będzie osobny,
rzeczywiście istniejący wynik zawierający stan/CID PSM 0x11 i 0x13 albo
ślad konfiguracji kanałów. Metadane urządzenia nie zastępują tej informacji.

Warunek sukcesu: identyfikacja urządzenia z pasującym stanem sesji i osobny
dowód kanałów/raportów. Warunek odrzucenia tej metody: wrapper tylko przekazuje
nieznane dane albo natywny interfejs nie udostępnia stanu kanałów. Wtedy
wynik pozostaje UNKNOWN; nie zgadujemy rozmiarów, indeksów ani prywatnych ioctl.
Nowy odczyt kodu innego modułu wymaga osobnego, konkretnego zakresu — zgoda na
HidControl nie obejmuje automatycznie SceSysCore ani pamięci ShellCore.

Bez dodatkowego odczytu można już analizować istniejący klog i pasywnie
korelować otwarcie szczegółów istniejącego urządzenia w UI. Daje to dowód
klasyfikacji tylko wtedy, gdy log identyfikuje to urządzenie; brak wpisów
nie rozstrzyga kanałów. Narzędzie `native_bt_evidence.py` realizuje część
analizy bez żadnych wywołań konsoli i bez automatycznej adopcji DeviceId.

### USB — właściciel interfejsu i konkretny wynik backendu bez detach

Gotowy ELF: [dualsense4hen-usb-owner.elf](../../dist/research/dualsense4hen-usb-owner.elf).
Kod: `src/usb_owner.c`, `platform/ps4/usb_owner_probe.c`.
SHA256: `2b1c221e8577db8deb311c39addae7e1994aac8f2af4ed2ea8e09dacf4c5b212`.
Budowanie: `powershell -File tools/build-research.ps1 -UsbOwner`.

1. Sprawdzić firmware i dokładny profil RX biblioteki; przy innym odmówić.
2. Init/GetDeviceList, descriptor 054c:0ce6, liściowe GetBusNumber/Address.
   Odczyty getterów zweryfikowano w lokalnym profilu; nie kopiujemy pól
   prywatnego obiektu urządzenia. Nie skanujemy /dev ani starych numerów ugen.
3. Otworzyć wyłącznie odpowiadający urządzeniu węzeł w O_RDONLY. Ponownie
   odczytać Device Descriptor **na tym fd**, by ograniczyć ryzyko ponownego
   użycia bus/address po odłączeniu. Przy innym VID/PID nie pytać o interfejs.
4. Wykonać tylko ACTIVE=0x8004557c i GET_IFACE_DRIVER=0xc0205579 dla
   interfejsu 3, logując wynik i errno osobno; zamknąć własny fd.
5. FreeDeviceList(list,1), Exit i zwolnienie własnego modułu. Bez Open libusb,
   Claim, FS_INIT/OPEN, interrupt, GET_REPORT, detach, resetu lub zasilania.

Sukces ACTIVE i nazwa uhid* potwierdzą przypisanie tego sterownika, **nie
związek przyczynowy z 0x802400ff**. Brak nazwy przy udanym zapytaniu osłabi
hipotezę uhid. ENXIO, EPERM albo inne niepowodzenie nie stanowią dowodu braku
sterownika — logujemy je bez ukrywania. Wynik może ujawnić ograniczenie RO;
nie przełączamy automatycznie na O_RDWR. Do rozróżnienia faktycznego błędu
FS_OPEN potrzebne byłoby osobne zweryfikowane śledzenie tego konkretnego
wywołania, nie odczyt ostatniego errno po InterruptTransfer.

ELF zbudowano i sprawdzono strukturalnie, kod query przeszedł testy na atrapach.
**Nie uruchomiono go jeszcze na PS4.** Bieżąca sesja ma odłączony USB;
nie podłączamy kabla ani nie zmieniamy transportu podczas tego audytu raportu.
Przyszły test USB musi być uzgodniony jako oddzielne okno sprzętowe.

Instrukcja: w przyszłym oknie podłączyć potwierdzony kabel danych do
DualSense, zachować sprawny DS4 do menu, udostępnić FTP/PayLoader i zgłosić
gotowość. ELF jest wysyłany jednorazowo przez 9090, bez instalacji PKG.
Log: /data/dualsense4hen-probe.log. Wymagane linie: firmware/profil,
VID/PID/bus/address, open+errno, identity, ACTIVE+errno, NAME+errno, close,
complete i exit. Na razie nie potrzeba naciskać X: to pomiar własności,
nie kolejny nieskuteczny transfer.

## VDA i klog

Mechanizm jest użytecznym elementem dalszej integracji, ale raport myli
potwierdzenie referencji z testem 12.52 i pomija operacje wymagające osobnej
zgody. Nasz adapter już używa 120 B, Mbus przed Pad, statusu AddDevice oraz
rosnącego timestamp/count. Brakuje sprzętowo zweryfikowanego uzyskania i
własności nowego ID oraz testu menu/gry. W tej turze **nie tworzymy VDA**,
nie ustawiamy SetProcessPrivilege, authid/caps i nie patchujemy autoryzacji.

Przy późniejszym włączeniu trzeba mieć jednego czytelnika klog, własny
marker i ograniczone okno, wykluczyć stare/lustrzane linie oraz inne ADD,
przerwać przy niejednoznaczności. Nie przenosić fallbacku „pierwszy ADD” lub
kasowania nienależących do nas uchwytów. Statyczny subtype/model to kandydat,
nie automatyczny dowód własności. Nie używać równoległego klienta klog przy
rejestracji. Ta praca jest niezależna od bezpośredniego odbioru BT.

## Źródła niezależne od raportu

- [S1: FreeBSD 9.3 usb_ioctl.h](https://github.com/freebsd/freebsd-src/blob/releng/9.3/sys/dev/usb/usb_ioctl.h), numery i struktury; nie dowód całego jądra Orbis.
- [S2: FreeBSD 9.3 usb_generic.c](https://github.com/freebsd/freebsd-src/blob/releng/9.3/sys/dev/usb/usb_generic.c), rozdzielenie zapytania ACTIVE, DETACH i FS_OPEN.
- [S3: FreeBSD 9.3 libusb20_ugen20.c](https://github.com/freebsd/freebsd-src/blob/releng/9.3/lib/libusb/libusb20_ugen20.c), backend i redukcja błędów; porównano z zatwierdzonym lokalnym RX 12.52.
- [S4: Control4Free vda.c](https://github.com/MoHadiShibli/Control4Free/blob/5a8f9e3d0e75378644e7f7b2df6176c386a24da9/src/vda.c), a także main.c i klog_line.c w tej samej rewizji; upstream HEAD sprawdzony 2026-10-20.
- [S5: SplashDown src/psbutton.c](https://github.com/seregonwar/SplashDown/blob/main/src/psbutton.c), osobne ścieżki PS4/PS5, podnoszenie poświadczeń i fallback identyfikacji. Nie wykonano jego payloadu.
- [S6: Linux hid-playstation.c](https://github.com/torvalds/linux/blob/cd4f494ba535e848e2d299707d363de84e61a518/drivers/hid/hid-playstation.c), formaty raportów/feature/CRC; kod referencyjny, nie port Orbis.
- [S7: własny audyt BluetoothHid](../BLUETOOTH_ABI_1252.md), [Usbd](../USBD_ABI_EXPERIMENT.md), [HidControl](HID_CONTROL_ABI_EXPERIMENT.md) i [sprzęt](../HARDWARE_RESULTS.md).
- [S8: nondebug README](https://github.com/nondebug/dualsense/blob/b87450eb1c4ba8d74948f13b43d4c1eaa8ffa4f5/README.md); historyczny opis USB ma 273 B, nasza sztuka 289 B — długości nie wolno uogólniać.
- [S9: PS5_DevLoop README](https://github.com/griches/PS5_DevLoop/blob/main/README.md); opis dotyczy PS5, nie testu tej konsoli PS4.

## Weryfikacja implementacji

Nowe testy własności USB: 109 sprawdzeń PASS (tożsamość na fd, tylko RO,
trzy dozwolone zapytania, errno, fd=0, zamknięcie przy błędach, brak detach).
Python: 26 testów PASS, w tym dekoder numerycznego ABI z odmową nieznanego
profilu i analiza natywnych zdarzeń bez przypisywania tożsamości/własności.
Regresje C parsera, ACL, BT, Pad i USB przeszły. Atrapy nie potwierdzają
odbioru HID na PS4. Nie zarejestrowano rzeczywistego X z DualSense.
