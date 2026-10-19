# R0: alternatywny transport Bluetooth

Eksperyment rozstrzyga warunek architektur L2CAP/HCI przed portowaniem stosu.
Nie korzysta ze sceUsbd, BluetoothHid, MBus lub VDA i nie powtarza odczytu 0x84.

## Zakres i warunki wnioskowania

ELF sprawdza firmware 12.52, rozwiązuje istniejące socket/__error libkernel
i wykonuje dokładnie trzy kontrakty z odziedziczonych nagłówków FreeBSD:

| Próba | domain | type | protocol |
|---|---:|---:|---:|
| Kontrola IPv4 UDP | 2 | 2 | 17 |
| Raw HCI | 36 | 3 | 134 |
| L2CAP SEQPACKET | 36 | 5 | 135 |

Pochodzenie: sys/socket.h, netinet/in.h i ng_btsocket.h w payload SDK v0.9,
porównane z FreeBSD. Static asserts identyfikują kontrakt, nie dostępność
na Orbis. To trzy jawne próby, bez skanowania rodzin lub protokołów.

Każdy poprawny fd, także 0, jest natychmiast zamykany. Błąd kontroli IPv4
lub close przerywa dalsze próby. Snapshot errno poprzedza log i close;
wartość wyjściowa errno procesu jest przywracana. Brak eksportu oznacza
UNTESTED, bez zastępowania go zgadywanym API.

Nie ma bind/connect/listen/recv/send/ioctl, HCI Reset, filtrów radia,
rejestracji urządzenia, odczytu kluczy ani przejęcia kontrolera. Nie zmienia
pamięci bibliotek, poświadczeń, jądra lub ustawień. Nie ładuje modułów USB/BT.

## Pomiar rzeczywistej PS4

PS4 Slim właściciela, 12.520.001 (0x12520001), zgłoszony GoldHEN v2.4b18.12.
Wysłanie: **2026-10-19 21:38:06 UTC**, pobranie nowego przyrostu logu:
21:38:11 UTC. Nie wykonywano próby parowania.

| Próba | Socket | Errno | Cleanup |
|---|---:|---:|---|
| IPv4 UDP | fd=25 | 0 | close=0, errno=0 |
| HCI | -1 | **43** | Nie powstał deskryptor |
| L2CAP | -1 | **43** | Nie powstał deskryptor |

Nagłówki celu określają 43 jako EPROTONOSUPPORT. `native-bt-gate complete=0`
i `probe exit=0` oznaczają **zakończenie pomiaru**, nie działający Bluetooth.
Log/JSON są w ignorowanym build/hardware/native-bt-gate-01.*. Pełne logi
systemowe i moduły Sony nie są dystrybuowane.

Odrzucono użycie dokładnie tych kontraktów w badanym procesie i konfiguracji.
Nie ustalono, czy istnieje inne API Orbis, osobny proces z inną polityką lub
wewnętrzny interfejs HCI. To nie dowód fizycznej niezgodności radia z DualSense.

Inne wyniki należy rozumieć następująco:

- Utworzony socket: kandydat do dalszego testu przyłączenia i współdzielenia
  radia, nie dowód danych HCI, parowania lub raportów.
- EPERM/EACCES: odmowa uprawnień, brak podstaw do orzeczenia braku implementacji.
- Brak kontroli IPv4/eksportu: pomiar nierozstrzygający dla BT.
- Inny błąd: dokładna wartość pozostaje nierozpoznana.

## ELF, wykonanie i wycofanie

Plik: `dist/research/dualsense4hen-native-bt-gate.elf`.
SHA-256: `3a359fd32781a18f844cc75a3688d7dbafb1c7a03db15c15e826a970694c4c0a`.
To payload PayLoadera, nie eboot. Nie wymaga X, pairing ani odłączania DS4.

```powershell
./tools/build-research.ps1
python tools/check-elf.py build/research/dualsense4hen-native-bt-gate.elf
python tools/console-probe.py --host ADRES_PS4 --elf dist/research/dualsense4hen-native-bt-gate.elf --output build/hardware/native-bt-gate.log --wait 5
```

Używa Zig 0.13.0 i nagłówków payload SDK z BUILD.md, bez patchującego CRT.
Walidator ELF potwierdził brak importów i W+X. Nie uruchamiać równolegle
innego payloadu tego projektu; respektuje flock i marker stop. Nie dodawać
do AutoRun. Po powrocie, według logu, nie pozostaje otwarty socket pomiaru.
Wycofanie: usunięcie własnego ELF oraz opcjonalnie logu/lock, kiedy żaden test
nie działa. Nie ma ustawienia systemowego lub bondingu do usuwania.

## Niezależny prototyp ACL/HIDP

`src/acl_input.c` przyjmuje HCI ACL bez bajtu typu H4. Wymaga rzeczywistego
handle oraz uzgodnionego lokalnego CID kanału interrupt. PSM 0x13 nie jest CID.
Składa ograniczony pakiet L2CAP **Basic mode**, sprawdza HIDP DATA/INPUT 0xA1
i przekazuje raport do niezależnego parsera BT. Błąd nie zmienia wyjścia.

Nie implementuje discovery, SSP, szyfrowania, SDP, sygnalizacji, ERTM ani
backendu PS4. Nie jest pełnym stosem; nie podłączono go do radia. Może obsłużyć
inny później zweryfikowany transport ACL. Klient działający nad L2CAP nie
potrzebowałby składania ACL. Skrypt cross-kompiluje oba moduły dla PS4.

**PASS 21289** kontroli hosta: każda granica fragmentacji, bajtowe kontynuacje,
obcy handle/CID, restart ramki, przepełnienie, zła długość/PB/CRC, PSM/CID,
10 tys. uszkodzonych wejść i cykl socketów. Fixtures są syntetyczne,
nie pochodzą z PS4. Wynik mocków nie zmienia wyniku sprzętowego transportu.
