# Natywny konsument GetDeviceInfo — zakres następnego kroku

Status 2026-10-30: **przygotowany, oczekuje osobnej zgody na odczyt pliku**.
Gałąź: `codex/architecture-research-1252`. Żadnego nowego pliku ani pamięci
systemowej konsoli nie odczytano w ramach przygotowania tego eksperymentu.
USB pozostaje odłożone; `dualsense4hen-usb-owner.elf` nie jest uruchamiany.

Wznowienie 2026-10-30: właściciel zgłosił wyłączenie/włączenie konsoli
i brak automatycznego ponownego połączenia. Usunął wpis „Zapomnij urządzenie”
i ponownie sparował DualSense; obecnie zgłasza połączenie Bluetooth.
To nowa sesja, nie kontynuacja sesji pomiarów z 20 października.
Nie ustalono przyczyny braku reconnect: sama ta obserwacja nie dowodzi
braku zapisania kluczy, błędu uwierzytelnienia ani odrzucania raportów HID.
Nie zlecamy kolejnego usuwania wpisu, parowania lub restartu.

Próba pasywnego pobrania klog z dotychczasowego 192.168.88.7:3232
zakończyła się timeout połączenia (8 s), bez otrzymania logu i bez payloadu.
Nie wiadomo, czy przyczyną jest adres, serwer czy dostępność sieci.

## Nowy dowód z dostępnych materiałów

Przeszukano 546 publicznych opisów modułów PS4LibDoc, przypiętych do
`a71315e7f36e312ae71e9e3a92982e9ffbfc725f` (gałąź 9.00).
Jedynym opisanym konsumentem wskazanych Get* jest
`system/sys/SceSysCore.elf.json`: importuje GetDeviceInfo z BluetoothHid oraz
GetDeviceInfo, GetDeviceName i GetBatteryState z HidControl.
Wynik odtwarzalny: [native-consumer-candidates.json](native-consumer-candidates.json).
To **wskazówka dla 12.52**, nie potwierdzenie importów lub ABI na 12.52.
Nie mylić procesu `SceSysCore.elf` z biblioteką `libSceSysCore.sprx`.

