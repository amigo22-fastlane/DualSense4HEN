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

Opcjonalne warianty USB budowane lokalnie:

- `-IncludeUsbAltZero`: osobny `dualsense4hen-usb-alt0-probe.elf`, wybiera
  wyłącznie istniejące ustawienie HID alt 0. Na tej konsoli zwróciło błąd.
- `-IncludeUsbdAbiDump`: `dualsense4hen-usbd-abi-probe.elf`, odczyt wyłącznie
  RX/R biblioteki libSceUsbd; wymaga osobnej zgody właściciela na tę bibliotekę.
  W tej sesji zgodę uzyskano i wykonano audyt, a pliki z PS4 usunięto.
  Zakres i ustalenia: `USBD_ABI_EXPERIMENT.md`. Ten ELF nie jest w dist.

Aktualny zwykły ELF USB (rewizja 6) zawiera tylko odczyty HID, parser,
podgląd mapowania, diagnostykę errno i skorygowane ABI sprzątania; nie kopiuje
biblioteki Sony. Zwykła kompilacja nadal buduje trzy ELF-y oraz vda.o.

## ELF na Linux

Przygotowano `Makefile.ps4` dla Clang/lld i nagłówków SDK:

```sh
make -f Makefile.ps4 PS4_PAYLOAD_SDK=/opt/ps4-payload-sdk
```

Ta konkretna ścieżka Makefile nie została uruchomiona w środowisku Windows.
Zbudowano i sprawdzono ścieżkę PowerShell/Zig. Adapter VDA pozostaje prototypem
testowanym na hoście; poniższy PKG służy diagnostyce USB, nie globalnemu wejściu.

## Samodzielny PKG OpenOrbis na Windows

Wykonana kompilacja: `tools/build-app.ps1`, Zig 0.13.0 + OpenOrbis v0.5.4
([oficjalne wydanie](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/releases/tag/v0.5.4)).
Pobrano `toolchain-llvm-18.tar.gz`, SHA-256
`3c7cd5bb593ca74fa1c13fd59f3938dc0fc07985167f7275063019e63abe4526`.
Domyślna lokalizacja po rozpakowaniu: `.tools/OpenOrbis/PS4Toolchain`.
Używany jest także pojedynczy nagłówek libusb.h z payload SDK opisany wyżej.

```powershell
./tools/build-app.ps1 -Sdk C:/tools/OpenOrbis/PS4Toolchain -Zig C:/tools/zig/zig.exe -UsbHeader C:/tools/ps4-payload-sdk/target/include/libusb.h -Python python
```

Skrypt kompiluje C, linkuje przez `zig ld.lld` z `link.x`, normalnym CRT,
statyczną libc OpenOrbis i compiler-rt, następnie tworzy SELF, SFO, GP4 i PKG
narzędziami Windows z SDK. Nie używa CRT payload SDK ani jego patchy.
PkgTool.Core wymaga .NET; w tej sesji lokalnie dostępny .NET 7/8 uruchomił
narzędzie z procesowym `DOTNET_ROLL_FORWARD=Major`. Zmienna jest przywracana
po kompilacji; nie instalowano systemowego .NET 3.

Wyjście: `build/app/IV0000-DSHN00001_00-DS4HENUSBDIAG001.pkg`.
Dystrybucja: `dist/app/DualSense4HEN-USB-Diagnostic.pkg`, wersja 1.03.
Zwykły ELF w build/app służy do konwersji SELF, nie do PayLoadera.
Weryfikacja obejmuje hashe/podpis PKG oraz własny walidator spójności SFO,
GP4 i nagłówka PKG. Regresja błędnego prefiksu tytułu jest w testach Python.
Poprawna struktura nie zastępuje instalacji i pomiaru na PS4.

Opcjonalny `./tools/build-ps4.ps1 -IncludeBtPassive` buduje oddzielny
`build/ps4/dualsense4hen-bt-passive.elf`: callback bez RegisterDevice, maks. 30 s.
To wariant do przyszłego pomiaru, nie implementacja parowania. Warunki profilu
i wycofanie są takie jak observera; zobacz `BLUETOOTH_ABI_1252.md`.

`-IncludeUsbAccessAudit` buduje dodatkowy `dualsense4hen-usb-access-probe.elf`.
To ten sam kod USB z odczytem dostępu open O_RDONLY + close do /dev/usbctl,
bez dodatkowych ioctl; wariant skompilowano, przetestowano na atrapach i
uruchomiono w PayLoaderze. Aplikacja 1.03 włącza taki sam audyt.
