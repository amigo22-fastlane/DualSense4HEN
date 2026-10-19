# Lokalny audyt libSceHidControl — przygotowany, nieuruchomiony

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
Nie uruchomiono na konsoli. Poprzednie zgody obejmowały BluetoothHid i Usbd;
ten zakres dotyczy **innej biblioteki** i wymaga osobnego zatwierdzenia
lokalnego odczytu jej kodu Sony przez właściciela.

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
