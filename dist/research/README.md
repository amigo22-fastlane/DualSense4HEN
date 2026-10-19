# Niezależne eksperymenty architektury

`dualsense4hen-native-bt-gate.elf` sprawdza trzy jawne kontrakty socketów,
bez USB, rejestracji HID, VDA, pairing lub transmisji. Payload 12.52; nie AutoRun.

Pomiar PS4: kontrola IPv4 działa, HCI/L2CAP według FreeBSD zwracają errno=43.
Nie jest to sterownik ani dowód działania DualSense.

[Instrukcja i wynik](../../docs/research/NATIVE_BT_GATE.md).
[Porównanie architektur](../../docs/research/ARCHITECTURE_REVIEW.md).
GPL-3.0-only; zewnętrzne nagłówki: ../../THIRD_PARTY.md.

`dualsense4hen-connected-bt.elf` rev2: 35 s subskrypcji istniejącej usługi
BluetoothHid i widoku scePad, bez RegisterDevice/USB/VDA. Nie uruchamiać
ponownie w aktualnym gospodarzu — moduł observera rev1 pozostaje obecny.
Rev1 wykonana: zero zdarzeń; rev2 z poprawką UserService nie testowana sprzętowo.

`dualsense4hen-pad-view.elf` rev2: pomiar scePad bez subskrypcji Bluetooth.
Wykonana na PS4: odczyty działają, lecz kontrola DS4 L2 nie dała wejścia.
Nie potwierdza ani nie wyklucza rejestracji DualSense.

`dualsense4hen-hidcontrol-abi.elf`: przygotowany audyt RX/R innej biblioteki
Sony. **Nie uruchomiono, wymaga odrębnej zgody na lokalne skopiowanie kodu.**
Nie jest zwykłym observerem wejścia. Nie wywołuje funkcji HID Control.

[Istniejąca sesja, wyniki i instrukcje](../../docs/research/EXISTING_BT_SESSION.md).
[Dokładny zakres audytu](../../docs/research/HID_CONTROL_ABI_EXPERIMENT.md).
