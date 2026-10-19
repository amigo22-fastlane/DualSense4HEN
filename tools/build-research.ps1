param(
    [string]$Zig="$PSScriptRoot/../.tools/zig-windows-x86_64-0.13.0/zig.exe",
    [string]$Sdk="$PSScriptRoot/../.tools/ps4-payload-sdk",
    [string]$OpenOrbis="$PSScriptRoot/../.tools/OpenOrbis/PS4Toolchain",
    [switch]$ConnectedBt,
    [switch]$PadOnly,
    [switch]$HidControlAudit
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$oldTemp=$env:TEMP; $oldTmp=$env:TMP; $oldCache=$env:ZIG_GLOBAL_CACHE_DIR
Push-Location $root
try {
    New-Item -ItemType Directory -Force build/research,build/tmp | Out-Null
    $env:TEMP="$root/build/tmp"; $env:TMP=$env:TEMP; $env:ZIG_GLOBAL_CACHE_DIR="$root/build/zig-cache"
    & $Zig cc -target x86_64-freestanding -std=c11 -Wall -Wextra -Werror -O2 -g0 -ffreestanding -fno-builtin -fno-stack-protector -fPIE -pie -nostdlib -isystem "$Sdk/target/include" -Iinclude -Iplatform/ps4 platform/ps4/runtime.c platform/ps4/native_bt_gate.c src/socket_gate.c '-Wl,-e,_start,-T,platform/ps4/probe.ld,-z,max-page-size=0x4000' -o build/research/dualsense4hen-native-bt-gate.elf
    if ($LASTEXITCODE -ne 0) { throw 'Native Bluetooth contract gate compilation failed' }
    foreach ($source in @('src/acl_input.c','src/hid.c')) {
        $object='build/research/'+[IO.Path]::GetFileNameWithoutExtension($source)+'.o'
        & $Zig cc -target x86_64-freestanding -std=c11 -Wall -Wextra -Werror -O2 -ffreestanding -fno-builtin -fno-stack-protector -fPIC -isystem "$Sdk/target/include" -Iinclude -c $source -o $object
        if ($LASTEXITCODE -ne 0) { throw 'Independent input module cross compilation failed' }
    }
    Get-FileHash build/research/dualsense4hen-native-bt-gate.elf -Algorithm SHA256
    if ($HidControlAudit) {
        & $Zig cc -target x86_64-freestanding -std=c11 -Wall -Wextra -Werror -O2 -g0 -ffreestanding -fno-builtin -fno-stack-protector -fPIE -pie -nostdlib -DD4H_HID_CONTROL_ABI_DUMP -isystem "$Sdk/target/include" -Iinclude -Iplatform/ps4 platform/ps4/runtime.c platform/ps4/probe.c '-Wl,-e,_start,-T,platform/ps4/probe.ld,-z,max-page-size=0x4000' -o build/research/dualsense4hen-hidcontrol-abi.elf
        if ($LASTEXITCODE -ne 0) { throw 'HID Control audit compilation failed' }
        Get-FileHash build/research/dualsense4hen-hidcontrol-abi.elf -Algorithm SHA256
    }
    if ($ConnectedBt -or $PadOnly) {
        $probeFlags=@('-DD4H_BT_PASSIVE','-DD4H_BT_CONNECTED')
        $probeSources=@('platform/ps4/bt_observer.c','src/bt_monitor.c','src/hid.c')
        $probeOutput='build/research/dualsense4hen-connected-bt.elf'
        if ($PadOnly) { $probeFlags=@('-DD4H_PAD_ONLY'); $probeSources=@(); $probeOutput='build/research/dualsense4hen-pad-view.elf' }
        & $Zig cc -target x86_64-freestanding -std=c11 -Wall -Wextra -Werror -O2 -g0 -ffreestanding -fno-builtin -fno-stack-protector -fPIE -pie -nostdlib @probeFlags -isystem "$Sdk/target/include" -idirafter "$OpenOrbis/include" -Iinclude -Iplatform/ps4 platform/ps4/runtime.c platform/ps4/connected_bt_probe.c platform/ps4/pad_snapshot.c @probeSources '-Wl,-e,_start,-T,platform/ps4/probe.ld,-z,max-page-size=0x4000' -o $probeOutput
        if ($LASTEXITCODE -ne 0) { throw 'Existing-session Bluetooth diagnostic compilation failed' }
        Get-FileHash $probeOutput -Algorithm SHA256
    }
} finally {
    $env:TEMP=$oldTemp; $env:TMP=$oldTmp; $env:ZIG_GLOBAL_CACHE_DIR=$oldCache
    Pop-Location
}
