param(
    [string]$Zig = "$PSScriptRoot/../.tools/zig-windows-x86_64-0.13.0/zig.exe",
    [string]$Sdk = "$PSScriptRoot/../.tools/ps4-payload-sdk"
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
$oldTemp = $env:TEMP
$oldTmp = $env:TMP
$oldCache = $env:ZIG_GLOBAL_CACHE_DIR
try {
    New-Item -ItemType Directory -Force build/host,build/tmp | Out-Null
    $env:TEMP = "$root/build/tmp"
    $env:TMP = $env:TEMP
    $env:ZIG_GLOBAL_CACHE_DIR = "$root/build/zig-cache"
    & $Zig cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 -Iinclude src/hid.c src/ps4_pad.c src/vpad.c platform/ps4/vda.c tests/test_core.c -o build/host/test_core.exe
    if ($LASTEXITCODE -ne 0) { throw 'C compilation failed' }
    & ./build/host/test_core.exe
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
    foreach ($variant in @('registered','passive')) {
        $btFlags=@()
        if ($variant -eq 'passive') { $btFlags=@('-DD4H_BT_PASSIVE') }
        & $Zig cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 @btFlags -Iinclude platform/ps4/bt_observer.c tests/test_bt.c -o "build/host/test_bt_$variant.exe"
        if ($LASTEXITCODE -ne 0) { throw 'BT mock compilation failed' }
        & "./build/host/test_bt_$variant.exe"
        if ($LASTEXITCODE -ne 0) { throw 'BT mock tests failed' }
    }
    if (Test-Path "$Sdk/target/include/libusb.h") {
        # Only the descriptor header; using the full PS4 sysroot for host tests would be wrong.
        New-Item -ItemType Directory -Force build/test-headers | Out-Null
        Copy-Item "$Sdk/target/include/libusb.h" build/test-headers/libusb.h
        & $Zig cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 -Iinclude -isystem build/test-headers -include tests/usb_compat.h src/hid.c src/ps4_pad.c platform/ps4/usb_probe.c tests/test_usb.c -o build/host/test_usb.exe
        if ($LASTEXITCODE -ne 0) { throw 'USB mock tests compilation failed' }
        & ./build/host/test_usb.exe
        if ($LASTEXITCODE -ne 0) { throw 'USB mock tests failed' }
        & $Zig cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 -DD4H_USB_ALT_ZERO_PROBE -Iinclude -isystem build/test-headers -include tests/usb_compat.h src/hid.c src/ps4_pad.c platform/ps4/usb_probe.c tests/test_usb.c -o build/host/test_usb_alt0.exe
        if ($LASTEXITCODE -ne 0) { throw 'USB alt 0 mock tests compilation failed' }
        & ./build/host/test_usb_alt0.exe
        if ($LASTEXITCODE -ne 0) { throw 'USB alt 0 mock tests failed' }
        & $Zig cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 -DD4H_USB_ACCESS_AUDIT -Iinclude -isystem build/test-headers -include tests/usb_compat.h src/hid.c src/ps4_pad.c platform/ps4/usb_probe.c tests/test_usb.c -o build/host/test_usb_access.exe
        if ($LASTEXITCODE -ne 0) { throw 'USB access mock tests compilation failed' }
        & ./build/host/test_usb_access.exe
        if ($LASTEXITCODE -ne 0) { throw 'USB access mock tests failed' }
    } else { Write-Output 'USB mock tests skipped: SDK descriptor header unavailable' }
} finally {
    $env:TEMP=$oldTemp; $env:TMP=$oldTmp; $env:ZIG_GLOBAL_CACHE_DIR=$oldCache
    Pop-Location
}
