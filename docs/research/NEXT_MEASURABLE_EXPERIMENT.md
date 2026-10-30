# Decyzja: zamknięcie etapu A i przygotowanie jednego pomiaru USB

**Aktualizacja po wykonaniu B, 2026-10-30:** jednorazowy test USB-owner
został osobno zatwierdzony i zakończył się odczytem tożsamości urządzenia,
ale błędami ACTIVE/NAME (errno 6/22). Brak dowodu zajęcia przez uhid.
[Nowe wyniki i rozstrzygający następny krok](USB_OWNER_RESULTS.md).
Pozostała część dokumentu zachowuje stan przygotowania z de0d8fd.

2026-10-30. Punkt odniesienia: 1dd0454. W tej iteracji nie wykonano żadnych
operacji na PS4: połączeń FTP/klog, payloadów, parowania, kolejek lub VDA.
Ponownie sprawdzono lokalne kopie i zbudowano narzędzia odbioru logu.

| Zadanie | Stan | Konkretny wynik |
|---|---|---|
| A: GetDeviceInfo / istniejące Bluetooth | **BLOCKED na zatwierdzonych materiałach** | devId z rekordu MBus potwierdzony; brak pełnego writer-side ABI i identyfikacji DualSense |
| B: USB-owner | **READY FOR SEPARATE APPROVAL — NOT RUN** | Ten sam ELF, lokalna kontrola struktury i profilu; gotowy jednorazowy sender, kolektor i analizator |
| C: wirtualny DS4 | **PLAN ONLY — NOT RUN** | Oddzielono operacje stanowe, zmianę przywilejów, własność ID i sprzątanie |

BLOCKED oznacza koniec bieżącej iteracji audytu GetDeviceInfo, nie porzucenie
Bluetooth. Wznowienie ma sens po uzyskaniu konkretnego nowego kodu lub
potwierdzonego raportu wejścia; nie powtarzamy tego samego pustego observera.

## A. Co wiemy o możliwości odbioru Bluetooth

PS4 według właściciela utrzymuje bezprzewodowe Połączono/Kontroler.
To potwierdza obserwowany stan UI, ale nie dokumentuje zestawienia PSM
0x11/0x13, szyfrowania, dostarczania HID lub rejestracji w globalnym scePad.
Wcześniejszy observer nie otrzymał zdarzeń; nie znamy replay/filtrów jego
klienta. Próba scePad nie przeszła dodatniej kontroli L2 DS4, więc nie
wyznacza miejsca odrzucania DualSense. Pierwszego prawdziwego raportu nadal brak.

SceSysCore odbiera 48-bajtowe rekordy /dev/mbus. W zweryfikowanej ścieżce
przekazuje ID rekordu, po znakowaniu górnych bitów, do GetDeviceInfo jako
dolne 32 bity. Istnieje też lookup ID w lokalnym rejestrze 32 wpisów.
Żaden dotychczasowy log nie wiąże konkretnego ID z DualSense 054c:0ce6.
Nie odczytujemy tych kolejek bez ustalenia, czy są per-client czy współdzielone.

Ponowny lokalny profil HidControl nadal wykazuje /dev/hid i 0x80104801:
16 B opakowania zawiera wskaźnik, nie definiuje rozmiaru wskazywanego wyniku.
SceSysCore jest konsumentem pól, a biblioteka przekazuje żądanie do ioctl.
Nie dysponujemy zweryfikowaną implementacją sterownika Orbis 12.52,
która zapisuje wynik. Otwarty FreeBSD i nagłówki eksportów nie zastępują jej.
Nie ustalono pól CID ani licznika raportów. Szczegóły i hashe:
[DEV_ID_AND_QUERY_BOUNDARY.md](DEV_ID_AND_QUERY_BOUNDARY.md).

### Czy dalszy audyt ma sens i czego dokładnie brakuje

Tak, **kod producenta wyniku** może ujawnić rozmiar i stan urządzenia.
Samo pełne ABI GetDeviceInfo nie gwarantuje jednak dostępu do raportów;
wynik może być jedynie opisem urządzenia. Dalsze czytanie tych samych
wrapperów ma małą wartość dla celu pierwszego X, więc ten tor wstrzymujemy.

