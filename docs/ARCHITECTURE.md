# Architektura

```text
wewnętrzny BT PS4 [ABI/parowanie jeszcze niezweryfikowane]
              albo USB [tylko eksperyment odbioru]
                       |
           raport HID z report ID, bez HIDP a1
                       |
       d4h_parse -> d4h_state -> d4h_map_ps4
                       |
       d4h_vpad (własność, zegar, watchdog, stop)
                       |
       d4h_vda_backend -> scePadVirtualDeviceInsertData
                       |
                    MBus -> menu i gry [do testów sprzętowych]
```

Wtyczki GoldHEN z analizowanych projektów działają w procesie gry.
Globalne urządzenie wymaga payloadu i VDA. Żadna warstwa parsera nie zna
Bluetooth API Orbis, socketów Linux, FreeBSD netgraph ani urządzeń `/dev/hidraw`.

Interfejs backendu VDA pozwala uruchomić identyczną logikę z testowym sinkiem.
Warstwa PS4 wymaga wcześniej załadowanego MBus, kompletnej tablicy rzeczywistych
funkcji, prawidłowych uprawnień i identyfikatora własnego urządzenia.
`AddDevice` zwraca status, **nie handle**. Sam numer z obcego logu nie dowodzi
własności. Pierwszy prototyp nie tworzy automatycznie urządzeń na konsoli.

Na wejściu błędny raport nie zmienia poprzedniego stanu. Warstwa czasu nie
odświeża watchdogu dla błędnych raportów. Po utracie danych trzeba wywołać
watchdog (zalecane 500 ms) i `stop` przy odłączeniu. Neutralizacja poprzedza
usunięcie urządzenia; nieudane usunięcie zachowuje własność do ponowienia.
Zegar musi być monotoniczny w mikrosekundach. Powrót zegara neutralizuje pad.

Create → Share, Options → Options, PS → bit 0x10000, klik touchpada →
0x100000. Kierunki i cyfrowe L2/R2 pochodzą z bitów HID, analogowe triggery
pozostają niezależne. Pozycje touchpada 1920×1080 skalujemy do 1920×942 DS4.
Nieaktywnych kontaktów nie przekazujemy. Mute nie ma odpowiednika DS4.
Surowe IMU jest parsowane do badań, ale nie jest przekazywane bez kalibracji.
Brak jeszcze rumble, audio, adaptive triggers, orientacji i wybudzania konsoli.
