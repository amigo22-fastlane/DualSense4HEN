param(
    [string]$Zig = "$PSScriptRoot/../.tools/zig-windows-x86_64-0.13.0/zig.exe",
    [string]$Sdk = "$PSScriptRoot/../.tools/ps4-payload-sdk",
    [switch]$IncludeAbiDump,
    [switch]$IncludeUsbAltZero,
    [switch]$IncludeBtPassive,
    [switch]$IncludeUsbAccessAudit,
    [switch]$IncludeUsbdAbiDump
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
$oldTemp=$env:TEMP; $oldTmp=$env:TMP; $oldCache=$env:ZIG_GLOBAL_CACHE_DIR
try {
    if (!(Test-Path "$Sdk/target/include/libusb.h")) { throw 'PS4 payload SDK headers not found' }
    New-Item -ItemType Directory -Force build/ps4,build/tmp | Out-Null
    $env:TEMP="$root/build/tmp"; $env:TMP=$env:TEMP; $env:ZIG_GLOBAL_CACHE_DIR="$root/build/zig-cache"
    $common=@('cc','-target','x86_64-freestanding','-std=c11','-Wall','-Wextra','-Werror',
        '-O2','-g0','-ffreestanding','-fno-builtin','-fno-stack-protector','-fPIE','-pie','-nostdlib',
        '-isystem',"$Sdk/target/include",'-Iinclude','-Iplatform/ps4',
        'platform/ps4/runtime.c','platform/ps4/probe.c',
        '-Wl,-e,_start,-T,platform/ps4/probe.ld,-z,max-page-size=0x4000')
    & $Zig @common -o build/ps4/dualsense4hen-bt-probe.elf
    if ($LASTEXITCODE -ne 0) { throw 'BT probe compilation failed' }
    & $Zig @common -DD4H_USB_PROBE platform/ps4/usb_probe.c src/hid.c src/ps4_pad.c -o build/ps4/dualsense4hen-usb-probe.elf
    if ($LASTEXITCODE -ne 0) { throw 'USB probe compilation failed' }
    if ($IncludeUsbAccessAudit) {
        & $Zig @common -DD4H_USB_PROBE -DD4H_USB_ACCESS_AUDIT platform/ps4/usb_probe.c src/hid.c src/ps4_pad.c -o build/ps4/dualsense4hen-usb-access-probe.elf
        if ($LASTEXITCODE -ne 0) { throw 'USB access audit compilation failed' }
    }
    if ($IncludeUsbAltZero) {
        & $Zig @common -DD4H_USB_PROBE -DD4H_USB_ALT_ZERO_PROBE platform/ps4/usb_probe.c src/hid.c src/ps4_pad.c -o build/ps4/dualsense4hen-usb-alt0-probe.elf
        if ($LASTEXITCODE -ne 0) { throw 'USB alt 0 probe compilation failed' }
    }
    if ($IncludeAbiDump) {
        & $Zig @common -DD4H_ABI_DUMP -o build/ps4/dualsense4hen-bt-abi-probe.elf
        if ($LASTEXITCODE -ne 0) { throw 'BT ABI probe compilation failed' }
    }
    if ($IncludeUsbdAbiDump) {
        & $Zig @common -DD4H_USB_ABI_DUMP -o build/ps4/dualsense4hen-usbd-abi-probe.elf
        if ($LASTEXITCODE -ne 0) { throw 'USBD ABI probe compilation failed' }
    }
    & $Zig @common -DD4H_BT_OBSERVER platform/ps4/bt_observer.c src/hid.c -o build/ps4/dualsense4hen-bt-observer.elf
    if ($LASTEXITCODE -ne 0) { throw 'BT observer compilation failed' }
    if ($IncludeBtPassive) {
        & $Zig @common -DD4H_BT_OBSERVER -DD4H_BT_PASSIVE platform/ps4/bt_observer.c src/hid.c -o build/ps4/dualsense4hen-bt-passive.elf
        if ($LASTEXITCODE -ne 0) { throw 'BT passive observer compilation failed' }
    }
    & $Zig cc -target x86_64-freestanding -std=c11 -Wall -Wextra -Werror -O2 -ffreestanding -fno-builtin -fno-stack-protector -fPIC -isystem "$Sdk/target/include" -Iinclude -c platform/ps4/vda.c -o build/ps4/vda.o
    if ($LASTEXITCODE -ne 0) { throw 'VDA prototype cross compilation failed' }
    $built=@('build/ps4/dualsense4hen-bt-probe.elf','build/ps4/dualsense4hen-usb-probe.elf','build/ps4/dualsense4hen-bt-observer.elf')
    if ($IncludeAbiDump) { $built+='build/ps4/dualsense4hen-bt-abi-probe.elf' }
    if ($IncludeUsbAltZero) { $built+='build/ps4/dualsense4hen-usb-alt0-probe.elf' }
    if ($IncludeBtPassive) { $built+='build/ps4/dualsense4hen-bt-passive.elf' }
    if ($IncludeUsbAccessAudit) { $built+='build/ps4/dualsense4hen-usb-access-probe.elf' }
    if ($IncludeUsbdAbiDump) { $built+='build/ps4/dualsense4hen-usbd-abi-probe.elf' }
    Get-FileHash $built -Algorithm SHA256 | Format-Table -AutoSize
} finally {
    $env:TEMP=$oldTemp; $env:TMP=$oldTmp; $env:ZIG_GLOBAL_CACHE_DIR=$oldCache
    Pop-Location
}
