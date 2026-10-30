# Pochodzenie devId i granica bezpiecznego zapytania

2026-10-30. Analiza wyłącznie zatwierdzonych lokalnych kopii, bez połączenia
z PS4. Wyniki dotyczą obrazu SceSysCore o SHA256
`968fd8d7ac0f65d3949fed95e8d113e57ee77108bf349aad545ea639cf893b69`.
Nie są pomiarem wykonania tych ścieżek dla DualSense.

## Łańcuch źródła identyfikatora

1. SceSysCore otwiera **`/dev/mbus`** w natywnym trybie 2. Call-site open:
   RVA 0x6f437; własny deskryptor jest zapisywany w slocie wskazywanym
   przez kod przy 0x6f467. Slotu działającego procesu nie odczytywano.
2. W pętli kevent sprawdzana jest zgodność identyfikatora źródła zdarzenia
   z tym fd. Native read przy 0x6f664 żąda **48 B** do lokalnego rekordu.
   Publiczny NID oraz relokacja/PLT potwierdzają, że to import `read`.
3. Z użycia rekordu wynikają: rodzina urządzenia u32 przy +0, kod zdarzenia
   u32 przy +4 i identyfikator u64 przy +8. Inne pola pozostają nieopisane;
   nie przypisujemy tym rekordom semantyki surowych raportów HID.
4. Przed dispatch, z wyjątkiem kodu zdarzenia 4, kod 0x701b0 wykonuje
   normalizację: OR wybranego znacznika rodziny do górnych bitów identyfikatora.
   Przypadki 1–13, 16, 32 i 33 mają znaczniki rodzina<<48; pozostałe
   rozpoznane przypadki pozostawiają wartość bez zmian. To nie alokacja
   kolejnego numeru urządzenia. **Dolne 32 bity zostają zachowane.**
5. Rekord trafia do dispatchera 0x702b0. W zweryfikowanej ścieżce
   GetDeviceInfo identyfikator +8 jest przenoszony do argumentu wrappera,
   a ten przekazuje dolne 32 bity jako `devId` do importu.

Odpowiedź dla tej ścieżki: **devId pochodzi z identyfikatora w natywnym
rekordzie /dev/mbus**, nie z losowego indeksu, listy scePad ani VID/PID.
Kod userspace go przekazuje i znakuje; pierwotne nadanie identyfikatora
przez dostawcę /dev/mbus znajduje się poza analizowanym obrazem.
Nie ustalono, który rzeczywisty rekord bieżącej sesji dotyczy DualSense.

Jest też druga ścieżka: 0x614c3 wywołuje lookup 0x883e0, który skanuje
32 wpisy o kroku 0xd8, porównuje dwa bajtowe selektory i zwraca zapisany
64-bitowy ID; GetDeviceInfo jest wywoływane przy 0x614f5. To lokalny rejestr
urządzeń, nie generator losowych ID. Nie przypisujemy selektorom nazw
bus/address bez dalszego dowodu. Zapisujący ID do tego rejestru nie został
jeszcze prześledzony aż do dostawcy. Nie twierdzimy, że wszystkie wywołania
GetDeviceInfo są bezpośrednio wywoływane przez nowe zdarzenie.

OR znacznika nie jest odwracalną identyfikacją konkretnego kontrolera:
bez wiedzy o początkowych górnych bitach i źródle rekordu nie wolno na
podstawie samego prefiksu ID odtwarzać urządzenia. ID z historycznego klog
nadal nie upoważnia do zapytania o DualSense ani do adopcji uchwytu VDA.

Reprodukcja: `orbis_consumer_elf.py ... --native-profile`, jak w
[NATIVE_CONSUMER_RESULTS.md](NATIVE_CONSUMER_RESULTS.md). Rozszerzony wynik
[dev-id-provenance.json](dev-id-provenance.json) zawiera wyłącznie własne
wnioski i liczby. Pełny hash jest wymagany przed interpretacją profilu.

## Dlaczego nie znamy jeszcze rozmiaru zapisu

HidControl i BluetoothHid tworzą opakowania zawierające devId i wskaźnik,
następnie wywołują ioctl na /dev/hid (0x80104801) lub /dev/bt (0x8010a40b).
**16 B zakodowane w żądaniu dotyczy opakowania, nie obiektu pod wskaźnikiem.**
W tych zatwierdzonych wrapperach nie znaleziono kopiowania całego wyniku
do wskazanego bufora. SceSysCore czyta pola po powrocie z wywołania.
Kod wykonujący właściwy zapis przez wskaźnik leży po drugiej stronie ioctl.
Nie ma podstaw, aby nazwać dolną granicę odczytów 37/12 B rozmiarem struktury
lub przyjąć 64 B na podstawie wolnego miejsca na stosie.

