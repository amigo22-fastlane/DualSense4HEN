# Wyniki testów

## 2026-10-19 — pierwszy commit

- Host: Windows x86_64, Zig 0.13.0, frontend Clang, C11, `-Wall -Wextra
  -Wpedantic -Werror -g -O1`.
- Polecenie: `./tools/test-host.ps1`.
- Wynik: **PASS: 42402 checks**, 20000 deterministycznych błędnych wejść.
- Testowane: trzy formaty HID, wszystkie długości ucięcia, nadmiarowy bajt,
  CRC każdego bitu pełnego raportu BT (poza report ID), wszystkie kierunki i
  przyciski, signed IMU, dwa sloty touchpada i skalowanie, timeout, monotoniczne
  znaczniki, zawinięcie licznika, błędy sinka, ponowienie usunięcia, kolejność
  VDA init/privilege i odmowa AddDevice po błędzie uprawnień.
- Fixtures są syntetyczne. Żaden test nie potwierdza parowania Bluetooth,
  firmware ABI, instalacji ELF ani działania menu/gier na fizycznej PS4.
- Sandbox Windows blokował atomowe operacje plikowe kompilatora; test
  wykonano poza sandboxem, z cache i TEMP skierowanymi do `build/`.
- CI z ASan/UBSan jest skonfigurowane, ale jego wynik z GitHub nie został
  jeszcze zaobserwowany. Lokalny wynik nie jest wynikiem sanitizerów.

## 2026-10-19 — etap sprzętowy i USB mocks

- Host: **PASS 42403** kontroli rdzenia oraz **PASS USB 497** kontroli
  deskryptorów, ograniczonej pętli odbioru i sprzątania po błędach.
- Kompilacja: trzy samodzielne ELF-y oraz `vda.o`, bez linkowania CRT SDK.
- Walidator ELF: x86_64 ET_DYN, 3 segmenty LOAD, brak importów/interpretera,
  tylko relokacje względne, zapisy do własnego segmentu RW, brak W+X,
  symbole bootstrapu sekcyjne i hidden. Pierwsze błędne symbole ABS są teraz
  wykrywane jako regresja.
- Wyniki fizycznej PS4 opisuje `HARDWARE_RESULTS.md`: działający runtime,
  biblioteki i callback, odrzucona rejestracja urządzenia, brak raportów.
- GitHub Actions pierwszego commita pozostało w stanie `queued` podczas
  odczytu. Nie deklarujemy pozytywnego wyniku CI/ASan/UBSan bez jego uzyskania.

## 2026-10-19 — przygotowanie ponownego pomiaru USB

- `tools/test-host.ps1`: **PASS 42403** rdzenia, **PASS USB 8972** kontroli.
  Dodatkowo: próbkowanie do końca 30-sekundowego okna przy symulowanych
  250 raportach/s, przycisk PS po 20 s widoczny w podglądzie DS4, odłączenie
  po poprawnych raportach zwraca błąd i zwalnia zasoby, błędny raport nie jest
  mapowany. To atrapy API na komputerze, nie dane z konsoli.
- `python -m unittest discover -s tests -p test_probe_log.py -v`: **6 testów OK**.
  Stary `exit=0`, brak nowej sesji, niekompletna sesja, nowa odmowa oraz
  podmieniony/ucięty log nie stają się fałszywym sukcesem nowego pomiaru.
- Zbudowano trzy ELF-y i `vda.o`; struktura trzech dystrybuowanych ELF-ów
  przeszła `check-elf.py`. Wymieniono tylko ELF USB; BT probe i observer
  pozostają bez zmian. SHA-256 rewizji 2 USB:
  `ca51d304e4d128eb8e33135934de9112e77e1a0eeeb1aeeb9eb0e11bc07940b7`.
- `analyze-bt-errors.py` wykonano na wcześniej zatwierdzonym lokalnym segmencie
  libSceBluetoothHid, po sprawdzeniu SHA-256. Odtworzono 12 liczbowych mapowań,
  w tym 0x802f3701 → 0x81540002. To analiza offline, nie nowy eksperyment BT.
- Nowy kabel jest potwierdzony na PC przez właściciela. **Nowy pomiar PS4
  jeszcze nie został wykonany/potwierdzony**. Szczegóły: `USB_RETEST.md`.

## 2026-10-19 — wykonanie ponownego pomiaru i audyt USB

Powyższy wpis opisuje stan przygotowania; następnie właściciel potwierdził
gotowość i wykonano osiem prób opisanych w `HARDWARE_RESULTS.md`.

- Końcowe `tools/test-host.ps1`: **PASS 42403** kontroli rdzenia,
  **PASS USB 10504** standardowej sondy, **PASS USB 10642** wariantu alt 0.
  Testy obejmują pakiet 64 B, natywny błąd i timeout, odczyty kontrolne,
  uszkodzone/niepełne extra HID, brak opcjonalnego eksportu, snapshot errno,
  zwolnienie po nieudanym wyborze alt i jawny drugi argument FreeDeviceList.
- **7 testów Python OK**: rozdzielanie nowej sesji od skumulowanych logów,
  odmowy i niekompletne uruchomienia, rozpoznawanie nowszych rewizji.
- Sześć ELF-ów przeszło walidację: trzy dystrybucyjne, opcjonalny alt 0 oraz
  dwie sondy audytu ABI (BT i USBD). BT probe i observer zachowały poprzednie sumy.
