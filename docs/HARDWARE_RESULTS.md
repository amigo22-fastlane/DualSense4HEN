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
| Nowy kabel, ponowny test PS4 | **Oczekuje na wykonanie i potwierdzenie** | Przygotowano rewizję 2 ELF i procedurę `USB_RETEST.md`; brak nowego logu sprzętowego. |

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

**Nie przetestowano:** bezpośredniego parowania i reconnect, raportów BT/USB,
wirtualnego DS4 na sprzęcie, PS/Options/Create w menu i grach, touchpada w
grach, rest mode ani pełnej kompatybilności. Nie deklarujemy ich działania.

## Korekta interpretacji USB i nowy ELF

Właściciel zidentyfikował poprzedni kabel jako niepozwalający wykryć kontrolera
na komputerze, a inny kabel jako poprawnie przesyłający dane. Poprzednia pusta
lista pozostaje autentycznym wynikiem API, ale nie izoluje wpływu kabla od
uprawnień, backendu lub kontekstu procesu. Najpierw należy powtórzyć pomiar.

Stary ELF można uruchomić ponownie; rewizja 2 poprawia obserwowalność:
loguje wszystkie VID/PID, konfigurację celu i próbki przez całe okno, następnie
mapuje odczytany stan do DS4 wyłącznie w podglądzie. Nie wywołuje VDA.
Hostowe testy nowych ścieżek nie stanowią pomiaru na konsoli.
Szczegóły i wymagane logi: [USB_RETEST.md](USB_RETEST.md).
