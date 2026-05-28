# Cross-compile emu_manager with Zig (aarch64-linux-gnu)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Dist = Join-Path $Root "dist"
$Sysroot = Join-Path $Root "sysroot"

& (Join-Path $PSScriptRoot "setup_sysroot.ps1")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$zig = Get-Command zig -ErrorAction Stop
Write-Host "Using: $($zig.Source)"
& zig version

New-Item -ItemType Directory -Force -Path $Dist | Out-Null

$Target = "aarch64-linux-gnu"
$Out = Join-Path $Dist "emumanager"

$SysrootU = ($Sysroot -replace '\\', '/')
$RootU = ($Root -replace '\\', '/')
$OutU = ($Out -replace '\\', '/')
$LibSdl2 = "$SysrootU/usr/lib/aarch64-linux-gnu/libSDL2.so"
$LibSdl2Ttf = "$SysrootU/usr/lib/aarch64-linux-gnu/libSDL2_ttf.so"

Write-Host "==> Building $Out ..."

$zigArgs = @(
    "cc",
    "-target", $Target,
    "-O2",
    "--sysroot", $SysrootU,
    "-I$RootU/include",
    "-I$SysrootU/usr/include",
    "-I$SysrootU/usr/include/aarch64-linux-gnu",
    "-I$SysrootU/usr/include/SDL2",
    "$RootU/src/main.c",
    "$RootU/src/scan.c",
    "$RootU/src/fs_util.c",
    "$RootU/src/ui.c",
    "-o", $OutU,
    "-Wl,-rpath,/usr/lib/aarch64-linux-gnu",
    "-Wl,-rpath,/usr/lib",
    $LibSdl2,
    $LibSdl2Ttf,
    "-lm",
    "-lpthread",
    "-ldl"
)

& zig @zigArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed." -ForegroundColor Red
    exit $LASTEXITCODE
}

$size = [math]::Round((Get-Item $Out).Length / 1KB, 1)
Write-Host ""
Write-Host "OK: $Out ($size KB)" -ForegroundColor Green
Write-Host ""
Write-Host "Deploy binary + packaging/ to the device (see README)."
