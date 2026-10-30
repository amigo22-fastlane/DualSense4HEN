# Zamienniki DS4, protokół PS4 i znaczenie dla DualSense4HEN

Audyt 2026-10-30, gałąź `codex/architecture-research-1252`.
Nie wykonano żadnego nowego eksperymentu na PS4. Publiczny kod sklonowano
do ignorowanego `research/`; nie uruchamiano go na kontrolerach ani konsoli.

**Kompatybilny zamiennik dowodzi, że PS4 może obsłużyć inny sprzęt niż Sony
DS4. Nie dowodzi, że zaakceptuje dowolny kontroler Bluetooth HID.**
Istotny jest protokół widoczny dla PS4: transport, profil/deskryptory,
raporty i ich aktywacja, uwierzytelnianie urządzenia oraz przypisanie wejścia.
Nazwa handlowa „wireless” nie określa tego transportu.

## Trzy rodzaje zamienników

| Rodzaj | Konkretny publiczny przykład | Co widzi PS4 i co jest potwierdzone | Przydatność dla naszego celu |
|---|---|---|---|
| Bezpośredni Bluetooth | **HORI Wireless Controller Light, PS4-133** | Instrukcja producenta każe wybrać urządzenie w Settings → Bluetooth Devices; nie wymaga odbiornika USB | Potwierdza istnienie alternatywnego kontrolera używającego wewnętrznego BT PS4. Instrukcja nie ujawnia implementacji auth/HID |
| Przewodowy USB | **Nacon Wired Compact Controller** | Instrukcja producenta opisuje kontroler USB dla PS4; to inny transport niż BT | Przydatny do porównania USB HID i inicjalizacji; nie jest dowodem otwarcia kanałów BT |
| Bezprzewodowy z odbiornikiem | **Nacon Asymmetric Wireless Controller** | Producent wymienia Bluetooth USB dongle; konsola komunikuje się z odbiornikiem przez USB | Radio kontroler–odbiornik nie zapewnia zgodności z wewnętrznym Bluetooth PS4 |
| Tani klon bez określonego modelu | Brak identyfikowalnej próbki w projekcie | Nie znamy firmware, protokołu, ewentualnego odbiornika ani czasu działania | Nie wolno przypisać wszystkim takich samych kluczy, układu scalonego lub obejścia |

