# Pochodzenie i licencje

Projekt: GPL-3.0-only. Pełny tekst: `LICENSE`.

`include/ds4hen/ps4_pad.h` i `platform/ps4/vda.c` adaptują układ danych,
parametry i kolejność wywołań z [Control4Free](https://github.com/MoHadiShibli/Control4Free),
Copyright © 2026 MoHadiShibli, GPL-3.0. Autorzy Control4Free wskazują
[SplashDown](https://github.com/seregonwar/SplashDown), seregonwar, GPL-3.0,
jako źródło mechanizmu VDA. Zachowujemy te informacje i pełną licencję w
`third_party/licenses/Control4Free-GPL-3.0.txt`. Nie kopiujemy klienta WWW,
fontów, grafiki ani serwera Control4Free.

Parser napisano niezależnie na podstawie formatów protokołu: deskryptorów
[nondebug/dualsense](https://github.com/nondebug/dualsense) i struktur
[hid-playstation](https://github.com/torvalds/linux/blob/master/drivers/hid/hid-playstation.c).
Repozytorium nondebug nie zawiera licencji: nie dołączamy jego HTML/JS ani
deskryptorów. Nie przenosimy kodu sterownika Linux ani linuksowych interfejsów.

Pozostałe repozytoria w `research/` to wyłącznie lokalny materiał audytu,
wyłączony z Gita. Szczegóły, wersje i ograniczenia: `docs/AUDIT.md` i
`docs/sources.lock.json`. SDK i kompilatory pozostają zewnętrznymi narzędziami;
ich dystrybucja wymaga zachowania własnych licencji.

Sondy używają nagłówka deskryptorów `libusb.h` z dystrybucji PS4 payload SDK
v0.9 (FreeBSD, BSD-2-Clause, Sylvestre Gallon). Informacja o licencji jest
zachowana w `third_party/licenses/libusb-BSD.txt`. Nie linkujemy libusb z PC.
Payloady nie linkują CRT payload SDK, który wykonuje automatyczne patche mmap
i zmiany jail. Własny runtime payloadu korzysta ze sprawdzonych syscalli PS4.

Samodzielna aplikacja jest osobnym celem: linkuje zwykły CRT i statyczną libc
z oficjalnego OpenOrbis v0.5.4, po audycie startupu OpenOrbis/musl. Nie wykonuje
helperów jailbreak ani zmian poświadczeń. Zachowane pełne informacje licencyjne:
`third_party/licenses/OpenOrbis-musl-COPYRIGHT.txt`. Linkowany compiler-rt ma
Apache-2.0 z LLVM exception: `third_party/licenses/LLVM-compiler-rt.txt`.
PKG zawiera również te teksty, GPL projektu i `platform/ps4/app/NOTICE.txt`.
Dołączone libc.prx i libSceFios2.prx to puste stuby z dystrybucji OpenOrbis,
nie moduły skopiowane z konsoli. Narzędzie LibOrbisPkg (LGPL-3.0) pozostaje
zewnętrznym narzędziem, bez kopiowania jego implementacji do projektu.

Ustalenia ABI biblioteki Sony pochodzą z lokalnego odczytu modułu własnej konsoli,
zatwierdzonego wyraźnie przez właściciela. Kod/binarne segmenty Sony, pełne logi
systemowe i disassembly **nie są dystrybuowane**. `bt_observer.c` jest własną
implementacją klienta tego interfejsu; profil zawiera identyfikator wersji kodu
i udokumentowany slot danych używany wyłącznie do odczytu uchwytu wątku.
