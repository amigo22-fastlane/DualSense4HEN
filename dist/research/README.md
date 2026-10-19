# Niezależne eksperymenty architektury

`dualsense4hen-native-bt-gate.elf` sprawdza trzy jawne kontrakty socketów,
bez USB, rejestracji HID, VDA, pairing lub transmisji. Payload 12.52; nie AutoRun.

Pomiar PS4: kontrola IPv4 działa, HCI/L2CAP według FreeBSD zwracają errno=43.
Nie jest to sterownik ani dowód działania DualSense.

[Instrukcja i wynik](../../docs/research/NATIVE_BT_GATE.md).
[Porównanie architektur](../../docs/research/ARCHITECTURE_REVIEW.md).
GPL-3.0-only; zewnętrzne nagłówki: ../../THIRD_PARTY.md.
