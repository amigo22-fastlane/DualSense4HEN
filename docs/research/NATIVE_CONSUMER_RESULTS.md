# Natywny konsument HidControl — wynik 2026-10-30

**SceSysCore rzeczywiście używa obu GetDeviceInfo w pobranym pliku z konsoli.
Odtworzono statyczne ścieżki wywołań i część kontraktu. Nie ustalono pełnego
rozmiaru wyników i nie wywołano żadnego zapytania HID. Odbiór raportów przez
PS4 pozostaje niepotwierdzony.**

## Pochodzenie i zakres

Właściciel zatwierdził wyłącznie odczyt `/system/sys/SceSysCore.elf`, do 16 MiB,
oraz analizę lokalną, bez odszyfrowywania SELF i pamięci procesu. Podał nowy
adres konsoli: 192.168.88.12. Normalny FTP udostępnił bezpośrednio ELF64
x86_64 typu 0xfe10; nie wysyłano komend odszyfrowywania ani mapowania SELF.
Nie wnioskujemy o wewnętrznej realizacji serwera FTP na podstawie jego wyniku.

- Pobranie: 2026-10-30 10:39:52 UTC (11:39:52 czasu Polski).
- Rozmiar: **940552 B**, zgodny z SIZE i liczbą odebranych bajtów.
- SHA256: `968fd8d7ac0f65d3949fed95e8d113e57ee77108bf349aad545ea639cf893b69`.
- RX w pliku: 858648 B; RW w pliku: 1000 B. Odczyt objął cały plik zgodnie
  ze zgodą, w tym statyczne dane; nie objął żywej pamięci RW.
- Firmware 12.52 jest wcześniejszym odczytem z konsoli i konfiguracją
  zgłoszoną przez właściciela. Sam transfer FTP nie mierzy wersji firmware.
- Binarium, pełna deasemblacja i pliki robocze pozostają w ignorowanym
  `build/hardware/native-consumer/`. Nie trafiły do GitHub.

Nie wykonano payloadu, nowych ioctl, odłączenia urządzenia, zmian uprawnień,
AuthID, konfiguracji, parowania, patchy lub odczytu innych plików systemowych.

## Odtwarzalny nowy dowód

Narzędzie [orbis_consumer_elf.py](../../tools/orbis_consumer_elf.py) interpretuje
tablice dynamiczne i relokacje w lokalnym pliku. Dopasowuje publiczne NID-y
do rzeczywistych importów. Profil miejsc wywołań wymaga pełnego hasha
wskazanego obrazu; przy innym pliku odmawia interpretacji profilu.
Nie emituje kodu Sony ani deasemblacji, nie łączy się z konsolą.

```powershell
python tools/orbis_consumer_elf.py build/hardware/native-consumer/SceSysCore.file research/ps4libdoc/system/sys/SceSysCore.elf.json --native-profile
```

Wszystkie poniższe adresy są **RVA w zatwierdzonym pliku**, wyłącznie do
analizy offline. Nie są offsetami do patchowania ani dostępu do procesu.

| Ustalenie | Dowód w lokalnym pliku | Granica wniosku |
|---|---|---|
| Importy HidControl GetDeviceInfo, GetDeviceName, GetBatteryState oraz BluetoothHid GetDeviceInfo są obecne | NID-y z tablicy symboli mają relokacje importów do odpowiednich slotów | To już dowód z tej konsoli, a nie tylko lista importów 9.00 |
| Istnieje statyczna ścieżka do HidControl GetDeviceInfo | 0x70824 → 0x537f0; 0x5380f → 0x5b5f0; 0x5b61f → PLT 0x91ec0 → slot 0xd5858 | Nie jest to ślad wykonania podczas bieżącego połączenia |
| Istnieje statyczna ścieżka do BluetoothHid GetDeviceInfo | 0x7181c → 0x53820; 0x5383f → 0x5b660; 0x5b68f → PLT 0x91e40 → slot 0xd5818 | Nie dowodzi, że ten wariant obsłużył DualSense |
| Pierwszy argument jest 32-bitowym devId | Oba wrappery przekazują dolne 32 bity identyfikatora do importu; natywne komunikaty błędów nazywają go devId | Nie jest to potwierdzony indeks enumeracji; nie skanujemy losowych liczb |
| HidControl wynik ma używane pola co najmniej do bajtu 0x24 | Odczyty bajtu 0, qword +8, word/dword +0x10, word +0x12, bajtów +0x16/+0x17 i flagi +0x24 w natywnych konsumentach | Dolna granica odczytywanego zakresu to 37 B; **nie sizeof struktury** |
| BluetoothHid wynik jest używany co najmniej przez 12 B | Natywny konsument kopiuje qword +0 oraz dword +8 | **Nie dowodzi sizeof=12** ani pola CID |
| GetDeviceName dostaje rzeczywiście przygotowany bufor | Natywna funkcja zeruje 0x202=514 B przed wywołaniem; potem kopiuje pole od +0x101 | To informacja o buforze nazwy, nie rozmiar GetDeviceInfo i nie zgoda na nowe zapytanie |