[Przypięty opis źródłowy](https://github.com/idc/ps4libdoc/blob/a71315e7f36e312ae71e9e3a92982e9ffbfc725f/system/sys/SceSysCore.elf.json).

Zatwierdzona wcześniej kopia HidControl potwierdza /dev/hid, żądanie
0x80104801, 32-bitowy argument i wskaźnik w 16-bajtowym opakowaniu.
**Nie potwierdza rozmiaru wskazywanego obiektu ani semantyki selektora.**
Wyniki: [HID_CONTROL_ABI_EXPERIMENT.md](HID_CONTROL_ABI_EXPERIMENT.md).
Nie wywołujemy GetDeviceInfo z większym, zgadywanym buforem.

Sprawdzono też rzeczywisty GoldHEN Plugin SDK: `proc_rw` komendy
PROCESS_RW nie zawiera PID. Nie traktujemy go jako potwierdzonego interfejsu
odczytu obcego procesu. W tym eksperymencie nie używamy go w ogóle.
[Nagłówek SDK](https://github.com/GoldHEN/GoldHEN_Plugins_SDK/blob/bcea3c7ef01dac6d7f9f49ebf9e90fe66d86f5f7/include/GoldHEN.h).

## Dlaczego ten eksperyment

Celem jest znalezienie rzeczywistego miejsca wywołania GetDeviceInfo przez
natywnego konsumenta na 12.52, zamiast powtarzania observera, który nie
otrzymał zdarzeń istniejącej sesji. Najpierw sprawdzamy zwykły plik na dysku,
bez ingerencji w działający SceSysCore. Sam transfer pliku nie mierzy ruchu BT.

| Pytanie | Obecny dowód | Co pozostaje do wykazania |
|---|---|---|
| Czy UI utrzymuje połączenie bez USB? | Potwierdzone przez właściciela | Szczegóły transportu nie wynikają z UI |
| Czy istnieje aktywna sesja HID? | Nieustalone | Udokumentowany stan urządzenia z konkretnego kontraktu |
| Czy otwarto kanały L2CAP PSM 0x11/0x13? | Nieustalone | Informacja o kanałach lub ślad ich zestawienia, nie sama nazwa „Kontroler” |
| Czy system otrzymuje raporty? | Nieustalone; zero callbacków nie rozstrzyga | Licznik/ślad wejścia przypisany do DualSense i reakcja na X |
| Czy dane zatrzymują się przed scePad? | Hipoteza | Najpierw potwierdzony odbiór; kontrolne L2 DS4 poprzedniej sondy również nie działało |

Pełne ABI może umożliwić bezpieczne pytanie o stan już istniejącego urządzenia.
Nie zakładamy, że GetDeviceInfo ujawnia CID albo raporty — pokaże to dopiero
analiza pól i ich pochodzenia. Zapytanie o stan nie zastąpi dowodu odbioru HID.

## Dokładny zakres wymagający zgody

1. Wyłącznie standardowy FTP konsoli 192.168.88.7:2121: logowanie anonymous,
   tryb binarny, `SIZE` i `RETR` dokładnie **`/system/sys/SceSysCore.elf`**.
   Bez listowania katalogów, szukania innych plików i uruchamiania payloadu.
2. Maksymalnie **16 MiB** całego pliku na dysku. Ten zakres obejmuje także
   statyczne dane pliku, nie tylko RX/R. Nie obejmuje żywej pamięci RW,
   pamięci obcego procesu ani jądra. Limit czasu pojedynczej operacji sieciowej: 8 s.
3. Zapis wyłącznie lokalny, nowy i bez nadpisywania:
   `build/hardware/native-consumer/SceSysCore.file` oraz `.file.json`
   z rozmiarem, SHA256, czasem UTC i klasyfikacją formatu. Katalog jest ignorowany
   przez Git. Kod Sony, deasemblacja i dane pliku nie trafią na GitHub.
4. Jeśli SIZE jest niedostępne, plik przekracza limit lub FTP odmawia dostępu:
   **stop**, bez zmiany praw, AuthID i metody dostępu.
5. Jeśli plik jest SELF lub innym nieobsługiwanym kontenerem: **stop**.
   Rozszerzenie `.elf` nie dowodzi odszyfrowanego ELF. Narzędzie nie szuka
   nagłówka ELF wewnątrz kontenera i nie odszyfrowuje go, nie mapuje przez
   specjalny loader ani nie odczytuje procesu w zastępstwie.
6. Jeżeli będzie zwykłym ELF64 x86_64, analiza lokalna obejmie jego importy,
   odwołania do wskazanych Get*, alokacje i użycie buforów. Nie uruchamiamy pliku.
   Każdy dodatkowy plik lub odczyt pamięci wymaga nowego zakresu i zgody.

Brak STOR/DELE/CHMOD, ioctl, detach, patchy, zmian parowania, konfiguracji,
uprawnień lub zatrzymywania procesu. Nie są potrzebne naciśnięcia przycisków.
Nie trzeba nic instalować na PS4; istniejące połączenie ma pozostać aktywne.

## Gotowy kod i warunki dalszego zapytania

Narzędzie: [tools/native_consumer.py](../../tools/native_consumer.py).
Przed zgodą wolno używać wyłącznie lokalnych podkomend `inventory` i `inspect`.
Poniższy odczyt sieciowy zostanie wykonany **dopiero po osobnej zgodzie**:

```powershell
New-Item -ItemType Directory -Force build/hardware/native-consumer | Out-Null
python tools/native_consumer.py fetch 192.168.88.7 build/hardware/native-consumer/SceSysCore.file
```

Warunki dopuszczenia przyszłej sondy GetDeviceInfo: potwierdzone importy i
wywołania 12.52, pochodzenie i zakres selektora, sposób inicjalizacji oraz
wielkość alokacji wyniku, wersja/układ i użycie pól, zakres zapisu przez
kontrakt ioctl, obsługa błędów i własność kontekstu. Sama największa alokacja
w jednym call-site nie dowodzi maksymalnego zapisu przez sterownik.
Jeżeli tych warunków nie uda się wykazać, sonda nie zostanie uruchomiona.

Walidacja lokalna: 35 testów Python, w tym 9 nowych: filtr importów/eksportów
i BOM, poprawny i uszkodzony ELF, osadzony ELF w kontenerze, limity transferu,
niepełny transfer, odmowa nadpisania i dokładny zakres komend FTP.
Próby na atrapach nie stanowią wyniku sprzętowego ani potwierdzenia ABI.
