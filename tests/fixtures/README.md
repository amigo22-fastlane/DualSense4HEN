# Wektory syntetyczne

Te pliki nie pochodzą z konsoli ani fizycznego DualSense. Zawierają report ID,
bez bajtu HIDP `a1`. `tools/make-fixtures.py` generuje je deterministycznie;
CRC Bluetooth oblicza niezależnie biblioteka Python `zlib`.

`*-controls` i `bt-extended`: Cross, kierunek NE, Create/Share, Options, PS,
klik touchpada, analogi 0/255/64/192 i triggery 17/254. Pełne raporty zawierają
także mute, surowe IMU i kontakt 7 w prawym dolnym rogu. Minimalny BT nie ma
pozycji touchpada ani IMU. Potrzebne są później osobne próbki sprzętowe.
