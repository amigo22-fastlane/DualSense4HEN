# Lokalny audyt libSceUsbd — wykonany po odrębnej zgodzie

Powód: na 12.52 nowe testy wykazują VID/PID, konfigurację i działający odczyt
289-bajtowego deskryptora raportów HID. Odczyty interrupt 128 B i 64 B oraz
jawny wybór alt 0 zwracają 0x802400ff. GET_REPORT(input,id=1) zwraca
0x80240004. Nie ustalono jeszcze przyczyny; brak danych nie dowodzi konieczności
patcha lub podniesienia uprawnień. Publiczne nagłówki deklarują ABI, ale nie
wyjaśniają tych ścieżek backendu na 12.52.

Przygotowano `build/ps4/dualsense4hen-usbd-abi-probe.elf`, budowany przez
`tools/build-ps4.ps1 -IncludeUsbdAbiDump`. Nie jest w standardowej dystrybucji.
Nie należy uruchamiać go w ramach zgody na zwykły test HID lub wcześniejszy
odczyt libSceBluetoothHid; wymaga zgody właściciela na **libSceUsbd**.

Zakres po zatwierdzeniu:

1. Sprawdzenie firmware 12.52, załadowanie/pożyczenie libSceUsbd w procesie
   payloadu. Bez wywoływania Init, ClaimInterface lub transferów USB.
2. Pobranie adresów i rozmiarów przez istniejące sceKernelGetModuleInfo.
   Odczyt wyłącznie segmentów RX i R, do 4 MiB każdy, najwyżej czterech.
   Segmenty RW są pomijane; brak dostępu do pamięci jądra i innych bibliotek.
3. Tymczasowy zapis `/data/dualsense4hen-usbd-segN.bin`, pobranie przez FTP
   do ignorowanego `build/hardware/`, sprawdzenie rozmiarów i SHA-256.
4. Lokalny audyt przepływu InterruptTransfer/ControlTransfer, tłumaczenia
   statusów, otwierania endpointu, obsługi zdarzeń i ograniczeń backendu.
   Wyniki nie są testem działania kontrolera.
5. Usunięcie wyłącznie utworzonych plików segmentów z konsoli po potwierdzonym
   pobraniu. Do Git trafiają własny kod sondy i wnioski; **żadne binaria Sony,
   ich fragmenty, deasemblacja ani pełne logi systemowe**.

Nie modyfikuje kodu biblioteki ani jądra, authid, konfiguracji parowania,
firmware, AutoRun ani plugins.ini. Własne pliki na /data są jedynym trwałym
skutkiem przed ich usunięciem; bibliotekę załadowaną przez sondę zwalnia jej
zwykłym cyklem modułu. Istniejącego modułu nie odładowuje.

## Wynik zatwierdzonego wykonania

Właściciel udzielił osobnej zgody na libSceUsbd. Sonda o SHA-256
35871d98c4a844756474e7ee1823fe4d30080fd52382cb271f3338d64c39f822
uruchomiła się na jego 12.52, zapisała RX i R, pominęła RW i zakończyła się
exit=0. Pobrano i sprawdzono kopie, po czym usunięto dokładnie dwa utworzone
pliki z PS4. Binaria i deasemblacja są tylko w ignorowanym build/hardware/.

Profil biblioteki:

- RX: 65536 B, SHA-256 dc8aa9f70baa6f6e32d1ea938cdd51761f6601aa4c7e63ff11f6509b848b5dff.
- R: 16384 B; kopia załadowanej biblioteki zawiera relokowane wskaźniki,
  więc nie używamy jej hasha jako uniwersalnego profilu.
- Publiczne ABI InterruptTransfer i ControlTransfer zgadza się z przepływem
  argumentów w tej bibliotece. Parametry control są ośmioma argumentami,
  w tym długość 16-bitowa i timeout 32-bitowy; nie zmieniamy sygnatur na ślepo.

Własne ustalenia z przepływu kodu (bez publikowania bajtów Sony):

1. Wrapper statusów tłumaczy libusb -7 na 0x80240007 i -99 na 0x802400ff.
   Inne ujemne wartości również mogą trafić do ogólnego kodu. Poprawiono
   sondę: natywny timeout jest zliczany i pętla pozostaje ograniczona.
2. ClaimInterface jedynie ustawia bit interfejsu w strukturze pod mutexem.
   Nie wywołuje w tej ścieżce operacji sterownika. Dlatego claim=0 nie dowodzi
   przejęcia endpointu lub zgody jądra na transfer.
3. InterruptTransfer uruchamia wewnętrzną obsługę zdarzeń samodzielnie.
   Nie dodano drugiej pętli HandleEvents na podstawie samej hipotezy o jej braku.
4. Wewnętrzna ścieżka control rozróżnia niektóre stany, lecz inne błędy
   backendu tłumaczy na libusb -4, a wrapper na 0x80240004. Ten kod po
   GET_REPORT nie jest samodzielnym dowodem odłączenia urządzenia.
5. Backend wyboru alt używa własnych ioctl i sprowadza niektóre ich błędy do
   ogólnego -99. Nie wywołujemy tych ioctl samodzielnie i nie przenosimy do Orbis
   stałych FreeBSD/Linux. Odczyt errno po całym API nie jest śledzeniem każdego
   ioctl i może pokazywać błąd późniejszego poll/write zamiast pierwotnej przyczyny.

