# DualSense4HEN

Prototyp badawczy natywnego DualSense → wirtualny DualShock 4 na PS4.
Cel: bezpośredni Bluetooth z wewnętrznym modułem PS4, menu systemowe i gry,
bez telefonu, PC, Remote Play i adaptera podczas użytkowania.

**Nie jest jeszcze działającym sterownikiem Bluetooth. Nie testowano na PS4.**
Sprzęt zgłoszony przez właściciela: PS4 Slim, firmware **12.52**, GoldHEN
**v2.4b18.12**. Poprzednie 12.53 zostało przez właściciela skorygowane;
12.53 nie jest obsługiwane ani zweryfikowane przez ten projekt.

## Pierwszy etap

- C11: parser USB 0x01 (64 B), BT 0x01 (10 B), BT 0x31 (78 B, CRC32).
- Wspólny stan kontrolera, mapowanie DS4, dwa kontakty touchpada.
- Adapter 120-bajtowego stanu PS4 i prototyp `scePadVirtualDevice*`.
- Testy błędnych długości, CRC, mapowania, neutralizacji i usuwania urządzenia.
- Syntetyczne raporty z niezależnie wygenerowanym CRC, nie próbki sprzętowe.

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
