# VDA na PS4 12.52 — plan, bez uruchomienia

2026-10-30. Ten dokument nie jest zgodą na wykonanie. Nie wywołano VDA,
SetProcessPrivilege, zmian AuthID/caps ani nowych operacji na konsoli.

## Co wiemy

Dotychczasowy pomiar wykazał obecność eksportów Init, SetProcessPrivilege,
AddDevice, InsertData i DeleteDevice. To nie dowód, że ich wywołanie się
powiedzie w tym gospodarzu ani że wejście trafi do menu/gier.
ABI AddDevice (32 B) i InsertData (120 B) pochodzi z audytu
[Control4Free](https://github.com/MoHadiShibli/Control4Free/blob/5a8f9e3d0e75378644e7f7b2df6176c386a24da9/src/vda.c),
nie z audytu biblioteki Pad tej kompilacji. AddDevice zwraca status, nie ID.
Kod referencyjny odzyskuje DeviceId z klog; nie jest to dowolny świeży ID.

**Istniejący `d4h_vda_init()` wywołuje privilege(1).** Nie należy używać go
w eksperymencie „bez zmiany przywilejów”. Dopisano to przy jego deklaracji;
zachowania kodu nie zmieniono. Nie zaimplementowano ani nie uruchomiono
nowego payloadu VDA. Referencyjne zmiany AuthID/caps pozostają poza zakresem.
GetRemoteSetting ma niepotwierdzony rozmiar wyniku; nie włączamy go do testu.

## Warunki przed stworzeniem choć jednego urządzenia

1. Osobna zgoda właściciela na konkretny ELF i listę wywołań. Powinna jasno
   rozdzielić próbę przy obecnych uprawnieniach od ewentualnej późniejszej
   zmiany SetProcessPrivilege. Brak zgody nie może uruchamiać fallbacku.
2. Potwierdzenie pełnego ABI i profilu libScePad 12.52 dla wybranych operacji.
   Jeśli potrzebny jest nowy audyt RX/R, najpierw osobna zgoda na ten moduł.
   Nie kopiujemy rozmiarów wyjściowych nieznanych Get* ani offsetów z firmware obcego.
3. Działający log, sprawdzona metoda jednoznacznego odzyskania **własnego**
   ID i zweryfikowane DeleteDevice. Nie wykonywać AddDevice, jeśli nie ma
   realnego sposobu ustalenia własności oraz usunięcia stworzonego urządzenia.
4. Klog może mieć wyłącznego czytelnika: nie konkurować z aktywnym klientem
   GoldHEN. Nie czytać /dev/mbus. Potrzebne marker, bazowy zakres logu,
   ograniczony czas i brak równoległych VDA/Remote Play/zmian urządzeń.
   Jeśli sam klog nie wystarcza do pewnej własności, etap Add pozostaje BLOCKED.
5. Użytkownik zna możliwe okno wyboru użytkownika i pozostaje przy konsoli
   ze sprawnym DS4. Nie zgadywać userId=1 jako zwykłego ID konta; znaczenie
   pola trzeba potwierdzić. Nie kończyć pożyczonego UserService/Pad.

## Proponowane oddzielne etapy i operacje

| Etap | Lista potrzebnych operacji po zatwierdzeniu | Kryterium wyniku |
|---|---|---|
| VDA-0: przygotowanie | Kontrola firmware/profilu, zwykły loader Mbus przed Pad, rozwiązanie konkretnych eksportów, odczyt istniejącego zalogowanego użytkownika zgodnie ze znanym kontraktem | Brak eksportów/profilu/ABI → stop przed Init/Add |
| VDA-1: obecne uprawnienia | scePadInit według własności kontekstu; **bez SetProcessPrivilege**; najwyżej jedno AddDevice, tylko po spełnieniu warunków własności i cleanup | Odmowa to wynik kontekstu, nie powód do AuthID/caps lub automatycznej eskalacji. Niezerowy status sam nie dowodzi, że nic nie powstało |
| VDA-2: cykl życia | Przy potwierdzonym własnym ID: ograniczona seria neutralnych InsertData; potem neutralny stan i DeleteDevice dokładnie tego ID, zamknięcie własnych zasobów | Potwierdzone utworzenie/usunięcie bez wpływu na DS4; nie jest jeszcze dowodem działania przycisków |
| VDA-3: wejście testowe | Dopiero po zgodzie na wskazaną akcję: jeden krótki neutral → D-pad → neutral na znanym bezpiecznym ekranie, z użytkownikiem obserwującym menu | Ruch w menu dowodzi ścieżki wirtualnego wejścia; dane jawnie oznaczone **synthetic/VDA-only** |
| VDA-4: realny kontroler | Zamiast generatora dane tylko z potwierdzonego odbioru i parsera DualSense, z identyfikacją źródła | Osobne potwierdzenie menu oraz gry; dopiero to test integracji |

Nie przygotowujemy testu polegającego na kasowaniu 0–63, skanowaniu handle,
adoptowaniu innego DeviceId, nieograniczonym PS/X lub uruchamianiu aplikacji
przyciskiem X na przypadkowo zaznaczonej ikonie. Nie modyfikujemy SceShellCore.

## Hipoteza, ryzyko i rollback

- **Hipoteza:** obecny kontekst może utworzyć własne urządzenie VDA, a dane
  tego urządzenia zostaną włączone do systemowego wejścia bez zmian jądra.
- **Dowód:** statusy, jednoznaczne własne ID, ślad neutralnego InsertData,
  obserwacja uzgodnionej akcji oraz usunięcie tego samego ID.
- **Sukces:** potwierdzi drogę publikacji wejścia, niezależnie od USB/BT.
  Nie potwierdzi odbioru fizycznego DualSense.
- **Niepowodzenie:** odmowa Init/Add/Insert przy zachowanej własności i
  sprzątaniu zawęzi ograniczenie kontekstu; nie dowiedzie konieczności patcha.
- **Odczytowość:** Init/Add/Insert/Delete są operacjami stanowymi. Ten test
  **nie jest tylko odczytowy**, nawet bez AuthID/caps. Wymaga osobnej zgody.
- **Ryzyko:** nieznane ABI, utrata/niejednoznaczność ID, pozostawienie
  wirtualnego urządzenia, niezamierzona akcja menu lub błąd procesu.
  Kryteria wejścia mają ograniczyć te ryzyka; nie obiecujemy braku awarii.
- **Rollback:** neutralny stan, Delete wyłącznie potwierdzonego własnego ID,
  zwykłe zwolnienie własnych modułów. Przy utracie własności nie usuwać innych
  urządzeń. Reset konsoli nie jest automatycznym fallbackiem ani częścią zgody.

Stan gotowości: **PLAN ONLY — NOT RUN; brak gotowego bezpiecznego ELF VDA**.
W pierwszej kolejności wykonujemy odrębnie zatwierdzony pomiar USB-owner.