| Wymóg dodatkowego badania | Minimalny zakres i ograniczenie |
|---|---|
| Obiekt | Handler ioctl urządzenia /dev/hid obsługujący 0x80104801 w tej kompilacji; alternatywnie /dev/bt 0x8010a40b, nie oba naraz |
| Lokalizacja | W zweryfikowanym obrazie właściwej kompilacji: dispatch po kodzie żądania → gałąź tworząca wynik → copyout/zapis do wskazywanego bufora. Powiązać z obsługą danego cdev, nie samym przypadkowym wystąpieniem stałej |
| Zakres | Kod tej gałęzi i bezpośrednio potrzebnych funkcji wytwarzających/kopiujących wynik oraz stałe długości. Bez kluczy, parowania, live RW i pełnego zrzutu jądra |
| Adres i liczba bajtów | Nieustalone; zatwierdzone pliki nie ujawniają adresu cdevsw ani granic tych funkcji. Nie podajemy fikcyjnego offsetu lub liczby bajtów |
| Wymagane uprawnienia | Dostęp do odpowiedniego obrazu offline albo osobno zweryfikowany odczyt kodu jądra. Zwykły GetDeviceInfo i sama obecność menu GoldHEN nie wykazują takiej zdolności |
| Ryzyko / decyzja | Sposób nowego odczytu nie jest jeszcze zweryfikowany. Nie ma gotowego payloadu tego audytu i nie wykonujemy go bez osobnej zgody. Nie prosimy o zgodę na nieograniczony zrzut |

To konkretne brakujące miejsce zapisu, a nie propozycja zgadywania struktur.
Na obecnym etapie nie można bezpiecznie wskazać nowego pomiaru BT, który
rozstrzygnie odbiór raportów. Przechodzimy do B.

## B. Rekomendowany jeden eksperyment: USB-owner

**Hipoteza:** interfejs 3 aktualnie wykrytego DualSense ma przypisany sterownik
systemowy. Odczyt metadanych wykaże to przypisanie lub konkretną odmowę.
To inny punkt pomiaru niż ponawianie InterruptTransfer kończącego się ff.
Enumeracja i deskryptor wcześniej działały w PayLoaderze, więc ten test ma
już sprzętową podstawę, choć jego nowe zapytania nie były wykonane.

**Szukany dowód:** bieżący bus/address, zgodne VID/PID odczytane ponownie na
otwartym fd, osobne raw/errno ACTIVE i NAME, zamknięcie fd oraz unload własnego
modułu. Wynik ma zdecydować o następnym badaniu transportu, nie udawać raport X.

ELF: `dist/research/dualsense4hen-usb-owner.elf`, **51088 B**.
SHA256 `2b1c221e8577db8deb311c39addae7e1994aac8f2af4ed2ea8e09dacf4c5b212`.
Kontrola lokalna: ELF64 x86_64 ET_DYN, entry 0x620, 3 LOAD, brak importów,
relokacji i W+X. Runtime sprawdza 12.52, profil RX Usbd i entrypoints getterów.
To gotowość do próby na zgłoszonej konfiguracji, **nie potwierdzenie działania
ELF lub jego ioctl na sprzęcie**. Artefakt nie został zmieniony.

Bezpośredni wrapper query dopuszcza wyłącznie DEVICE_DESC=0x40125569,
ACTIVE=0x8004557c i NAME=0xc0205579. Właściwy węzeł jest otwierany O_RDONLY.
Standardowe Init/GetDeviceList biblioteki wykonują też wcześniej badane
operacje enumeracji i otwierają węzły backendu w O_RDWR. Nie należy opisywać
całego cyklu biblioteki jako „dokładnie trzy ioctl i żadnych zapisów”.
Nie ma wywołania konfiguracji/claim/FS/transferu/detach/resetu ani zasilania.
Runtime zapisuje własny log/lock i zarządza własnym modułem; konfiguracja
systemu i parowanie pozostają poza zakresem. Query sterownika nadal może
odmówić dostępu lub ujawnić błąd jądra; nie gwarantujemy braku awarii.

### Gotowa procedura i automatyzacja

1. **Teraz niczego nie podłączać ani nie uruchamiać.** Najpierw właściciel
   potwierdza zakończenie obserwacji BT i osobno zezwala na USB i ten ELF.
