# Następny eksperyment po 0x81540002

**Wykonano 2026-10-30 po osobnej zgodzie:** FTP z nowego .12 udostępnił
940552 B SceSysCore.elf jako czytelny ELF64. Potwierdzono importy, statyczne
łańcuchy wywołań i devId jako 32-bitowy argument; pełny rozmiar wyniku nadal
nieustalony. Klog pobrany, bez jednoznacznej tożsamości DualSense lub raportu X.
[NATIVE_CONSUMER_RESULTS.md](research/NATIVE_CONSUMER_RESULTS.md).
Nie uruchamiać GetDeviceInfo ze zgadywanym rozmiarem/ID; USB nadal odłożone.
Poniższy zapis oczekiwania na zgodę opisuje wcześniejszy stan prac.

**Wznowienie 2026-10-30:** po wyłączeniu/włączeniu konsoli DualSense nie
połączył się automatycznie; właściciel usunął wpis i ponownie sparował pad.
Obecna sesja BT jest nowa. Nie powtarzamy parowania ani nie przenosimy na nią
wyników starego observera. Próba pobrania klog pod poprzednim adresem/portem
zakończyła się timeout; nowy log nie został uzyskany. Zakres oczekującego na
zgodę odczytu jednego pliku pozostaje opisany poniżej.

**Bieżąca decyzja właściciela (2026-10-20): Bluetooth przed USB.**
Przygotowano [NATIVE_CONSUMER_EXPERIMENT.md](research/NATIVE_CONSUMER_EXPERIMENT.md):
zwykły odczyt jednego pliku SceSysCore.elf przez FTP, do 16 MiB, bez pamięci
procesu i bez wywołania GetDeviceInfo. Oczekuje osobnej zgody na nowy zakres.
Publiczne importy wskazują konsumenta na 9.00; trzeba zweryfikować 12.52.
USB owner ELF pozostaje gotowy, **nie uruchamiać automatycznie**.
Poniższe starsze propozycje USB i parowania nie są bieżącym zleceniem wykonania.

**Aktualizacja 2026-10-20:** właściciel potwierdził istniejące połączenie
DualSense w UI także bez USB. Pierwszy observer bez RegisterDevice wykonał
35 s bez zdarzeń. Nie usuwać urządzenia, nie parować ponownie, nie restartować
konsoli dla świeżego kontekstu. Obecny priorytet i wyniki:
[EXISTING_BT_SESSION.md](research/EXISTING_BT_SESSION.md).
Zweryfikowano nowy raport: [DEEP_RESEARCH_REVIEW.md](research/DEEP_RESEARCH_REVIEW.md).
Następny pomiar USB ma pytać o właściciela/nazwę interfejsu, bez detach,
FS_INIT/OPEN lub kolejnego transferu. Nie podłączać USB podczas bieżącej
obserwacji BT. Audyt HidControl nie wystarczył do bezpiecznego GetDeviceInfo.
Poniższa starsza propozycja Create+PS jest nieaktualna dla trwającej sesji.

Obecna blokada: `sceBluetoothHidRegisterDevice(0x054c,0x0ce6)` odrzuca próbę,
mimo poprawnych Init i RegisterCallback. Nie wiadomo jeszcze, czy przyczyną
jest semantyka argumentów, rezerwacja identyfikatorów Sony, polityka sterownika,
uprawnienia gospodarza czy inny warunek. Brak raportów nie dowodzi fizycznej
niezgodności modułów Bluetooth. Nie potwierdzono konieczności patcha jądra.

1. Wyjaśnić symboliczną semantykę błędu i argumentów RegisterDevice z
   niezależnego call-site na **tym samym firmware** albo dokumentacji ABI.
   Odtworzono typy i przekazanie dwóch 16-bitowych wartości; etykiety VID/PID
   pozostają hipotezą do potwierdzenia po stronie sterownika. Nie testować
   losowych wartości ani wildcardów bez ustalonej semantyki.
2. Zbudowano oddzielny pasywny observer bez RegisterDevice, po audycie tworzenia
   watchera przez RegisterCallback. Ma 30-sekundowe okno i te same warunki
   bezpiecznego join. Nie testowano go na konsoli. Zestawić przyszły log
   z komunikatem istniejącej sesji systemowej listy Bluetooth;
   brak zdarzeń nadal nie rozstrzygnie fizycznej zgodności modułu.
3. Rozdzielić dostęp HID od bond/discovery. Sprawdzić wywołania `/dev/bt`
   przez systemowy proces obsługujący ekran Bluetooth; `libSceBluetoothHid`
   nie dostarcza potwierdzonego publicznego API parowania. Publiczna lista
   `libSceHidControl` zawiera connect/info, lecz bez wykazanego ABI bonding.
4. **USB z nowym kablem wykonano:** count=1, DualSense 054c:0ce6, deskryptor
   HID 289 B odczytany; nadal zero wejścia, interrupt=0x802400ff. Poprzedni
   pusty wynik nie opisuje obecnej konfiguracji. Lokalny audyt sceUsbd ujawnił
   translację statusów i brakujące parametry sprzątania; poprawiono kod.
   Zbudowano i zainstalowano samodzielną aplikację OpenOrbis. Wersja 1.00
   ujawniła crash przy CRT _exit, a 1.01 wystartowała i zakończyła się bez
   zgłoszonego błędu, lecz bez logu USB. Wersja 1.02 używa deklarowanych
   wrapperów libkernel i logu: Init=0, lista pusta. PayLoader zaraz potem widział
   054c:0ce6 i deskryptor, lecz transfer był nadal odrzucony. Wersja 1.03
   potwierdziła open O_RDONLY /dev/usbctl także w aplikacji (errno=0), ale
   count=0 i errno=1 po enumeracji pozostały. W PayLoaderze count=1.
   Następnie zlokalizować w zatwierdzonej lokalnej kopii biblioteki dokładną
   ścieżkę odmowy enumeracji i otwarcia endpointu. Przed osobną sondą query
   ustalić pełne ABI i wykazać, że operacja jest tylko do odczytu. Samo errno
   po API nie wystarcza; nie wdrożono hooków ani prywatnych ioctl.
   Nie zmieniać praw urządzeń ani authid.
   Audyt już wykazał, że backend dodatkowo otwiera węzły ugen w O_RDWR i może
   pomijać nieudane urządzenia bez zwrócenia błędu listy. Najpierw ustalić
   rzeczywiste bus/address DualSense przez zweryfikowane eksporty; następnie
   porównać dostęp do tego konkretnego węzła, bez losowego skanowania i ioctl.
   Procedura: USB_APP.md. Status/errno po API nie zastępuje śledzenia
   konkretnego błędu backendu. Szczegóły: `USBD_ABI_EXPERIMENT.md`.
5. Dopiero po odebraniu i walidacji realnych raportów włączyć VDA z kontrolą
   własności nowego urządzenia, korelacją klog i odtworzeniem uprawnień.
   Następnie testować przyciski w menu i grze, odłączenie, reconnect i rest mode.

Nie implementujemy niezweryfikowanego patcha allowlisty ani offsetów jądra.
Jeśli audyt wykaże konieczność takiej zmiany, najpierw trzeba opisać dokładny
warunek, oryginalne bajty, zgodność konkretnej kompilacji firmware, skutki dla
DS4/klawiatur i sposób atomowego rollbacku. Samo istnienie GoldHEN tego nie
rozstrzyga. Obecny kod nie zawiera takiej modyfikacji.