Rewizja 5 rozwiązuje istniejący eksport libkernel __error według deklaracji
int *__error(void) z nagłówków SDK i nazwy eksportu PS4LibDoc, bez offsetów TLS.
Zeruje własny slot errno przed wywołaniem i kopiuje wynik przed logowaniem.
Na PS4: descriptor=289/errno=0, GET_REPORT=0x80240004/errno=5,
interrupt=0x802400ff/errno=35, alt0=0x802400ff/errno=5.
W SDK errno 5 to EIO, 35 to EAGAIN/EWOULDBLOCK. To etykiety nagłówka użyte
pomocniczo, nie dowód konkretnej przyczyny w sterowniku tej konsoli.

## Następny rozstrzygający eksperyment

Porównać tę samą enumerację i interrupt IN w osobnej aplikacji homebrew
z PayLoader/ScePartyDaemon, z niezmienionym kablem i endpointem oraz identycznym
logiem status/errno. Pozwoli to oddzielić kontekst procesu od problemu
backendu lub własności HID. Zbudowano już rzeczywisty PKG OpenOrbis i właściciel
potwierdził instalację poprawionej paczki. Wersja 1.00 zakończyła się SIGSYS
w _exit, a 1.01 nie dostarczyła logu USB mimo startu i zakończenia bez zgłoszonego
crasha. Wersja 1.02 poprawiła wrappery i kanał logu: Init=0, count=0, exit=-22.
Zaraz potem PayLoader na tym samym podłączeniu ponownie wykrył kontroler
i deskryptor HID, lecz nie wejście. Wersja 1.03 potwierdziła open O_RDONLY
/dev/usbctl i close z errno=0 także w aplikacji; lista nadal była pusta,
errno=1 po API. PayLoader otwiera ten węzeł i widzi urządzenie. To wyklucza
sam brak dostępu open jako wystarczające wyjaśnienie pustej listy. Szczegóły:
USB_APP.md i HARDWARE_RESULTS.md. Nadal nie ustalono konkretnego miejsca odmowy.
Nie deklarujemy, że samo podniesienie uprawnień lub patch rozwiąże problem.

Jeśli potrzebne będzie śledzenie konkretnego ioctl, trzeba najpierw przygotować
osobno zweryfikowane ABI i pasywny mechanizm pomiaru, z kontrolą własności
oraz zakończenia. Nie wdrożono hooków biblioteki, zmian jądra, resetów,
DetachKernelDriver, SetConfiguration ani arbitralnych komend HID OUT.

## Korekta ABI sprzątania po audycie

Lokalny przepływ kodu potwierdził, że GetDeviceList zwraca wynik ze znakiem
rozszerzony do 64 bitów, Exit jest wywołaniem bez wyniku statusowego, a
FreeDeviceList przekazuje dwa argumenty i rzeczywiście używa drugiego do
warunkowego oddawania referencji urządzeń. Nasz wcześniejszy prototyp
korzystał z niepełnej deklaracji OpenOrbis z jednym argumentem. Rewizja 6
stosuje jawne FreeDeviceList(list, 1), 64-bitowy wynik listy i void Exit.
Zwalnianie konfiguracji ma void zgodnie z niezależnymi deklaracjami ps4sdk.
Nie traktuje się rejestru wyniku z funkcji zwalniających jako statusu.

Nie jest to próba naprawy transferu przez zmianę losowych sygnatur: drugie
pole FreeDeviceList ma potwierdzone użycie, a ta funkcja była wywoływana dopiero
po nieudanym odbiorze. Testy atrap wymagają teraz unref_devices=1. Końcowy ELF
rev 6 został uruchomiony na PS4: deskryptor 289 B nadal działa, wejście nie.

## Dlaczego backend może zwrócić pustą listę bez błędu API

Dalsza analiza tej samej, wcześniej zatwierdzonej kopii RX wykazała, że
enumeracja nie kończy się na otwarciu /dev/usbctl. Przetwarza nazwy węzłów
ugen, otwiera poszczególne `/dev/ugen<bus>.<address>` w trybie O_RDWR i odpytuje
ich metadane. Przy nieudanym otwarciu lub zapytaniu pomija urządzenie,
zwalnia obiekt tymczasowy i kontynuuje skan. Końcowa ścieżka może zwrócić
sukces backendu, mimo że nie dodano żadnego urządzenia do listy.

To wyjaśnia mechanizm, przez który Init=0 i count=0 mogą ukryć odmowę niższej
warstwy. Nie dowodzi, który dokładnie open lub ioctl został odrzucony w 1.03;
ostatnie errno=1 jest tylko obserwacją po API. Odczyt /dev/usbctl nie dowodzi
możliwości otwarcia węzła konkretnego urządzenia ani dostępu do endpointu HID.

Następny pomiar powinien najpierw ustalić bus/address przez rzeczywiste
eksporty sceUsbd na wyliczonym DualSense w PayLoaderze, po odrębnej weryfikacji
ich ABI. Dopiero na tej podstawie porównać otwarcie dokładnie tego samego
węzła z aplikacji, bez transferów, zmian praw i poświadczeń. Bez potwierdzonej
ścieżki nie skanować losowych węzłów i nie odtwarzać prywatnych ioctl na ślepo.
Nie wdrożono jeszcze tej sondy; jej zakres różni się od wykonanego O_RDONLY
na /dev/usbctl. Nie ma dowodu konieczności patcha jądra.