- Końcowy dystrybuowany USB rev 6:
  `5a3fd7ff571de753792ac6a93ea251b12d7ac709989e932658e374a17beaf88d`.
  Wykonano go na konsoli; log: enumeracja i deskryptor HID OK, brak raportów
  wejścia, zwolnienie interfejsu=0, exit=-6. To częściowa diagnostyka sprzętowa,
  nie pozytywny test sterownika do grania.
- Odczyt biblioteki USBD wykonano po osobnej zgodzie; RX/R zostały lokalnie,
  RW nie odczytano, oba tymczasowe pliki usunięto z konsoli. Nie publikujemy
  kodu Sony ani deasemblacji. Nie wykonano patchy jądra i nie włączono VDA.

## 2026-10-19 — aplikacja i pasywny observer

- Rdzeń C: **42403 kontroli PASS**; standard USB: **10911**, alt 0: **11058**,
  audyt dostępu USB: **11233** (w tym odmowa open i odtworzenie errno).
  Dodano testy callbacku dopiero po poprawnym raporcie i logowania zboczy X.
- BT mock: **92 kontrole** wariantu rejestracji i **382** wariantu pasywnego.
  Sprawdzają odmowę wspólnego kontekstu i niezgodnego profilu, Init/callback
  error, pin modułu, unregister+join, limit 30 sekund/stop oraz brak wywołania
  RegisterDevice w wariancie pasywnym. Nie są testem parowania ani sterownika.
- Python: **13 testów OK**, w tym regresja pierwszego błędnego identyfikatora
  PKG i spójność metadanych. CI rozszerzono o testy Python i mocki BT; nowego
  wyniku GitHub Actions nie przypisujemy do lokalnych testów.
- Zbudowano aplikację OpenOrbis 1.03, SELF i PKG. PkgTool potwierdził hashe
  i podpisy, własny walidator potwierdził spójność TITLE_ID/CONTENT_ID w SFO,
  GP4 i nagłówku PKG. Właściciel potwierdził później instalację i pomiar 1.03;
  wynik nie potwierdza wejścia HID.
- Cztery nowe ELF-y (probe, USB, observer i passive) przeszły check-elf.py;
  następnie osobno sprawdzono wariant USB access przed uruchomieniem.
  Dotychczasowe trzy pliki dist/diagnostics zachowano jako zidentyfikowane
  sprzętowo wersje; nowy passive jest osobnym, jeszcze nietestowanym na PS4
  artefaktem. Zmiany runtime we wspólnym źródle dają nowe hashe przy przebudowie.
- Sprzęt: 1.02 dostarczyła log: Init=0, lista pusta, exit=-22. Porównawczy
  PayLoader nadal widzi 054c:0ce6 i deskryptor, lecz nie odbiera raportów.
  Audyt open O_RDONLY /dev/usbctl działa w obu kontekstach. Wersja 1.03 nadal
  ma count=0, errno=1 po enumeracji. Nie potwierdzono rzeczywistego naciśnięcia X.

## 2026-10-19 — niezależna gałąź architektury od 13aa707

- `tools/test-host.ps1`: rdzeń 42403, nowy transport research **21289**,
  BT 92/382, USB 10911/11058/11233 kontroli PASS.
- Python: 13 testów OK. W tej sesji użyto ścieżki PowerShell/Zig; CMake nie
  jest dostępny lokalnie. Cel CMake/CTest rozszerzono o nowy test; nie nazywamy
  go lokalnie wykonanym CTest lub testem sanitizerów.
- Nowy test obejmuje dokładne trzy kontrakty socketów, fd=0, brak kontroli
  pozytywnej, odmowę uprawnień i nieudany close, bez nieograniczonych prób.
- ACL/HIDP: pełna i każda fragmentacja raportu syntetycznego, kontynuacje,
  handle/CID/PSM, CRC, granice bufora i 10000 uszkodzonych ramek. Bez realnego HCI.
- Cross-kompilacja: ELF native-bt-gate oraz obiekty ACL i parsera dla PS4.
  ELF przeszedł check-elf.py: ET_DYN, 3 LOAD, brak importów i W+X.
- **Pomiar PS4**: firmware 0x12520001, socket IPv4 fd=25/close=0,
  HCI i L2CAP fd=-1/errno=43; complete=0, exit=0. Nie wykonywano pairing,
  transmisji, USB, VDA, prywatnych ioctl lub patchy. To wąski negatywny wynik
  kontraktu socketów FreeBSD w PayLoaderze, nie dowód braku natywnego BT.
- Szczegóły i instrukcja: `research/NATIVE_BT_GATE.md`. Nowe wyniki są oddzielone
  od bazy; nie zmieniono wcześniej dystrybuowanych ELF/PKG.
- Po wysłaniu kodu zaobserwowano **GitHub Actions success** dla commita
  `7259532d6964bf628f3dc8fa3511004ba3d7ed67`:
  [run 39330057806](https://github.com/amigo22-fastlane/DualSense4HEN/actions/runs/39330057806).
  Workflow wykonuje CMake/CTest z ASan/UBSan i testy Python. Obejmuje rdzeń,
  transport research oraz mocki BT; opcjonalne mocki USB uruchomiono lokalnie,
  ponieważ CI nie otrzymuje nagłówka SDK. To test hosta, nie test radia PS4.
