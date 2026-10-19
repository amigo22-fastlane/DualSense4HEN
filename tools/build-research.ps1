param(
    [string]$Zig="$PSScriptRoot/../.tools/zig-windows-x86_64-0.13.0/zig.exe",
    [string]$Sdk="$PSScriptRoot/../.tools/ps4-payload-sdk"
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
} finally {
    $env:TEMP=$oldTemp; $env:TMP=$oldTmp; $env:ZIG_GLOBAL_CACHE_DIR=$oldCache
    Pop-Location
}
