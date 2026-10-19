# Następny eksperyment po 0x81540002

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
   z normalną próbą Create+PS i komunikatem systemowej listy Bluetooth;
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