Źródła producentów:
[HORI PS4-133 — instrukcja](https://hori.jp/image/2019/01/ps4-133_manual_en.pdf),
[Nacon Wired — instrukcja](https://www.nacongaming.com/media/downloadable_media/PS4OFCPADCL_IB_Web_20190424.pdf),
[Nacon Asymmetric — specyfikacja i receiver](https://www.nacongaming.com/en/asymmetric-wireless-controller).
Nie utożsamiamy ONYX z ONYX Plus: podobna nazwa nie ustala transportu.

Nie znaleziono w przejrzanych źródłach kompletnego, otwartego firmware
konkretnego taniego klona z potwierdzonym natywnym BT na PS4 12.52.
Poniższe projekty dają jednak rzeczywiste implementacje części protokołu.

## Otwarte implementacje i licencje

| Projekt i przypięty commit | Zweryfikowany kod | Wniosek i ograniczenie |
|---|---|---|
| GIMX, `e44a1cc9823939165d3d92418e9d679c66496ee1`, GPLv3 | `core/connectors/btds4.c`: SDP/HID, oba PSM, transmisja i dwukierunkowy relay Control do prawdziwego DS4 | To emulacja DS4 przez BT z obsługą kanału kontrolnego i urządzeniem uwierzytelniającym; nie sam mapper przycisków i nie backend Orbis |
| PassingLink, `b5a6ec0df749ea4dffdf1535044a47e2930f4a5f`, MIT | `src/output/usb/ps4/hid.cpp`, `auth.cpp`: descriptor, input, feature requests, stan i podpis challenge | Działający kod modelu urządzenia USB; nie implementacja parowania wewnętrznego BT PS4 |
| GP2040-CE, `0e33f08fbaf8bd3528950ed0d6f2ea73d259b153`, MIT plików PS4 | PS4Driver, PS4Auth i USBListener: identyfikacja, raporty, auth lokalny lub przez USB host | Rozdziela generowanie wejścia i odpowiedzi auth; zależności mają osobne licencje. Kod dla MCU/USB nie jest wtyczką GoldHEN |
| Linux hid-playstation, GPL-2.0-or-later | Osobne formaty DS4 i DualSense, CRC i parsery | Referencja protokołu hosta; nie uwierzytelnia kontrolera jako PS4 i nie opisuje Orbis ioctl |
| SDL PS4 HIDAPI, zlib | Detekcja raportem capabilities 0x03, różnice producentów i trybu enhanced | Pokazuje, że nawet host PC potrzebuje więcej niż VID/PID; nie jest implementacją zabezpieczeń PS4 |

Kod projektów nie został dołączony do produkcyjnego DualSense4HEN.
Nie pobierano prywatnych kluczy ani firmware kontrolerów. Zgodność licencji
pozwala rozważać wykorzystanie konkretnych komponentów z zachowaniem notice,
ale nie dostarcza sterowników urządzeń Orbis. Wersje i zakres:
[ds4-compatibility-sources.json](ds4-compatibility-sources.json).

Źródła kodu:
[GIMX BT](https://github.com/matlo/GIMX/blob/e44a1cc9823939165d3d92418e9d679c66496ee1/core/connectors/btds4.c),
[PassingLink auth](https://github.com/passinglink/passinglink/blob/b5a6ec0df749ea4dffdf1535044a47e2930f4a5f/src/output/usb/ps4/auth.cpp),
[PassingLink HID](https://github.com/passinglink/passinglink/blob/b5a6ec0df749ea4dffdf1535044a47e2930f4a5f/src/output/usb/ps4/hid.cpp),
[GP2040 PS4](https://github.com/OpenStickCommunity/GP2040-CE/tree/0e33f08fbaf8bd3528950ed0d6f2ea73d259b153/src/drivers/ps4).

## Co obejmuje kompatybilność, poza VID/PID

Poniższe warstwy są punktami diagnostycznymi, nie gwarantowaną kolejnością
procedur każdego firmware. Niektóre inicjalizacje i auth zachodzą równolegle.

| Warstwa | Kompatybilny model DS4 / otwarta implementacja | DualSense i stan naszego dowodu |
|---|---|---|
| Tożsamość i profil | SDP/HID, typ urządzenia i capabilities; możliwe własne VID/PID licencjonowanego modelu | UI pokazuje DualSense/Kontroler; źródło tej klasyfikacji w UI nieustalone |
| Połączenie BT | Link, jego zabezpieczenia, reconnect/bonding to osobne etapy | Użytkownik potwierdził Połączono bez USB; brak trace auth/encryption. Reconnect po restarcie nie zadziałał automatycznie |
| Kanały HID | GIMX jawnie zestawia Control PSM 0x11 i Interrupt PSM 0x13 | Oba kanały w tej sesji nadal UNKNOWN; sam status UI ich nie wykazuje |
| Aktywacja i Feature | Kod DS4 implementuje calibration/capabilities/version i żądania kontrolne; BT ma własne ramki | DualSense ma inne feature reports; nie wysyłamy sekwencji DS4 do DualSense |
| Wejście | DS4 USB report 0x01; rozszerzony BT m.in. 0x11; vendor payload i CRC | DualSense USB też 0x01, lecz inny układ; BT pełny 0x31. Parser istnieje, realnego raportu z PS4 nie mamy |
| Uwierzytelnienie urządzenia PS4 | PassingLink/GP2040 realizują challenge-response, GIMX BT przekazuje kanał Control do DS4 | Nie mamy trace challenge/odpowiedzi ani wyniku uwierzytelnienia DualSense na PS4 |
| Urządzenie systemowe i użytkownik | Obsługiwany przycisk PS i zarejestrowane wejście pozwalają wybrać użytkownika | Użytkownik nie widzi wyboru użytkownika po PS. To objaw, nie wskazanie jednego konkretnego etapu odmowy |
| Menu/gry, klient scePad | Dane muszą dotrzeć do właściwego widoku wejścia | Brak sterowania menu. Dawna próba Pad nie przeszła nawet kontroli DS4 L2, więc nie dowodzi globalnego braku rejestracji |

Formatów nie wystarczy przemianować: w pełnym DS4 BT wspólne dane zaczynają
się po innym nagłówku niż w DualSense, a przyciski/triggery mają inne pozycje.
Zamiana samego 0x31 na 0x11 spowodowałaby błędną interpretację danych.
Obie rodziny mają też tryby uproszczone; brak pełnego trybu nie wyjaśnia
samodzielnie braku całego wejścia. Referencje:
[Linux hid-playstation](https://github.com/torvalds/linux/blob/master/drivers/hid/hid-playstation.c),
[SDL PS4 HIDAPI](https://github.com/libsdl-org/SDL/blob/main/src/joystick/hidapi/SDL_hidapi_ps4.c).
Komentarz SDL o przewodowych urządzeniach third-party nie jest dowodem,
że wszystkie takie kontrolery na rynku są przewodowe; przeczy temu instrukcja HORI.

### Dwa różne uwierzytelnienia

Zabezpieczenia linku Bluetooth (klucz linku, uwierzytelnienie, szyfrowanie)
to inna rzecz niż aplikacyjne uwierzytelnienie peryferium PS4.
PassingLink podpisuje challenge przez RSA-PSS/SHA-256; model USB używa
Feature F0 do przekazania challenge, F2 do stanu i F1 do odpowiedzi.
Nie wynika z tego, że każda tania kopia posiada ten sam klucz lub że wystarczy
odesłać stały pakiet. GIMX BT świadomie relacjonuje Control do prawdziwego DS4.
[Raport autora analizy DS4](https://fail0verflow.com/blog/2018/ps4-ds4/)
wskazuje weryfikację podpisu i stan auth w sterownikach peryferiów PS4;
nie dostarcza ABI ani offsetów naszej wersji 12.52.

GP2040-CE opisuje model USB, w którym wejście działa początkowo, a brak
poprawnego auth skutkuje limitem około 8 minut. To **nie odpowiada samo w sobie**
obserwacji „żaden przycisk nie działa od początku”. Inny punkt kontroli BT,
nieobsługiwany profil lub format nadal mogą blokować wcześniej.
[Dokumentacja kompatybilności GP2040-CE](https://gp2040-ce.info/faq/faq-console-compatibility/).
Nie przenosimy tego czasu jako wykrytego timeoutu naszej sesji.

### Zrzuty i przypadki klonów — co jest wiarygodnym tropem

GIMX zawiera zapisy SDP/HID i implementację ramek DS4. Dokumentacja autora
[dsremap — reverse engineering](https://dsremap.readthedocs.io/en/latest/reverse.html)
zawiera deskryptory oraz zaobserwowaną inicjalizację DS4 i różne Feature ID
DualSense. Jest zapisem konkretnego badania, nie uniwersalną specyfikacją
wszystkich kontrolerów. Nie kopiujemy jej sekwencji modyfikujących parowanie.

Zgłoszenie [BlueZ #2678](https://github.com/bluez/bluez/issues/2678) opisuje
MOVONE i ślad rozdzielający połączenie linku, błąd auth oraz późniejsze otwarcie
PSM 17/19 po innej inicjalizacji. To relacja autora na Linuksie, nie własny
test PS4. Istotna ostrożność: autor nazywa 0x14/02 „commit”, podczas gdy
dsremap opisuje ten kod jako unpair/disconnect. Nie rozstrzygamy semantyki
dla każdego klona i **nie wysyłamy tej komendy**.

Zgłoszenie [MissionControl #1142](https://github.com/ndeadly/MissionControl/issues/1142)
opisuje Redragon G818 o ID 054c:09cc i mapowaniu DS4, który nie kończy SSP
na Switchu. To nieweryfikowana przez nas relacja z innego systemu; pokazuje,
dlaczego zgodny descriptor/VID nie rozstrzyga połączenia. Nie dowodzi przyczyny
DualSense na PS4 i nie uzasadnia zmian kluczy ani wpisów parowania.

## Dlaczego Połączono/Kontroler nie wystarcza

Nazwa i ogólny typ mogą być dostępne wcześniej niż poprawne dostarczanie
wejścia konkretnego modelu. Nadal konkurują trzy hipotezy:

1. Link działa, lecz kanały HID lub wymagana inicjalizacja nie są kompletne.
2. Raporty docierają do niższej warstwy, ale profil/parser/stan auth PS4
   nie dopuszcza ich jako wejścia DS4.
3. Wejście jest przetwarzane częściowo, ale nie dochodzi do zarejestrowanego
   kontrolera/użytkownika i globalnego menu.

Żadnej z nich nie rozstrzyga obecny klog. Nie zakładamy konkretnego źródła
etykiety UI ani nie utożsamiamy świeżego MBus ID z DualSense.
Nowy dowód lokalny to [łańcuch devId](DEV_ID_AND_QUERY_BOUNDARY.md): rekordy
/dev/mbus → znakowanie ID → dispatcher → GetDeviceInfo. Nie mamy jeszcze
kodów writer-side ioctl i potwierdzonego punktu obserwacji surowego wejścia.

## Co wykorzystać w DualSense4HEN

Najbardziej użyteczny jest **podział funkcji** znany z projektów zgodnych:
osobny kanał kontroli i cykl inicjalizacji/auth, osobny transport wejścia,
osobny translator i publikacja kontrolera. Do naszego parsera można dostarczyć
raporty 0x01/0x31 bez udawania, że DualSense zmienił swój firmware na DS4.
Wymaga to potwierdzonego dostępu do raportów z istniejącej usługi PS4.

Hostowy translator do własnego wirtualnego DS4 pozostaje obiecujący, ale
wymaga uprawnień i rejestracji, których nie uznajemy za potwierdzone na 12.52.
Emulacja zewnętrznego DS4 oznaczałaby natomiast realizację jego całego
kontraktu i auth; otwarty kod MCU nie zmieni zdalnie protokołu DualSense.
Nie ma zweryfikowanego otwartego firmware DualSense z takim trybem.
Rozwiązania z odbiornikiem/PC/MCU pozostają wyłącznie źródłem wiedzy.

## Eksperymenty o wartości diagnostycznej

| Eksperyment | Hipoteza i potrzebny dowód | Sukces / niepowodzenie | Wpływ na architekturę | Odczytowość i ryzyko |
|---|---|---|---|---|
| **BT-ABI, następny minimalny krok** | Handler /dev/hid 0x80104801 ujawni zakres zapisu i pola stanu; dokładny fragment opisano w audycie devId | Kontrakt i tożsamość pozwolą przygotować sondę; ich brak pozostawia UNKNOWN | Rozstrzyga możliwość użycia usługi natywnej bez nowego parowania | Teraz tylko analiza offline; nowy kod systemowy wymaga osobnej zgody i określenia sposobu pozyskania |
| **BT-INPUT, dopiero po potwierdzeniu interfejsu** | Zaistniały kanał dostarcza do obserwatora raporty od jednoznacznie wskazanego DualSense; X zmienia dane | Poprawny raport i korelacja X potwierdzą odbiór; cisza klienta bez kontroli jego zakresu niczego nie odrzuci | Udany odbiór odblokuje parser i ścieżkę publikacji wejścia | Nie przygotowano ELF, bo nieznane są kolejka/replay/ABI; nowy read może konsumować wspólną kolejkę |
| **USB-OWNER, osobne późniejsze okno** | Interface 3 ma przypisany sterownik; pytamy ACTIVE i nazwę na tym samym znanym urządzeniu | Nazwa wskaże przypisanie, odmowa query nie oznacza braku sterownika; żaden wynik nie dowodzi sam przyczyny ff | Wybór obserwacji systemowego HID albo dalszego audytu backendu endpointu | Przygotowany ELF tylko query; fizyczny USB może zmienić transport, dlatego nie uruchomiono. [Procedura](USB_OWNER_EXPERIMENT.md) |

Nie zaplanowano testu „podmień VID/PID”, zapisu parowania, kopiowania kluczy,
patcha auth lub detach. W tej iteracji nie przeprowadzono nowych testów
syntetycznych raportów; rozszerzony profil wykonano na rzeczywistym,
wcześniej zatwierdzonym ELF, a testy jednostkowe chronią zakres narzędzia.
