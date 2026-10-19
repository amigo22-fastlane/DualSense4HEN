# DualSense4HEN

Prototyp badawczy natywnego DualSense → wirtualny DualShock 4 na PS4.
Cel: bezpośredni Bluetooth z wewnętrznym modułem PS4, menu systemowe i gry,
bez telefonu, PC, Remote Play i adaptera podczas użytkowania.

**Nie jest jeszcze działającym sterownikiem DualSense do menu i gier.**
Kod startowy ELF, eksporty Bluetooth/VDA oraz inicjalizację i callback Bluetooth
przetestowano na PS4. Rejestracja urządzenia `054c:0ce6` zwróciła
`0x81540002`; nie otrzymano raportów z bezpośredniego Bluetooth.
Sprzęt zgłoszony przez właściciela: PS4 Slim, firmware **12.52**, GoldHEN
**v2.4b18.12**. Poprzednie 12.53 zostało przez właściciela skorygowane;
12.53 nie jest obsługiwane ani zweryfikowane przez ten projekt.

## Pierwszy etap

- C11: parser USB 0x01 (64 B), BT 0x01 (10 B), BT 0x31 (78 B, CRC32).
- Wspólny stan kontrolera, mapowanie DS4, dwa kontakty touchpada.
- Adapter 120-bajtowego stanu PS4 i prototyp `scePadVirtualDevice*`.
- Testy błędnych długości, CRC, mapowania, neutralizacji i usuwania urządzenia.
- Syntetyczne raporty z niezależnie wygenerowanym CRC, nie próbki sprzętowe.
- Zbudowane sondy ELF i observer Bluetooth z profilem biblioteki z PS4 12.52.
- Samodzielny PKG OpenOrbis do porównania USB z PayLoaderem:
  [test aplikacji 1.03](docs/USB_APP.md). Odbiór rzeczywistych raportów nadal
  wymaga potwierdzenia; sam start aplikacji nie jest sukcesem HID.
- Test sprzętowy: Init=0, RegisterCallback=0, RegisterDevice=0x81540002,
  UnregisterCallback=0, join=0. Znaczenie błędu rejestracji jest nieustalone.
- Poprzedni test USB zwrócił pustą listę; właściciel następnie wykrył problem
  kabla. Nowy kabel pozwolił wyliczyć DualSense i odczytać deskryptor HID; odczyt wejścia nadal zwraca błąd:
  [procedura i logi](docs/USB_RETEST.md). Nie odebrano raportów wejściowych USB.

## Testy na komputerze

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Windows, przenośny Zig 0.13.0 (kompiluje kod C):

```powershell
./tools/test-host.ps1 -Zig C:/path/to/zig.exe
```

Nie trzeba mieć kontrolera do testów. Fixtures są zapisane w repozytorium;
opcjonalna regeneracja: `python tools/make-fixtures.py`.

Architektura: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).
Audyt: [docs/AUDIT.md](docs/AUDIT.md). Licencje: [THIRD_PARTY.md](THIRD_PARTY.md).

Budowanie ELF: [docs/BUILD.md](docs/BUILD.md).
Instalacja i wycofanie: [docs/INSTALL_ROLLBACK.md](docs/INSTALL_ROLLBACK.md).
Pomiary: [docs/HARDWARE_RESULTS.md](docs/HARDWARE_RESULTS.md).
Odzyskane ABI i zasady observera: [docs/BLUETOOTH_ABI_1252.md](docs/BLUETOOTH_ABI_1252.md).
Kolejny eksperyment: [docs/NEXT_EXPERIMENT.md](docs/NEXT_EXPERIMENT.md).

W `dist/diagnostics/` są wyłącznie nasze ELF-y diagnostyczne i ich sumy.
Nie wpisuj ich do `plugins.ini` ani AutoRun. Firmware 12.53 jest odrzucane;
żadnych offsetów 12.52 nie przenosimy na inne wersje. Licencja: GPL-3.0-only.
