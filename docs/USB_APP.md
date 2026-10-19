# Samodzielna aplikacja USB — 1.03

Cel: porównać ten sam kod USB w zwykłej aplikacji OpenOrbis i w procesie
PayLoadera. To diagnostyka dla 12.52, nie sterownik do gier. Komputer dostarcza
PKG i pobiera wyniki; nie przekazuje wejścia kontrolera.

## Instalacja i pomiar

1. Podłącz **DualSense z PS5** nowym kablem danych bezpośrednio do PS4.
   DualShock 4 pozostaw do obsługi menu.
2. Zainstaluj `DualSense4HEN-USB-Diagnostic.pkg` z `dist/app/` przez GoldHEN
   Package Installer. Na konsoli właściciela kopia nazywa się
   `/data/pkg/DualSense4HEN-USB-Diagnostic-1.03.pkg`. Zastąp poprzednią wersję.
3. Uruchom ikonę „DualSense4HEN USB Diagnostic”. Poczekaj 5 sekund od pierwszego
   powiadomienia; czarny ekran jest spodziewany, aplikacja nie rysuje interfejsu.
4. Naciskaj i puszczaj **X na DualSense z PS5 podłączonym przez USB** przez
   około 30 sekund. Możesz też poruszyć analogami i triggerami.
5. Zanotuj końcowe `result`, `reports`, `X_presses`. Aplikacja pozostaje otwarta
   po zwolnieniu USB, aby sandbox i log były dostępne przez FTP.
6. Pobierz log **przed zamknięciem aplikacji**. W bieżącej sesji robi to agent;
   użytkownik nie musi uruchamiać PowerShella.

Opcjonalna ręczna kolekcja:

```sh
python tools/collect-app-log.py --host ADRES_PS4 --output build/hardware/app-usb-103.log
```

Pliki do odesłania: `app-usb-103.log`, odpowiadający JSON z czasem i hashem,
treść końcowego powiadomienia, informacja o naciskaniu X na PS5 i zachowaniu
menu. Pełnego klog nie publikuj: zawiera też dane innych usług.

Log jest otwierany przez `sceKernelOpen` w `/data/dualsense4hen-app.log`,
a przy odmowie w `/download0/dualsense4hen-app.log`. Ten drugi plik powinien
być widoczny z FTP pod
`/mnt/sandbox/DSHN00001_000/download0/dualsense4hen-app.log` podczas działania
aplikacji; dostępność tej ścieżki wymaga testu na konsoli. Metadane własnej
aplikacji rezerwują standardowy obszar download (`DOWNLOAD_DATA_SIZE=64`).
Każdy start nadpisuje poprzedni log aplikacji, nie log PayLoadera.

Nie uruchamiaj jednocześnie sondy USB PayLoadera. Po pobraniu wyniku zamknij
aplikację **DualShockiem 4 przez przycisk PS → Zamknij aplikację**. Wersja 1.03
nie wraca do CRT `_exit` i nie próbuje uruchamiać ścieżki `exit`.

Jeśli LIST pokazuje log, a RETR zwraca 550, sprawdź właściciela i prawa pliku.
W teście 1.02 plik uid=1/mode=0600 wymagał osobnej zgody właściciela na 0644;
po tej ograniczonej zmianie pobranie działało. Narzędzie kolekcji samo nie
zmienia praw. Nie zmieniaj praw katalogów, węzłów USB ani plików systemowych.

## Co robi kod

Wspólne `usb_probe.c` zachowuje firmware guard, odczyt deskryptorów, wybór HID
z konfiguracji, odczyty kontrolne, interrupt IN 64 B / timeout 100 ms i limit
30 sekund. Odbiór może zakończyć się wcześniej po błędzie. Nie ma resetu,
odłączania sterownika, SetConfiguration, komend OUT ani zmiany uprawnień.

Wersja 1.03 dodatkowo otwiera `/dev/usbctl` z O_RDONLY i natychmiast zamyka
uchwyt, zapisując wynik/errno. Nazwę węzła i tryb sprawdzono w wcześniej
zatwierdzonym lokalnym audycie biblioteki 12.52. Nie wysyła prywatnych ioctl,
nie odczytuje surowych danych węzła ani nie zmienia jego praw. Init i enumeracja
mają osobny snapshot errno, bez utożsamiania go z konkretną operacją backendu.

Po poprawnym **rzeczywistym** raporcie callback przekazuje wynik istniejącego
parsera do licznika wejścia aplikacji. Log zawiera `USB input X=1/0`, próbki
surowe, przyciski, analogi, triggery i podgląd mapowania DS4. Pierwsze pięć
naciśnięć X może wywołać powiadomienie „real USB report - X pressed”.
Nie utworzono urządzenia VDA i nie ma generatora raportów w aplikacji PS4.

Eksport `sceKernelDebugOutText` jest dodatkowym kanałem logowania; nie zakładamy,
że GoldHEN klog pokaże ten kanał na każdej konsoli. Plik i powiadomienie są
oddzielnymi dowodami. Powodzenie instalacji lub brak crasha nie dowodzą USB.

## Budowanie i wycofanie

Zobacz `BUILD.md`. Zwykły ELF aplikacji `build/app/dualsense4hen-usb-app.elf`
jest wejściem do narzędzia SELF; **nie wysyłaj go do PayLoadera**. Do instalacji
służy PKG. Identyfikator tytułu: `DSHN00001`, wersja 1.03.

Po zakończeniu usuń własną aplikację z menu PS4 oraz jej PKG z `/data/pkg/`.
Opcjonalnie usuń tylko log tego projektu po jego pobraniu. Nie usuwaj ręcznie
całych katalogów sandboxu ani danych systemowych. Nie dodano AutoRun, pluginu,
patcha jądra lub ustawienia Bluetooth do wycofania.
