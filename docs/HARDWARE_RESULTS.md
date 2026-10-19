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
