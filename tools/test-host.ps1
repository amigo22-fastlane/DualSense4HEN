param([string]$Zig = "$PSScriptRoot/../.tools/zig-windows-x86_64-0.13.0/zig.exe")
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
} finally {
    $env:TEMP=$oldTemp; $env:TMP=$oldTmp; $env:ZIG_GLOBAL_CACHE_DIR=$oldCache
    Pop-Location
}