2. Po tej zgodzie podłączyć DualSense z PS5 sprawdzonym kablem danych.
   DS4 z PS4 zostawić do obsługi menu. Nie usuwać parowania. Nie trzeba PS
   ani X na DualSense — ten pomiar nie odbiera wejścia.
3. Sprawdzić aktualny IP oraz włączyć GoldHEN FTP 2121 i PayLoader 9090.
   Nie trzeba PKG ani instalacji. Klog nie jest wymagany dla tego pomiaru.
4. Agent może wysłać ELF i pobrać log automatycznie; użytkownik nie musi
   wykonywać PowerShella. Dla wykonującego ręcznie poniższa komenda jest
   do użycia **wyłącznie po powyższej zgodzie** (IP to ostatnio zgłoszone .12):

```powershell
python tools/usb_owner_test.py prepare
# Powyższe jest wyłącznie lokalne. Poniższe dopiero po zgodzie i USB:
python tools/usb_owner_test.py run --host 192.168.88.12 --directory build/hardware/usb-owner-01 --bt-finished --usb-approved
```

Flagi zapisują potwierdzenie operatora, nie zastępują zgody. Domyślnie nie
ma run ani harmonogramu. Nowy skrypt sprawdza dokładny SHA, zachowuje baseline,
nie usuwa stop-markera, wysyła raz i przez ograniczone okno odbiera log.
Nie wykonuje chmod, nie przestawia na O_RDWR i nie ponawia ELF po błędzie.
Niejednoznaczne FTP 550 nie jest traktowane jako dowód braku pliku.
GoldHEN 2.2 zwraca także ogólne „File unavailable” dla nieistniejącego
stop-markera. Tylko dla tego dokładnego komunikatu kolektor może sprawdzić
kompletne `NLST /data` (maks. 4096 nazw, po 1024 znaki). Zachowuje wyłącznie
informację o obecności własnej nazwy. Obecny marker lub błąd listowania
przerywa próbę; nie usuwa markera i nie zmienia praw.

Pliki lokalne: manifest.json (hash i czasy), baseline.log, cumulative.log,
new.log i analysis.json. Odczyt FTP jest ograniczony do własnego logu
diagnostycznego (4 MiB) i własnego stop-markera. W powyższym fallbacku
odczytywane są dodatkowo same nazwy katalogu; treść innych plików nie jest czytana.
Analizator odrzuca stary sukces, ucięty/wymieniony log, więcej niż jedną
transakcję i brak sprzątania modułu **po** probe exit.

Jeśli okno odbioru się skończyło, ponownie pobrać tylko log, bez wysłania ELF:

```powershell
python tools/usb_owner_test.py collect build/hardware/usb-owner-01
python tools/usb_owner_test.py analyze build/hardware/usb-owner-01/new.log
```

Przy odmowie FTP nie zmieniamy praw. Zachowujemy manifest i omawiamy wynik.
Kod wyjścia run: 0 = odczyty metadanych zakończone (nazwa może być pusta),
3 = zakończona próba z odmową/błędem, 2 = niepotwierdzone zakończenie.
Żaden z nich nie oznacza odebrania przycisku.

### Interpretacja wszystkich kategorii wyniku

