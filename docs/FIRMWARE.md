# Firmware i zakres zgodności

| Konfiguracja | Status |
|---|---|
| PS4 Slim 12.52 + zgłoszony GoldHEN v2.4b18.12 | Bootstrap i eksporty na sprzęcie potwierdzone; Init/callback BT działają, rejestracja odrzucona; sterownik do grania jeszcze nie działa. |
| 12.53 | Pierwotna informacja została skorygowana przez właściciela na 12.52. Nie testowano; sonda odrzuca tę wersję. Brak profilu ABI i dowodów zgodności. |
| Inne firmware / inne kompilacje biblioteki | Sonda nie podejmuje eksperymentu; wymagane osobne odczyty, audyt i testy. |

Wymaganie GoldHEN v2.4b18.10+ pochodzi z Control4Free dla jego payloadu,
nie jest niezależnym wynikiem testów tej aplikacji. Przetestowano wyłącznie
konfigurację zgłoszoną powyżej. Wersja Plugin SDK 0x100 i wersja FTP v2.2
nie identyfikują wydania GoldHEN. Nie udostępniamy offsetów jądra ani patcha
dobieranego „najbliższym” numerem firmware.