Pola: byte +0 porównywany z 1/2, pola identyfikacyjne i bit 7 bajtu +0x24
uczestniczą w decyzjach konsumenta. Znaczenie stanów, flagi i ewentualny
związek z uwierzytelnieniem/HID nie są jeszcze potwierdzone po stronie
producenta. Publiczny opis fail0verflow wskazuje śledzenie uwierzytelnienia
w sterownikach PS4; nie przypisuje jednak tej flagi w naszej kompilacji.
Nie mamy pola o potwierdzonym znaczeniu „HID Interrupt aktywny” ani licznika
raportów, więc nie stwierdzamy, że konsument odrzucił już odebrany X.

## Minimalny dodatkowy fragment kodu

Potrzebny jest **zarejestrowany handler ioctl urządzenia /dev/hid dla
0x80104801** w tej kompilacji systemu: jego gałąź tego żądania i bezpośrednie
ścieżki przygotowania wyniku oraz kopiowania do drugiego wskaźnika.
Należy ustalić źródło długości copyout/zapisu, warianty struktury, walidację
devId i znaczenie produkowanych pól. Minimalny zakres to kod tych funkcji
oraz użyte stałe długości, bez tablic kluczy, danych parowania, żywych RW,
całego procesu lub pełnego zrzutu jądra.

Nie podajemy wymyślonej nazwy `hid_ioctl`, modułu SPRX ani adresu: wrapper
identyfikuje węzeł i żądanie, ale nie ujawnia adresu funkcji cdevsw. Trzeba
najpierw zlokalizować tę gałąź w legalnie dostępnej kopii właściwego obrazu
lub ustalić osobno zweryfikowaną metodę lokalizacji. **Nie przygotowano
payloadu czytającego jądro i nie wykonano nowego odczytu.** Przed uzyskaniem
nowego fragmentu wymagany jest osobny, konkretny zakres i zgoda właściciela.
Jeżeli wybrana zostanie ścieżka BluetoothHid zamiast HidControl, odpowiednikiem
jest handler /dev/bt dla 0x8010a40b; nie trzeba od razu pozyskiwać obu.

Kolejny niezależny brak to semantyka dystrybucji zdarzeń: handler read/open
/dev/mbus lub /dev/bluetooth_hid musi wykazać, czy nowy klient ma własną
kolejkę, jakie urządzenia filtruje i czy odtwarza istniejące sesje.
**Read-only read może konsumować wspólną kolejkę.** Sama znajomość 48 B
nie wystarcza do uznania nowego czytnika za pasywny i bezpieczny dla DS4.
Nie otwieramy tych węzłów na PS4 w celu kolejnej próby.

## Eksperyment BT-ABI — przygotowany zakres badawczy, niewykonany

- **Hipoteza:** handler konkretnego żądania ujawnia ograniczony zapis i
  pola stanu istniejącego urządzenia, wystarczające do zapytania bez parowania.
- **Dowód:** rzeczywista długość zapisu i jej źródło, gałęzie dla niepoprawnego
  ID oraz pochodzenie pól; potem jednoznaczna tożsamość DualSense.
- **Sukces:** ustalenie kontraktu umożliwi przygotowanie sondy stanu. Nie
  oznacza automatycznie widoczności raportów lub kanałów L2CAP.
- **Niepowodzenie:** dynamiczny/nieznany wariant, brak writer-side kodu lub
  brak tożsamości pozostawia zakaz zgadywanego zapytania. Jeśli pola są tylko
  opisowe, potrzebny będzie osobny punkt wejścia raportu przed scePad.
- **Wpływ na architekturę:** rozstrzyga dostęp do istniejącej usługi; nie
  wymusza własnego parowania ani emulacji tożsamości zewnętrznego DS4.
- **Odczytowość i ryzyko:** obecna analiza offline jest tylko odczytowa wobec
  PS4 i nie ingeruje w konsolę. Pozyskanie nowego kodu nie jest zatwierdzone;
  jego sposób i ryzyko trzeba ocenić przed wykonaniem, bez patchy i AuthID.

Nie ma jeszcze wystarczających podstaw do nowego eksperymentu sprzętowego
GetDeviceInfo lub surowego odczytu kolejki. To wskazanie konkretnego brakującego
writer-side fragmentu, nie kolejny test syntetyczny ani odmowa celu Bluetooth.
