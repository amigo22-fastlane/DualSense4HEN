# Bluetooth HID — ustalenia z własnej konsoli 12.52

**Aktualizacja 2026-10-20:** istniejące połączenie bez USB jest potwierdzone
przez właściciela. Wykonano wariant pasywny 35 s: callback=0, join=0,
zdarzenia=0. Nie powtarzać poniższych historycznych instrukcji Create+PS lub
restartu. Obecny eksperyment zachowuje sesję; brak zdarzeń nie rozstrzyga
stanu L2CAP. [Wyniki](research/EXISTING_BT_SESSION.md).

Właściciel wyraźnie zatwierdził lokalny odczyt i analizę biblioteki Sony.
Segmenty pozostają wyłącznie w ignorowanym `build/hardware/`; nie dołączamy
ich do repozytorium, paczek ani dokumentacji. Poniżej są własne wnioski o ABI,
nie kod biblioteki. Analiza nie obejmuje jądra ani jego modyfikacji.

## Potwierdzone pomiary

Sonda ELF uruchomiła się przez GoldHEN w `ScePartyDaemon`. System zgłosił
`12.520.001`, numer `0x12520001`. `libSceBluetoothHid.sprx` załadowała się
i udostępniła wszystkie 17 eksportów znanych z PS4LibDoc. To osobne ustalenie
od możliwości parowania konkretnego urządzenia.

Kod odczytano z adresów i rozmiarów zwróconych przez
`sceKernelGetModuleInfo`, według struktury OpenOrbis o rozmiarze 352 bajty.
Biblioteka ma trzy segmenty po 16384 bajty: kod RX (względem bazy 0),
dane RW (+0x8000) i dane R (+0x4000).

SHA-256 segmentu kodu tej konkretnej biblioteki:
`48a4c7f14870555d0f9843f246b705cf821167e157e37d7bb53a0ef78090afc2`.
CRC32 z prefiksem A1 użyte jako dodatkowy identyfikator kompilacji:
`0xb489ea13`. Nie jest to mechanizm zabezpieczeń przed złośliwym modułem;
ma zapobiec przypadkowemu użyciu innej wersji ABI.

## Odzyskane sygnatury (eksperymentalne)

| Eksport | Ustalona postać i dowód przepływu argumentów |
|---|---|
| `sceBluetoothHidInit` | `int32_t (void)`; nie pobiera parametrów. Inicjuje mutex i otwiera `/dev/bluetooth_hid` do odczytu/zapisu. |
| `sceBluetoothHidThreadParamInitialize` | `(param*, int priority, uint64_t affinity)`; zapisuje pola +0, +8, zeruje +16. Jeszcze niewywoływane. |
| `sceBluetoothHidParamInitialize` | `(param*, thread_param*)`; zapisuje wskaźnik +0 i zeruje flagi +8. Jeszcze niewywoływane. |
| `sceBluetoothHidRegisterCallback` | `(callback, void *user, const param*)`; callback musi być niezerowy, trzeci argument może być NULL. Parametry opcjonalne odsyłają do ustawień wątku. |
| callback | `void (const event*, int count, void *user)`; watcher przekazuje tablicę, liczbę elementów oraz oryginalny user pointer. |
| `sceBluetoothHidUnregisterCallback` | `int32_t (void)`; kasuje callback, zatrzymuje watcher i zamyka jego kqueue. Nie znaleziono wywołania join; sam unregister nie jest dowodem zakończenia wątku. |
| `sceBluetoothHidRegisterDevice` | `int32_t (uint16_t arg1, uint16_t arg2)`; tworzy 4-bajtowy blok dwóch wartości 16-bitowych dla wewnętrznego ioctl. Wymaga wcześniej callbacku. Użycie ich jako VID/PID jest hipotezą wymagającą niezależnego potwierdzenia. **Nie ustalono API parowania**. |
| `sceBluetoothHidUnregisterDevice` | analogiczny blok VID/PID dla cofnięcia rejestracji. |
| `sceBluetoothHidGetInputReport` | dwa argumenty, przekierowane do helpera; pełna semantyka jeszcze nieustalona i funkcja nie jest wywoływana. |

Watcher buduje elementy o rozmiarze 0x68 (104) bajtów. Ustalony układ:
identyfikator urządzenia +0 (8 B), VID +8, PID +10, typ +12 (1 B), status
+13 (1 B), długość +14 (2 B), pole nieustalone +16 (8 B), raport +24
(maksymalnie 78 B), końcowe wyrównanie. Callback przekazuje najwyżej 16
elementów. Te obserwacje nie rozstrzygają jeszcze znaczenia wszystkich typów
zdarzeń; observer waliduje rozmiar, VID/PID i cały raport wraz z CRC.

## Bezpieczne zakończenie observera

