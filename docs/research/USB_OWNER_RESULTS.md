# USB-owner: rzeczywisty pomiar i granica wniosku

2026-10-30, PS4 Slim 12.52 / GoldHEN v2.4b18.12. Osobna zgoda właściciela:
„Tak — BT zakończone, zgoda na test USB-owner”, następnie „USB GOTOWE”.
Nie zmieniano parowania. Pomiar dotyczy USB, nie potwierdza bieżącego stanu BT.

| Operacja | Wynik rzeczywisty |
|---|---|
| ELF / profil | 51088 B, SHA256 w JSON; firmware 0x12520001, CRC RX 0x3efeb3ed |
| Init / enumeracja | 0 / jedno urządzenie, 054c:0ce6, bus 0, address 2 |
| O_RDONLY /dev/ugen0.2 | fd=27, errno=0 |
| Device Descriptor na tym fd | result=0, errno=0, 054c:0ce6 |
| ACTIVE interface 3 | raw=-1, errno=6 |
| GET_IFACE_DRIVER interface 3 | raw=-1, errno=22; nazwa nie została odczytana |
| Sprzątanie | close=0; unload własnego modułu ret=0/result=0 |
| Podsumowanie | complete=-4 / exit=-4; METADATA_QUERY_REFUSED, binding UNKNOWN |

Identyfikacja urządzenia i dostęp RO są potwierdzone. Nie jest to wynik
„brak sterownika” ani „interfejs zajęty”. Status analizatora REFUSED oznacza
nieudane query, nie ustaloną odmowę uprawnień. Pusta nazwa po błędzie nie
jest udanym odczytem pustej nazwy. Brak rzeczywistych raportów przycisków.

## Co zmienia para błędów

Nagłówek errno SDK nazywa 6 ENXIO, 22 EINVAL. Te nazwy nie wskazują jeszcze
gałęzi sterownika Orbis, która je zwróciła. W porównawczym kodzie
[FreeBSD releng/9.3 usb_generic.c](https://github.com/freebsd/freebsd-src/blob/releng/9.3/sys/dev/usb/usb_generic.c)
ACTIVE zwraca ENXIO bez interfejsu lub subdev. NAME zwraca EINVAL dla
brakującego interfejsu/idesc, albo błędnego wskaźnika/długości; obecny
interfejs bez sterownika może zwrócić sukces z pustym napisem. Zatem
para 6/22 pasuje także do braku aktywnego rekordu interfejsu 3, a nie
wyłącznie do zajęcia go przez sterownik. **To hipoteza porównawcza:** nie
posiadamy implementacji tych gałęzi jądra Orbis 12.52.

Natywne opakowanie NAME (32 B, pointer+0, maxlen+10, iface+18) zostało
zweryfikowane wcześniej, a sonda przekazała własny bufor 128 B. Nie ma
podstaw do losowej zmiany rozmiaru, przejścia na O_RDWR lub detach.
Nie ustalono przyczyny wcześniejszego 0x802400ff z endpointu 0x84.

## Jeden następny eksperyment — przygotowanie, bez wykonania

- **Hipoteza:** deskryptor konfiguracji dostępny w enumeracji nie oznacza,
  że interfejs 3 ma aktywny rekord w backendzie; błąd występuje przed
  otwarciem endpointu, a nie przez zajęcie go przez uhid.
- **Poszukiwany dowód:** aktualny indeks konfiguracji i udany/nieudany
  odczyt aktywnego deskryptora interfejsu 3, z kontrolą tożsamości na tym
  samym fd i surowym errno. Bez zmiany konfiguracji lub alternate setting.
- **Podstawa ABI:** w zatwierdzonym RX Usbd lokalny backend +0x9240,
  ioctl +0x9259/+0x926a używa 0x40045564, własnego 4 B wyniku oraz fd+0x70;
  po sukcesie kopiuje jeden bajt wyniku do wyjścia. Nie wywołano tej funkcji.
  Porównawczy FreeBSD zwraca tu **indeks**, nie bConfigurationValue:
  nie wolno interpretować zera jako „unconfigured”. Kontrakt query
  aktywnego interfejsu i sentinel konfiguracji wymagają jeszcze osobnego
  sprawdzenia natywnego kodu. Same publiczne nagłówki nie wystarczą.
- **Sukces:** spójny wynik konfiguracji i odczyt aktywnego interfejsu
  rozdzielą hipotezę brakującej konfiguracji od problemu zapytań o sterownik.
  Poprawny interfejs 3 osłabi hipotezę jego braku i skieruje badanie na
  specyfikę Orbis query lub pierwszy błąd FS_OPEN.
- **Niepowodzenie:** odmowa lub nieustalona semantyka indeksu pozostawi
  stan UNKNOWN; nie będzie podstawą do SET_CONFIG, detach czy kolejnego IN.
- **Architektura:** brak aktywnego interfejsu wymaga wyjaśnienia inicjalizacji
  transportu; aktywny interfejs kieruje uwagę na backend endpointu. Żaden
  z wyników nie rozwiązuje parowania lub odbioru Bluetooth.
- **Odczytowość / ryzyko:** planowany własny fd O_RDONLY, metadane, bez
  transferów, FS, resetu i zmian systemu. Prywatne ioctl nadal mogą zawieść
  w sterowniku. **Brak nowego ELF i brak zgody na jego uruchomienie**;
  najpierw kończymy audyt kontraktu i przedstawiamy dokładny zakres.

## Integralność i automatyzacja

Lokalny katalog `build/hardware/usb-owner-20261030-01` zawiera manifest
preflightu, send_attempted=false. Ogólne FTP 550 nie zostało uznane za
brak pliku. Dopiero kompletne, ograniczone NLST /data wykazało nieobecność
własnej nazwy markera; inne nazwy były odrzucane, bez czytania treści plików.
Poprawiono kolektor i dodano testy odmowy, obecnego markera i limitu nazw.

Katalog `build/hardware/usb-owner-20261030-02` zachowuje baseline,
cumulative/new.log, manifest i analizę. Jeden send UTC 12:05:43.861722,
świeży delta log pobrany UTC 12:05:45.121968: 795 B. Pełne lokalne logi
i kopie Sony pozostają poza Git; publikujemy własne metryki i hashe.
ELF pozostał identyczny. Nie uruchomiono żadnej kolejnej sondy.
