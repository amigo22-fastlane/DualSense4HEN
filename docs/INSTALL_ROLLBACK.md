# Uruchamianie diagnostyki i wycofanie

To pakiet badawczy dla PS4 **12.52**, nie gotowy sterownik do grania.
Nie umieszczaj sond w `plugins.ini`, `payloads.ini` ani AutoRun. Sonda zgłasza
wersję przez systemowy eksport i odmawia działań na innym numerze firmware.
GoldHEN może mieć opcje spoofingu — zanotuj także wersję z ustawień konsoli.

## Diagnostyka

1. Zbuduj i sprawdź ELF według `BUILD.md` albo porównaj sumy z `dist/diagnostics/SHA256SUMS.txt`.
2. Zostaw DualShock 4 jako kontroler pomocniczy. Zakończ grę przed testem.
3. Włącz GoldHEN PayLoader Server (domyślnie 9090) i FTP (2121).
4. Uruchom `python tools/console-probe.py --host ADRES_PS4 --elf dist/diagnostics/dualsense4hen-bt-probe.elf`.
5. Odczytaj `/data/dualsense4hen-probe.log`; narzędzie zapisuje lokalną kopię.
   Sam sukces TCP nie dowodzi uruchomienia, a stary wpis w kumulowanym logu
   nie dowodzi nowego testu. Sprawdź świeży nagłówek, firmware i zakończenie.

Komputer służy tu do **budowania i dostarczenia prototypu**, nie jest mostem
wejścia i nie uczestniczy w transmisji kontrolera. Docelowa wersja ma działać
samodzielnie, lecz taka wersja nie jest jeszcze gotowa.

Jeśli nie powstaje log, włącz serwer klog (3232), uruchom
`tools/capture-klog.py --host ADRES_PS4 --seconds 35` i dopiero wtedy wyślij ELF.
Pełny klog może zawierać dane innych usług; zostaje lokalnie w `build/`.

USB probe wymaga **DualSense z PS5**, kabla danych oraz bezpośredniego portu
konsoli. Skanuje deskryptory i tylko interfejs HID; nie używa endpointu Xbox,
nie odłącza sterownika systemowego, nie resetuje urządzenia i nie zmienia
konfiguracji USB. Po znalezieniu pada odbiera przez maks. 30 s i zwalnia interfejs.
Po zmianie kabla wykryto 054c:0ce6 i odczytano deskryptor HID; transfer
wejściowy zwraca 0x802400ff. To nie jest gotowe wejście USB.

Samodzielna aplikacja PKG i dokładna procedura naciskania X **na padzie PS5**:
[USB_APP.md](USB_APP.md). Nie wysyłaj jej ELF przez PayLoadera.

Observer Bluetooth ma dodatkowe warunki z `BLUETOOTH_ABI_1252.md`: zgodny hash
biblioteki, własny kontekst, jeden eksperyment na restart konsoli. Rejestracja
urządzenia w wykonanym teście zwróciła 0x81540002. Nie omijać tego błędu patchami.

## Zatrzymanie i rollback

`python tools/console-probe.py --host ADRES_PS4 --stop` tworzy wyłącznie pusty
`/data/dualsense4hen.stop`. Pętle testowe kończą się po najbliższym ograniczonym
odczycie. Sonda eksportów kończy się od razu. Observer usuwa własną rejestrację,
wyrejestrowuje callback i wykonuje join. Przy nieudowodnionym zakończeniu
wątku zatrzymuje wyjście payloadu; wtedy ręcznie zrestartuj konsolę.

Po zakończeniu testu usuń przez FTP tylko pliki tego projektu:

- `/data/dualsense4hen.stop` — przed świadomym następnym testem;
- `/data/dualsense4hen-probe.log` i `/data/dualsense4hen-probe.lock`;
- opcjonalne `/data/dualsense4hen-bt-seg0.bin`, `...seg1.bin`, `...seg2.bin`
  utworzone wyłącznie przez zatwierdzoną sondę ABI.

Plik lock może istnieć bez aktywnego procesu; o blokadzie decyduje flock,
nie obecność pliku. Nie usuwaj go w trakcie aktywnego testu. Nie kasuj plików
GoldHEN ani innych payloadów. Zwykły restart konsoli usuwa pozostały kontekst
HID z pamięci. Nie ma persistent patcha, konfiguracji startowej ani modułu
jądra do odinstalowania. Nie należy reinstalować ani aktualizować firmware.
