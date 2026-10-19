param(
    [string]$Zig = "$PSScriptRoot/../.tools/zig-windows-x86_64-0.13.0/zig.exe",
    [string]$Sdk = "$PSScriptRoot/../.tools/OpenOrbis/PS4Toolchain",
    [string]$UsbHeader = "$PSScriptRoot/../.tools/ps4-payload-sdk/target/include/libusb.h",
    [string]$Python = "$env:USERPROFILE/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe"
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$Sdk=(Resolve-Path -LiteralPath $Sdk).Path
$Zig=(Resolve-Path -LiteralPath $Zig).Path
$oldTemp=$env:TEMP; $oldTmp=$env:TMP; $oldCache=$env:ZIG_GLOBAL_CACHE_DIR
$oldSdk=$env:OO_PS4_TOOLCHAIN; $oldRoll=$env:DOTNET_ROLL_FORWARD
function Invoke-Checked([string]$Command,[string[]]$Arguments) {
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Tool failed: $Command ($LASTEXITCODE)" }
}
Push-Location $root
try {
    if (!(Test-Path "$Sdk/lib/crt1.o")) { throw 'OpenOrbis application CRT is missing' }
    New-Item -ItemType Directory -Force build/app/pkg/sce_sys,build/app/pkg/sce_module,build/app/pkg/assets/misc,build/app/headers,build/app/obj,build/tmp | Out-Null
    # The release omits libusb.h. Use only the audited BSD descriptor header,
    # never mix the payload SDK's complete sysroot with OpenOrbis musl.
    Copy-Item -LiteralPath $UsbHeader -Destination build/app/headers/libusb.h
    $env:TEMP="$root/build/tmp"; $env:TMP=$env:TEMP; $env:ZIG_GLOBAL_CACHE_DIR="$root/build/zig-cache"
    $env:OO_PS4_TOOLCHAIN=$Sdk
    $env:DOTNET_ROLL_FORWARD='Major' # Child tools only; no system runtime installation.
    $compile=@('cc','-target','x86_64-freebsd','-std=c11','-Wall','-Wextra','-Werror','-O2','-g0',
        '-ffreestanding','-fno-builtin','-fno-stack-protector','-fPIC','-c',
        '-DD4H_APPLICATION','-DD4H_USB_PROBE','-DD4H_USB_ACCESS_AUDIT','-isystem',"$Sdk/include",'-isystem','build/app/headers','-Iinclude','-Iplatform/ps4',
        '-include','platform/ps4/usb_header_compat.h')
    $sources=@('platform/ps4/app/main.c','platform/ps4/runtime.c','platform/ps4/probe.c',
        'platform/ps4/usb_probe.c','src/hid.c','src/ps4_pad.c')
    $objects=@()
    foreach ($source in $sources) {
        $object='build/app/obj/'+$source.Replace('/','_')+'.o'
        Invoke-Checked $Zig ($compile+@($source,'-o',$object))
        $objects+=$object
    }
    # Invoke LLD directly: Zig's libc discovery must not substitute its own
    # FreeBSD runtime for the audited OpenOrbis CRT and static musl archive.
    Invoke-Checked $Zig (@('ld.lld','-m','elf_x86_64','-pie','--script',"$Sdk/link.x",'--eh-frame-hdr',
        '-z','max-page-size=0x4000','--Map=build/app/link.map','-o','build/app/dualsense4hen-usb-app.elf',"$Sdk/lib/crt1.o")+
        $objects+@("$Sdk/lib/libc.a","$Sdk/lib/libkernel.so","$Sdk/lib/libclang_rt.builtins-x86_64.a"))
    Invoke-Checked "$Sdk/bin/windows/create-fself.exe" @('-in',"$root/build/app/dualsense4hen-usb-app.elf",
        '-out',"$root/build/app/dualsense4hen-usb-app.oelf",'--eboot',"$root/build/app/pkg/eboot.bin",'--paid','0x3800000000000011')
    # These are OpenOrbis's open-source empty module stubs, not console files.
    Copy-Item -LiteralPath "$Sdk/samples/hello_world/sce_module/libc.prx" -Destination build/app/pkg/sce_module/libc.prx
    Copy-Item -LiteralPath "$Sdk/samples/hello_world/sce_module/libSceFios2.prx" -Destination build/app/pkg/sce_module/libSceFios2.prx
    Copy-Item -LiteralPath LICENSE -Destination build/app/pkg/assets/misc/LICENSE.txt
    Copy-Item -LiteralPath third_party/licenses/OpenOrbis-musl-COPYRIGHT.txt -Destination build/app/pkg/assets/misc/OpenOrbis-musl-COPYRIGHT.txt
    Copy-Item -LiteralPath third_party/licenses/LLVM-compiler-rt.txt -Destination build/app/pkg/assets/misc/LLVM-compiler-rt.txt
    Copy-Item -LiteralPath platform/ps4/app/NOTICE.txt -Destination build/app/pkg/assets/misc/NOTICE.txt
    Invoke-Checked $Python @('tools/make-app-icon.py','build/app/pkg/sce_sys/icon0.png')
    $pkgtool="$Sdk/bin/windows/PkgTool.Core.exe"
    $sfo="$root/build/app/pkg/sce_sys/param.sfo"
    Invoke-Checked $pkgtool @('sfo_new',$sfo)
    $content='IV0000-DSHN00001_00-DS4HENUSBDIAG001'
    if ($content -cnotmatch '^[A-Z]{2}[0-9]{4}-[A-Z]{4}[0-9]{5}_00-[A-Z0-9]{16}$') { throw 'Invalid PS4 content ID format' }
    $versionText=Get-Content -LiteralPath include/ds4hen/usb_diagnostic.h -Raw
    if ($versionText -notmatch '#define D4H_APP_VERSION "([0-9]+\.[0-9]{2})"') { throw 'Application version missing' }
    $appVersion=$Matches[1]
    $entries=@(
        @('APP_TYPE','Integer','4','1'),@('APP_VER','Utf8','8',$appVersion),
        @('ATTRIBUTE','Integer','4','0'),@('CATEGORY','Utf8','4','gd'),
        @('CONTENT_ID','Utf8','48',$content),@('DOWNLOAD_DATA_SIZE','Integer','4','64'),
        @('SYSTEM_VER','Integer','4','0'),@('TITLE','Utf8','128','DualSense4HEN USB Diagnostic'),
        @('TITLE_ID','Utf8','12','DSHN00001'),@('VERSION','Utf8','8',$appVersion))
    foreach ($entry in $entries) {
        Invoke-Checked $pkgtool @('sfo_setentry',$sfo,$entry[0],'--type',$entry[1],'--maxsize',$entry[2],'--value',$entry[3])
    }
    Push-Location build/app/pkg
    try {
        Invoke-Checked "$Sdk/bin/windows/create-gp4.exe" @('-out','app.gp4','--content-id',$content,
            '--files','eboot.bin sce_sys/param.sfo sce_sys/icon0.png sce_module/libc.prx sce_module/libSceFios2.prx assets/misc/LICENSE.txt assets/misc/OpenOrbis-musl-COPYRIGHT.txt assets/misc/LLVM-compiler-rt.txt assets/misc/NOTICE.txt')
        Invoke-Checked $Python @("$root/tools/check-app.py",'--sfo','sce_sys/param.sfo','--gp4','app.gp4')
        Invoke-Checked $pkgtool @('pkg_build','app.gp4','..')
    } finally { Pop-Location }
    Invoke-Checked $pkgtool @('pkg_validate',"$root/build/app/$content.pkg")
    Invoke-Checked $Python @('tools/check-app.py','--sfo','build/app/pkg/sce_sys/param.sfo','--gp4','build/app/pkg/app.gp4','--pkg',"build/app/$content.pkg")
    Get-FileHash "build/app/$content.pkg" -Algorithm SHA256 | Format-List
} finally {
    $env:TEMP=$oldTemp; $env:TMP=$oldTmp; $env:ZIG_GLOBAL_CACHE_DIR=$oldCache
    $env:OO_PS4_TOOLCHAIN=$oldSdk; $env:DOTNET_ROLL_FORWARD=$oldRoll
    Pop-Location
}
