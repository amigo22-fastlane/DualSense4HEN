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
