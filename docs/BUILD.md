# Budowanie

## Testy hosta bez SDK

Wystarczy kompilator C11 i CMake:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Na Windows można użyć `tools/test-host.ps1 -Zig C:/narzedzia/zig.exe`.
Skrypt nie instaluje narzędzi systemowych. Jeśli dostępny jest PS4 payload SDK,
kopiuje **tylko** `libusb.h` do katalogu testowego i uruchamia dodatkowo atrapę
USB. Nie używa nagłówków systemowych PS4 przy budowaniu programu na Windows.
Odpowiednik CMake: `-DD4H_USB_HEADER=/path/to/sdk/target/include/libusb.h`.

## ELF na Windows — wykonane w tej sesji

Kompilator C: [Zig 0.13.0](https://ziglang.org/download/0.13.0/release-notes.html),
archiwum `zig-windows-x86_64-0.13.0.zip` z oficjalnej domeny ziglang.org.
SHA-256 archiwum: `d859994725ef9402381e557c60bb57497215682e355204d754ee3df75ee3c158`.

Nagłówki: [PS4 payload SDK v0.9](https://github.com/ps4-payload-dev/sdk/releases/tag/v0.9),
plik `ps4-payload-sdk.zip`. SHA-256 pobranej dystrybucji:
`3190e88255a72161e5ce2f22a8ae8f638a2c5a9d1ff1e0209ed692d36b4d9ec5`.
SHA-256 użytego `libusb.h`:
`a82bc79209498e4d7c5615730753950fad9f796cade96c16a26ed23c1edeff91`.

Rozpakuj je do `.tools/` lub podaj ścieżki jawnie:

```powershell
./tools/build-ps4.ps1 -Zig C:/tools/zig/zig.exe -Sdk C:/tools/ps4-payload-sdk
python tools/check-elf.py build/ps4/dualsense4hen-bt-probe.elf build/ps4/dualsense4hen-usb-probe.elf build/ps4/dualsense4hen-bt-observer.elf
```

Powstają trzy ELF-y oraz `vda.o`. To własny freestanding runtime bez importów
ELF i bez SDK `crt1.o`: nie wykonuje jego `__patch_init`, kernel read/write,
podnoszenia authid lub zmian sandboxu. Numery syscalli i deklaracje systemowe
sprawdzono w SDK i OpenOrbis. Nie jest to przeniesienie linuksowego HID/BlueZ.
Zig generuje C/x86_64, a PS4 API jest rozwiązywane przez systemowy syscall 591.
Własne relokacje są ograniczone do segmentu danych tego ELF.

Sonda odczytu segmentów do badań ABI nie jest częścią zwykłej dystrybucji.
Można ją zbudować świadomie przez `-IncludeAbiDump`. Wymaga zgody właściciela
na lokalny odczyt kodu własnej konsoli. Nie publikuj uzyskanych segmentów.
Capstone 5.0.3 był użyty wyłącznie do lokalnej analizy; nie jest zależnością
parsera, observera, testów ani kompilacji ELF.

## ELF na Linux

Przygotowano `Makefile.ps4` dla Clang/lld i nagłówków SDK:

```sh
make -f Makefile.ps4 PS4_PAYLOAD_SDK=/opt/ps4-payload-sdk
```

Ta konkretna ścieżka Makefile nie została uruchomiona w środowisku Windows.
Zbudowano i sprawdzono ścieżkę PowerShell/Zig. Nie ma fikcyjnego PRX/PKG:
observer jest payloadem systemowego HID, a adapter VDA pozostaje prototypem
testowanym na hoście. Docelowa aplikacja/AutoRun powstaną po potwierdzeniu
parowania, odbioru i globalnego wejścia.