Nie ma publicznego eksportu `sceBluetoothHidTerm`. Po inicjalizacji sonda
zachowuje moduł w procesie do restartu, zamiast odładować go z otwartym
deskryptorem i mutexem. Observer odmawia przejęcia już obecnego kontekstu.
W tej wersji oznacza to jeden eksperyment observera na uruchomienie konsoli.

Do zakończenia wątku używa rzeczywistego `scePthreadJoin`, po wyrejestrowaniu
filtra i callbacku. Uchwyt watchera biblioteka zapisuje w swoim segmencie
danych, **+0x8048 względem bazy modułu**. To udokumentowany tu, odczytywany
slot konkretnej biblioteki, ustalony z instrukcji zapisu po `scePthreadCreate`
i użycia przez funkcję zmiany priorytetu/affinity. To **nie offset jądra i nie
adres patcha**. Kod sprawdza hash całego segmentu kodu, jego rozmiar oraz
przynależność slotu do odczytywalnego segmentu, zanim go odczyta. Nie zapisuje
do tego slotu. Na innej bibliotece bezwarunkowo odmawia działania.

Jeżeli unregister lub join nie potwierdzi końca wątku, payload pozostaje
rezydentny z ważnym kontekstem callbacku; nie zwalnia kodu spod działającego
wątku. Wymagany jest wtedy ręczny restart konsoli, opisany w logu. Brak
automatycznego restartu lub zmian jądra.

Observer może otworzyć 180-sekundowe okno po udanej rejestracji. W wykonanym
teście rejestracja zwróciła 0x81540002, więc do okna nie doszło. Nie wywołuje
operacji bond, nie zapisuje
kluczy Bluetooth, nie dotyka konfiguracji systemowej ani nie udaje udanego
parowania. Test: odłączyć kabel DualSense, wejść do systemowej listy urządzeń
Bluetooth i użyć Create+PS. Wynik odmowy/powodzenia trzeba zapisać osobno.

## Dalsza analiza błędu 0x81540002

Z już lokalnie odczytanego kodu tej samej biblioteki odtworzono tabelę
translacji błędów RegisterDevice. Wewnętrzne wywołanie ioctl `0x8004622b`
przekazuje dwie wartości 16-bitowe w 4-bajtowym bloku. Ścieżka translacji
przekształca **0x802f3701 w 0x81540002**. Inne stany, w tym brak
inicjalizacji (0x81540004) i callbacku (0x81540006), mają oddzielne ścieżki.
Wynik jest zatem zgodny z odmową rejestracji w warstwie urządzenia, a nie
automatycznym dowodem błędu inicjalizacji lub braku callbacku.

To własna analiza przepływu kodu, nie nowy pomiar ioctl: wrapper przepuszcza
też niektóre nieznane statusy, więc sam zwrócony kod nie dowodzi jednoznacznie
surowego wyniku sterownika. Nie ustalono warunku odrzucenia, allowlisty,
ograniczenia Sony ani konieczności patcha. Nie zamieniamy tych hipotez w fakty.
Nie zmieniono argumentów ani nie uruchomiono ponownie observera.

Reprodukcja offline: `python tools/analyze-bt-errors.py build/hardware/bt-seg0.bin`.
Skrypt weryfikuje pełny SHA-256 i rozmiar przed użyciem tabeli +0x1ab0 i
ścieżki +0xb44 (adresy względne **tej biblioteki**, nie jądra). Wypisuje tylko
liczbowe mapowania, bez bajtów Sony. Binaria nadal pozostają poza GitHub.
Następny rozstrzygający krok to niezależny call-site rejestracji na 12.52 lub
potwierdzenie warunku sterownika, zanim zaproponujemy zmiany jego zachowania.

## Pasywne okno bez RegisterDevice

Dalszy lokalny audyt wcześniej zatwierdzonej biblioteki wykazał, że
RegisterCallback tworzy watcher przed rejestracją filtra urządzenia. Watcher
obsługuje zdarzenia cyklu połączenia i raportów, ale nie wykazano, jakie
zdarzenia sterownik rzeczywiście przekazuje bez filtra. Nie dowodzi to bondingu
DualSense ani możliwości uzyskania raportów bez RegisterDevice.

Dodano oddzielny wariant `D4H_BT_PASSIVE`, budowany przez `-IncludeBtPassive`.
Nie rozwiązuje i nie wywołuje RegisterDevice/UnregisterDevice; obserwuje callback
przez maksymalnie 30 sekund, po czym wyrejestrowuje go i wykonuje join.
Zachowuje zgodność profilu, własny kontekst i pin modułu do restartu.
Hostowe testy sprawdzają brak rejestracji, limit, stop i zakończenie watchera.
**Nie uruchomiono go jeszcze na PS4.** Najpierw należy zakończyć test USB;
przyszły test BT wymaga świeżego kontekstu, kabla odłączonego od DualSense
i zanotowania zachowania standardowej listy urządzeń Bluetooth przy Create+PS.
