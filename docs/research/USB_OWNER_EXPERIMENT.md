# USB-OWNER — zweryfikowana procedura, jeszcze bez wykonania na PS4

2026-10-30. Bluetooth pozostaje priorytetem. Nie uruchamiać teraz tej sondy,
nie podłączać USB i nie zmieniać transportu bieżącej sesji.

- **Hipoteza:** do HID interface 3 DualSense jest przypisany systemowy sterownik;
  może to współwystępować z odmową otwarcia endpointu IN 0x84.
- **Dowód:** bieżące VID/PID na tym samym fd, surowy wynik ACTIVE, wynik i
  nazwa z GET_IFACE_DRIVER, errno każdego wywołania oraz zamknięcie fd.
- **Sukces:** udane odczyty i nazwa sterownika wskażą przypisanie w tym momencie.
  **Nawet uhid nie dowodzi przyczyny 0x802400ff.**
- **Niepowodzenie:** odmowa O_RDONLY/ioctl lub brak enumeracji nie dowodzą
  wolnego interfejsu. Udana pusta nazwa osłabi hipotezę przypisania; nie
  rozstrzygnie wcześniejszego błędu transferu.
- **Wpływ na architekturę:** przypisanie sterownika przemawia za obserwacją
  istniejącej ścieżki HID. Brak przypisania przesuwa badanie na backend
  FS/endpoint i kontekst procesu; sam nie uzasadnia nowego sterownika.
- **Odczytowość:** O_RDONLY i tylko trzy query; bez libusb Open/Claim,
  transferów, FS_INIT/OPEN, detach, resetu, zasilania, AuthID lub patchy.
  Init/list/free/Exit i zwykły loader dotyczą własnego kontekstu biblioteki.
- **Ryzyko:** prywatne ioctl nadal uruchamiają kod sterownika. Zgodność
  opakowania i numerów została sprawdzona, ale brak testu na sprzęcie;
  możliwa odmowa RO, zmiana numeracji węzła lub nieprzewidziany błąd.
  Samo fizyczne podłączenie USB może zmienić transport, dlatego osobne okno.

## Audyt kodu i ABI

Źródła: `src/usb_owner.c`, `platform/ps4/usb_owner_probe.c`,
`include/ds4hen/usb_owner.h`. Zatwierdzony wcześniej RX libSceUsbd 12.52:
SHA256 `dc8aa9f70baa6f6e32d1ea938cdd51761f6601aa4c7e63ff11f6509b848b5dff`.

1. Firmware guard 12.52 oraz fingerprint RX przed użyciem profilu. GetBusNumber
   i GetDeviceAddress mają oczekiwane entrypoints +0x420/+0x430. Nie odczytujemy
   zgadywanych pól urządzenia. Przy obcym profilu lub rezydującym Usbd odmowa.
2. Węzeł jest wyliczany z aktualnie enumerowanego 054c:0ce6. Device Descriptor
   `0x40125569` odczytuje 18 B **na otwartym fd**, następnie sprawdzany jest
   typ/długość/VID/PID. Nie wyszukujemy przypadkowych węzłów.
3. ACTIVE: `0x8004557c`, 4-bajtowy indeks 3; kod biblioteki +0x92a0 używa tego
   opakowania. Logujemy raw return; nie opisujemy `0` jako linuksowego
   boolean „nieaktywny”. Sam native wrapper nie rozstrzyga wszystkich stanów.
4. GET_IFACE_DRIVER: `0xc0205579`; native +0x8610 używa 32-bajtowego bloku:
   pointer +0, uint16 max_length +10, uint8 interface +18. Sonda zapewnia
   128 B własnego bufora nazwy i log ograniczony/drukowalny.
5. Ponownie sprawdzono różne fd backendu: +0x6c dla NAME i +0x70 dla ACTIVE.
   Kod +0x8bef i +0x8c10 otwiera **tę samą zbudowaną ścieżkę** dwa razy,
   w natywnym O_RDWR=2; nie są to dwa inne urządzenia. Nasz pojedynczy nowy
   O_RDONLY nie odtwarza FS_INIT i może dostać odmowę. Nie przechodzimy wtedy
   automatycznie na O_RDWR i nie zmieniamy praw.
6. Po każdym open jest close, także przy złej tożsamości i błędach query.
   Lista zwalniana przez FreeDeviceList(list,1), Exit ma void. Runtime zwalnia
   własne moduły; cudzych nie odładowuje. Nie uruchamia VDA ani Bluetooth.

Numery 144/145 z raportu Deep Research nadal są zabronionym, niezweryfikowanym
zamiennikiem. W tej sondzie nie ma żadnej operacji DETACH ani FS.

## Plik i przyszły test po osobnej zgodzie

`dist/research/dualsense4hen-usb-owner.elf`
SHA256 **`2b1c221e8577db8deb311c39addae7e1994aac8f2af4ed2ea8e09dacf4c5b212`**.
Plik nie zmieniony. Ponowna kontrola strukturalna: ELF64 x86_64 ET_DYN,
bez importów i W+X. Numery query ponownie odtworzono z lokalnego RX przez
`tools/research_abi.py`. Nie powtarzano prób odbioru ani nie wysłano ELF.

Przyszła procedura:

1. Najpierw osobno uzgodnić okno USB i zapisać stan/koniec pomiarów Bluetooth.
   Nie usuwać wpisu parowania. Zachować działający DS4 do obsługi menu.
2. Dopiero w tym oknie podłączyć DualSense potwierdzonym kablem danych.
   Udostępnić FTP/PayLoader; sprawdzić bieżący IP, zamiast zakładać dawny.
3. Wysłać **wyłącznie** powyższy ELF przez `console-probe.py`, z osobnym
   lokalnym logiem i bazową długością logu. Niczego nie instalować/AutoRun.
4. Wymagać nowych wpisów `USB owner revision=1`, `identified`, osobnych
   statusów identity/ACTIVE/NAME oraz complete i exit. Sam TCP send lub stary
   sukces w logu nie potwierdzają wykonania. Przy błędzie zachować raw/errno.
5. Nie naciskać X dla tej sondy: nie odbiera raportów. Nie wykonywać potem
   automatycznie interrupt, GET_REPORT lub detach. Omówić wynik osobno.

Do wykazania przyczyny 0x802400ff potrzebny będzie dopiero **pierwszy błędny
wynik konkretnej operacji backendu**, nie ostatnie errno po całym API.
Taki eksperyment nie jest objęty niniejszą procedurą.
