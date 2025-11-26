[CmdletBinding()]
param(
    [Parameter()]
    [Alias("c")]
    [switch]$Clean,

    [Parameter()]
    [Alias("b")]
    [string]$BuildDir = "uli78_pro"
)

$ErrorActionPreference = "Stop"

Write-Host "Build directory: $BuildDir"

if ($Clean) {
    Write-Host "Cleaning build directory..."
    if (Test-Path $BuildDir) {
        Remove-Item -Path "$BuildDir/CMakeCache.txt", "$BuildDir/CMakeFiles", "$BuildDir/cmake_install.cmake" -Recurse -Force -ErrorAction SilentlyContinue
    }
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Set-Location -Path $BuildDir

Write-Host "Configuring project with CMake..."
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_SDLGPU=On -DBUILD_PRO=On -DBUILD_STATIC=On -DBUILD_TOOLS=On -DBUILD_PLAYER=On -DCMAKE_POLICY_VERSION_MINIMUM="3.5" ..

Write-Host "Building tools..."
cmake --build . --config MinSizeRel --target prj2cart
cmake --build . --config MinSizeRel --target bin2txt
cmake --build . --config MinSizeRel --target localplayer-sdl

Set-Location -Path ".."
Write-Host "Processing assets..."
.\assets.bat

Set-Location -Path $BuildDir
Write-Host "Building main project..."
cmake --build . --config MinSizeRel --parallel

Write-Host "Build finished successfully."
