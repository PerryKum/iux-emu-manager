# Download Debian arm64 SDL2 packages into sysroot/ for zig cross-compile
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Sysroot = Join-Path $Root "sysroot"
$PkgDir = Join-Path $Root "sysroot_pkgs"
$Marker = Join-Path $Sysroot ".ready"
$ExtractPy = Join-Path $PSScriptRoot "extract_deb.py"

if (Test-Path $Marker) {
    Write-Host "sysroot ready: $Sysroot"
    exit 0
}

$Base = "http://ftp.debian.org/debian/pool"
$Debs = @(
    "$Base/main/libs/libsdl2/libsdl2-2.0-0_2.32.4+dfsg-1_arm64.deb",
    "$Base/main/libs/libsdl2/libsdl2-dev_2.32.4+dfsg-1_arm64.deb",
    "$Base/main/libs/libsdl2-ttf/libsdl2-ttf-2.0-0_2.24.0+dfsg-3+b1_arm64.deb",
    "$Base/main/libs/libsdl2-ttf/libsdl2-ttf-dev_2.24.0+dfsg-3+b1_arm64.deb"
)

New-Item -ItemType Directory -Force -Path $PkgDir, $Sysroot | Out-Null

foreach ($url in $Debs) {
    $name = Split-Path $url -Leaf
    $dest = Join-Path $PkgDir $name
    if (-not (Test-Path $dest) -or (Get-Item $dest).Length -lt 1000) {
        Write-Host "Downloading $name ..."
        curl.exe -fL --progress-bar -o $dest $url
        if ($LASTEXITCODE -ne 0) { throw "download failed: $url" }
    }
    Write-Host "Extracting $name ..."
    python $ExtractPy $dest $Sysroot
}

if (-not (Test-Path (Join-Path $Sysroot "usr/include/SDL2/SDL.h"))) {
    throw "SDL2 headers missing in sysroot"
}

New-Item -ItemType File -Path $Marker -Force | Out-Null
Write-Host "sysroot ready: $Sysroot"