W jednym konsumentcie HidControl wynik znajduje się między rbp-0x70 a
canary przy rbp-0x30. Dostępna przestrzeń stosu nie określa sama dokładnego
typu ani rozmiaru zapisu przez sterownik. Nie zamieniamy jej w wymyślone
`struct SceHidDeviceInfo[64]` ani „bezpieczny większy bufor”.
Pobrany ELF nie ma tabeli sekcji (e_shoff=0, e_shnum=0); nie uzyskano z niego
definicji typów debugowych, która rozstrzygałaby sizeof. Publiczne nagłówki
OpenOrbis deklarują te Get* bez typowanego kontraktu wyniku; sprawdzone
wyszukiwania nazw funkcji i 0x80104801 nie dostarczyły takiej definicji.
To ograniczenie sprawdzonych materiałów, nie twierdzenie o nieistnieniu ABI.
Porównania pól z wartościami 1/2 i identyfikatorami sprzętu pomagają śledzić
klasyfikację; bez pełnego kontraktu nie opisujemy ich jako potwierdzonych
stanów ACL/HID. Nie zlokalizowano w tych użyciach potwierdzonych CID ani
licznika odebranych raportów. To nie dowód, że takie dane nigdzie nie istnieją.

Źródło znaczenia tagów ELF i relokacji:
[OpenOrbis dynamic_entries.py](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/0a1aaf9dd4a92695538bdeb09fb056d06dd11725/scripts/dynamic_entries.py),
[symbol_entries.py](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/0a1aaf9dd4a92695538bdeb09fb056d06dd11725/scripts/symbol_entries.py).
Nazwy importów: przypięta baza PS4LibDoc opisana w eksperymencie.

## Bieżący klog i nowa sesja

Po włączeniu serwerów pobrano pasywnie 65580 B klog przez port 3232 w oknie
8 s. SHA256: `e8413d5d4decdf79d343b67163ef33f023b957c8af39ac043648a8d88a75ad23`.
Pełny log pozostaje lokalny. Wcześniejszy timeout pod .7 wynikał z niedostępnego
wówczas punktu połączenia; nowy odczyt pod .12 zadziałał. Nie ustala to,
czy wcześniej zmienił się wyłącznie adres czy też stan serwera.

Znaleziono historyczne REMOVED dla 0x9000000000200 oraz ADDED/OWNER_CHANGED
dla 0x9000000000300, subtype 10. Log nie wiąże tych wpisów z nazwą DualSense
lub 054c:0ce6, nie daje właścicielskiego uchwytu VDA i nie potwierdza raportów.
Nie należy przyjmować dolnych 32 bitów żadnego z tych ID jako identyfikatora
DualSense do zapytania. Ślad wejścia na ekran ustawień Bluetooth też nie
potwierdza otwarcia obu kanałów HID.

Właściciel zgłosił, że po restarcie pad nie połączył się sam i dopiero po
„Zapomnij urządzenie” ponownie go sparował. Bieżąca sesja jest nowa.
Przyczyna braku automatycznego reconnect pozostaje nieustalona; nie ma
podstaw do stwierdzenia braku kluczy lub konieczności własnego parowania.

## Co blokuje pomiar stanu i jaki kolejny dowód jest potrzebny

Uzyskaliśmy native call-site i znaczenie pierwszego argumentu, ale nadal
brakuje **dokładnego zakresu zapisu wyniku ioctl oraz identyfikatora
jednoznacznie przypisanego do połączonego DualSense**. Samo powiększenie
bufora albo użycie świeżego DeviceId z klog nie rozwiązuje tych braków.
Dlatego w tym kroku nie przygotowano ELF wywołującego GetDeviceInfo.

Następny rozstrzygający audyt powinien dostarczyć definicję zapisu przez
obsługę 0x80104801 (/dev/hid) lub 0x8010a40b (/dev/bt), powiązaną z tą
kompilacją, oraz ścieżkę uzyskania identyfikatora z nazwą/adresem urządzenia.
To wymóg dowodowy, **nie upoważnienie do odczytu jądra, innego modułu,
kluczy lub pamięci procesu**. Brakuje zweryfikowanego sposobu i dokładnego
zakresu takiego dodatkowego odczytu; nie proponujemy go pod fikcyjnym
nazewnictwem modułu. Najpierw trzeba go określić na podstawie dostępnych
materiałów i przedstawić właścicielowi do osobnej zgody.

Potwierdzony stan urządzenia pozwoliłby rozdzielić sesję BT od sesji HID,
o ile semantyka pól rzeczywiście to ujawni. Potwierdzenie raportów wymaga
osobnego punktu obserwacji wejścia, identyfikacji źródła oraz korelacji z X.
Nie usunięto problemu transportu i nie deklarujemy działania menu lub gier.

Weryfikacja: 45 testów Python PASS (10 nowych testów ELF z własnymi danymi),
profil przeszedł na rzeczywistym lokalnym ELF. Brak nowych testów odbioru X,
parowania i transferów USB. Dane liczbowe: [native-consumer-results.json](native-consumer-results.json).
