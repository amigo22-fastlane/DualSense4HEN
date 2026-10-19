# Diagnostyka 0.1.0

To pliki **naszych** payloadów; nie zawierają binariów systemowych Sony.
GPL-3.0-only, patrz `../../LICENSE` i `../../THIRD_PARTY.md`.

- `dualsense4hen-bt-probe.elf`: wersja firmware i obecność eksportów, bez Init BT/VDA.
- `dualsense4hen-usb-probe.elf`: rewizja 6; jednorazowa enumeracja, standardowe deskryptory, maks. 30 s / 30000 odczytów HID, parser i podgląd mapowania DS4 bez wywołań VDA. Do 120 próbek przez całe okno. Odczyt deskryptora HID i enumeracja potwierdzone w logach konsoli; odbiór wejścia nadal kończy się błędem. Zawiera odczyty kontrolne HID i snapshot errno.
- `dualsense4hen-bt-observer.elf`: eksperymentalny callback HID z profilem biblioteki 12.52. Rejestracja Sony zwróciła na sprzęcie 0x81540002. Nie zapewnia parowania ani gry.

Sprawdź `SHA256SUMS.txt`. Budowanie: `../../docs/BUILD.md`.
Uruchamianie i wycofanie: `../../docs/INSTALL_ROLLBACK.md`.
Nie używać AutoRun. Po observerze kontekst biblioteki pozostaje w RAM do restartu.
Ponowny test USB: `../../docs/USB_RETEST.md`.
