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
2. W nowej sesji observera obserwować również zdarzenia cyklu połączenia,
   zachowując callback po odrzuconym filtrze, o ile audyt potwierdzi, że taka
   obserwacja jest poprawnym użyciem API. Zestawić log z normalną próbą
   Create+PS i komunikatem systemowej listy urządzeń Bluetooth. Nie wdrożono
   jeszcze tego wariantu; obecny observer zatrzymuje się po odmowie.
3. Rozdzielić dostęp HID od bond/discovery. Sprawdzić wywołania `/dev/bt`
   przez systemowy proces obsługujący ekran Bluetooth; `libSceBluetoothHid`
   nie dostarcza potwierdzonego publicznego API parowania. Publiczna lista
   `libSceHidControl` zawiera connect/info, lecz bez wykazanego ABI bonding.
4. **USB z nowym kablem wykonano:** count=1, DualSense 054c:0ce6, deskryptor
   HID 289 B odczytany; nadal zero wejścia, interrupt=0x802400ff. Poprzedni
   pusty wynik nie opisuje obecnej konfiguracji. Lokalny audyt sceUsbd ujawnił
   translację statusów i brakujące parametry sprzątania; poprawiono kod.
   Następnie porównać ten sam odbiór w samodzielnym kontekście aplikacji
   OpenOrbis i procesie PayLoadera. Status/errno po API nie zastępuje śledzenia
   konkretnego błędu backendu. Szczegóły: `USBD_ABI_EXPERIMENT.md`.
5. Dopiero po odebraniu i walidacji realnych raportów włączyć VDA z kontrolą
   własności nowego urządzenia, korelacją klog i odtworzeniem uprawnień.
   Następnie testować przyciski w menu i grze, odłączenie, reconnect i rest mode.

Nie implementujemy niezweryfikowanego patcha allowlisty ani offsetów jądra.
Jeśli audyt wykaże konieczność takiej zmiany, najpierw trzeba opisać dokładny
warunek, oryginalne bajty, zgodność konkretnej kompilacji firmware, skutki dla
DS4/klawiatur i sposób atomowego rollbacku. Samo istnienie GoldHEN tego nie
rozstrzyga. Obecny kod nie zawiera takiej modyfikacji.
