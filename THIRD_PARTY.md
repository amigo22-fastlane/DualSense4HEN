# Pochodzenie i licencje

Projekt: GPL-3.0-or-later. Pełny tekst: `LICENSE`.

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
