# Lokalny audyt libSceHidControl — wykonany po odrębnej zgodzie

Powód: połączony w UI DualSense nie daje zdarzeń nowemu klientowi BluetoothHid,
a kontrola DS4 przez scePad w PayLoaderze też zwraca zera. Powtarzanie tych
odczytów nie ustali stanu sesji. Publiczny OpenOrbis HidControl.h wymienia
GetDeviceInfo, GetDeviceName i GetBatteryState, lecz ma deklaracje bez
argumentów: **nie stanowią ustalonego ABI**. Nie wywołujemy ich z domysłami.

Hipoteza: GetDeviceInfo używa tego samego kontraktu informacji o urządzeniu
co BluetoothHid lub ujawnia granicę klasyfikacji urządzeń w natywnej usłudze.
Sukces audytu to ustalenie pól wejściowych, rozmiaru i struktury wyniku,
przepływu do sterownika oraz dowodu, że zapytanie jest obserwacyjne. Jeżeli
wrapper tylko przekazuje nieznany bufor dalej, audyt jest niewystarczający:
nie wykonamy zapytania, dopóki nie znajdziemy jego konsumenta lub pełnego ABI.
Nie obiecujemy, że ten moduł udostępni CID lub liczniki pakietów L2CAP.

Przygotowany plik: `build/research/dualsense4hen-hidcontrol-abi.elf`, kopia
w `dist/research/`. Budowanie: `powershell -File tools/build-research.ps1 -HidControlAudit`.
Przed uruchomieniem uzyskano osobną zgodę właściciela na lokalny odczyt
RX/R tej biblioteki. Poprzednie zgody obejmowały BluetoothHid i Usbd.

SHA256 przygotowanego ELF:
`9c83f379fc33b6b287c2421f90ed0bfa01d59956f9b7520143e14e4098d99d5b`.
Sprawdzony strukturalnie: ELF64 x86_64 ET_DYN, brak importów i segmentów W+X.

Zakres po zgodzie:

1. Firmware guard 12.52, zwykłe załadowanie/pożyczenie wyłącznie
   `/system/common/lib/libSceHidControl.sprx`. Bez wywołania jakiejkolwiek
   funkcji HID Control, w tym DisconnectPort, SetApplicationFocus i SetUserColor.
   Standardowy loader uruchamia normalny cykl start/stop modułu; nie podmieniamy go.
2. sceKernelGetModuleInfo, najwyżej cztery segmenty do 4 MiB każdy. Wyłącznie
   odczytywalne segmenty bez prawa zapisu (RX/R). RW, inne biblioteki i jądro
   nie są kopiowane. Logowane są względne adresy trzech eksportów Get*.
3. Własne pliki `/data/dualsense4hen-hidcontrol-segN.bin`, mode 0600, O_EXCL:
   istniejący plik nie zostanie nadpisany. Niepełny zapis, błąd open/close
   albo brak segmentów dają ujemny wynik sondy. Weryfikujemy również pełne
   rozmiary i hashe pobranych plików, nie tylko exit.
4. FTP do ignorowanego `build/hardware/`, sprawdzenie długości i SHA256,
   analiza tylko lokalnie, następnie usunięcie dokładnie utworzonych plików
   z konsoli po potwierdzonym pobraniu. Bez zmiany praw plików lub urządzeń.
5. Do Git trafiają własne wnioski i kod; brak segmentów Sony, bajtów kodu,
   deasemblacji i pełnego klog. Własny moduł jest zwalniany zwykłym loaderem;
   pożyczony pozostaje w gospodarzu. Nie czytamy pamięci systemowych procesów.

Nie zmienia parowania, konfiguracji urządzenia, authid, przywilejów, jądra,
firmware, AutoRun ani plugins.ini. Nie przejmuje radia i nie wysyła komend
HCI/L2CAP. Nie trzeba naciskać przycisków kontrolerów ani rozłączać sesji.

Testy na atrapach sprawdzają whitelistę modułu/eksportów, pomijanie RW,
wyłączny zapis nowych plików, odmowę przy innym firmware i stop marker.
Walidacja ELF nie jest potwierdzeniem uruchomienia na PS4.

Źródło nazw eksportów:
[OpenOrbis HidControl.h](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/include/orbis/HidControl.h).

## Wynik zatwierdzonego uruchomienia

2026-10-20 00:14:35 czasu Polski (22:14:35 UTC poprzedniego dnia): właściciel
udzielił osobnej zgody. Sonda wystartowała na 0x12520001; trzy wskazane
eksporty obecne, nie wywołane. Zapisano segment 0 RX=16384 B i segment 2
R=16384 B, pominięto segment 1 RW. Własny moduł unload=0, exit=0.
Pobrano oba pliki, sprawdzono rozmiary/kopie/SHA256, po czym usunięto dokładnie
te dwa pliki z PS4, bez chmod. Kopie pozostają w ignorowanym build/hardware.

- RX: SHA256 `90a1f20a98572dd0838eba12c8ee2b6560c4de749570e9f475f9c2a056312c17`.
- R: SHA256 `3a4d72847df5f8d367f8306e9205897a78b19088b2878c96a31d2980b5bd0fa8`;
  dane po relokacji, nie uniwersalny fingerprint modułu.
- Log: SHA256 `3f236114d52275659d03e2b4d6acf765cd4422ad50436359e9e29bc6e2fcf1e0`.

Ustalenia własnej analizy, bez dystrybucji kodu Sony:

1. HidControl posługuje się /dev/hid; GetDeviceInfo z BluetoothHid otwiera
   /dev/bt. Watcher callbacków BluetoothHid używa z kolei /dev/bluetooth_hid.
   To różne kontrakty; nie utożsamiamy ich z surowym HCI/L2CAP.
2. GetDeviceInfo przekazuje 32-bitowy pierwszy argument i wskaźnik w 16 B
   opakowaniu do żądania 0x80104801. GetDeviceName ma analogiczny układ
   i żądanie 0x8010484b. GetBatteryState dodaje pole o wartości 1 i używa
   0x8010480c. Funkcje sprawdzają inicjalizację i niepusty wskaźnik.
3. Kod wrappera nie ujawnia rozmiaru obiektu wskazywanego przez ten wskaźnik.
   **16 B nie jest rozmiarem wyniku GetDeviceInfo.** Pierwszy argument nie
   został jeszcze semantycznie zidentyfikowany. Nie wywołano żadnego z Get*.
4. Nie znaleziono w tym kontrakcie udokumentowanej listy CID, HCI lub odbioru
   pakietów. Sam audyt nie rozstrzygnął kanałów istniejącej sesji DualSense.
   Do dalszego zapytania potrzebny jest rzeczywisty konsument/struktury.

Reprodukcja liczb bez disassembly:
`python tools/research_abi.py hidcontrol build/hardware/hidcontrol-seg0.bin`.
Program weryfikuje pełny hash i długość przed użyciem własnego profilu.
Żadnego z opisanych prywatnych ioctl nie wykonano bezpośrednio.