| Wynik analizatora | Co oznacza / następny krok |
|---|---|
| DRIVER_NAME_REPORTED | NAME zwrócił nazwę, np. uhid*. To dowód zgłoszonej nazwy sterownika na tym urządzeniu; nie dowód przyczyny ff. Rozważyć ścieżkę istniejącego HID, bez detach |
| EMPTY_DRIVER_NAME | NAME wykonał się i zwrócił pustą nazwę. Osłabia hipotezę nazwanego właściciela; nie dowodzi wolnego endpointu ani możliwości transferu |
| METADATA_QUERY_REFUSED | Co najmniej jedno query się nie powiodło. Zachować każde raw/errno; udana NAME może nadal dać częściowy dowód. Brak zgody na automatyczny retry w O_RDWR |
| ENUMERATION_EMPTY / TARGET_NOT_IDENTIFIED | Pusta lista lub brak potwierdzonego targetu w dodatniej liście; możliwy też błąd odczytu deskryptora pominiętego urządzenia. Nie dowodzi fizycznej nieobecności pada |
| ENUMERATION_FAILED / ENUMERATION_LIMIT_REFUSED | Błąd lub przekroczenie limitu 256; brak zapytań o właściciela |
| NODE_OPEN_REFUSED | Nie udało się otworzyć dokładnego węzła RO. Nie jest to wynik ACTIVE ani dowód braku sterownika |
| IDENTITY_QUERY_FAILED / IDENTITY_CHANGED_OR_INVALID | Nieudana lub niezgodna ponowna identyfikacja na fd; nie wolno interpretować przypisania tego fd jako DualSense |
| REFUSED_BEFORE_USB_START / odmowa profilu, importów, getterów, resident module lub Init | Kontrola wejścia sondy odmówiła działania. Nie mówi nic o podłączonym kontrolerze; zachować kod exit |
| NODE_CLOSE_FAILED / MODULE_CLEANUP_FAILED | Problem sprzątania; nie nazywać pełnym sukcesem, nawet jeśli uzyskano nazwę. Nie restartować automatycznie |
| NO_NEW_RUN / INCOMPLETE / WAITING_FOR_MODULE_CLEANUP | Wykonanie lub zakończenie niepotwierdzone; collect bez ponownego send |
| AMBIGUOUS / INCONSISTENT / brak dowodów / log replaced lub unclassified failure | Zachować raw log. Nie wybierać wygodnego wyniku ani starego exit=0. Nie nadawać nieznanemu kodowi znaczenia |

Dokładne identyfikatory statusów i raw wartości: `usb_owner_log.py`.
[Szczegóły ABI](USB_OWNER_EXPERIMENT.md). ACTIVE raw=0 nie jest interpretowane
jako linuksowy boolean. Pierwszy błędny backendowy ioctl przy transferze
pozostaje osobnym przyszłym pomiarem, po wyniku OWNER i osobnej ocenie.

## C. Wirtualny DS4 i możliwe wyniki bez patchy

[VDA_TEST_PLAN.md](VDA_TEST_PLAN.md) zawiera etapy, niezbędne operacje,
osobną zgodę, utrzymanie obecnych uprawnień i własność/rollback. Sam
`d4h_vda_init` nie nadaje się do testu bez zmiany przywilejów: woła privilege(1).
Nie uruchomiono go. Test danych synthetic/VDA-only w przyszłości potwierdzi
wyłącznie publikację wejścia; nie będzie dowodem transportu fizycznego pada.

Bez patchy możemy po zatwierdzeniu zmierzyć tożsamość USB, statusy metadanych
i cleanup w aktualnym kontekście. Odbiór raportów i VDA zależą od tego, czy
zweryfikowane interfejsy pozwolą na nie przy obecnych uprawnieniach; brak
takiego dostępu nie uruchomi automatycznej eskalacji lub modyfikacji jądra.

## Co uznamy za pierwszy rzeczywisty raport wejściowy

1. Udokumentowane wykonanie konkretnego ELF na PS4 12.52 i świeży log,
   źródło powiązane z DualSense: dla USB bieżące 054c:0ce6/iface/endpoint;
   dla BT identyfikacja konkretnego urządzenia i właściwy kanał/usługa.
2. Zapis niezmienionych odebranych bajtów, długości, report ID i czasu przed
   parserem. Prawidłowy format; dla pełnego BT także poprawny CRC. Sam stan
   kontrolera po mapowaniu lub licznik callbacków nie wystarczają.
3. Sekwencja fizycznego X na DualSense: puszczony → naciśnięty → puszczony,
   odpowiadająca zmianom surowych danych i parsera. Właściciel potwierdza
   wykonanie; rozróżniamy inne pady i zapewniamy fazę bez przycisków.
4. Nie pochodzą to z generatora, fixtures, odtworzenia starego logu lub VDA.
   Odbiór USB nie będzie nazwany odbiorem BT. Odbiór we własnym procesie nie
   będzie nazwany obsługą menu/gier bez osobnego testu integracji.

USB-owner z definicji nie spełnia punktów 2–3: ma dostarczyć jeden nowy,
rozstrzygający wynik metadanych przed przygotowaniem takiego odbioru.
