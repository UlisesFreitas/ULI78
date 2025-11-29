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

# Borrar la carpeta de datos de la aplicación para un inicio limpio
$appDataPath = Join-Path $env:APPDATA "com.uli78.uli"
if (Test-Path $appDataPath) {
    Write-Host "Borrando la carpeta de datos de la aplicación: $appDataPath"
    Remove-Item -Path $appDataPath -Recurse -Force
}

# Define la ruta del archivo de registro
$logFile = Join-Path $PSScriptRoot "build.log.txt"

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
$teeArgs = @{FilePath=$logFile;Append=$false}
Write-Host "Configuring project with CMake..."
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_SDLGPU=On -DBUILD_PRO=On -DBUILD_STATIC=On -DBUILD_TOOLS=On -DBUILD_PLAYER=On -DCMAKE_POLICY_VERSION_MINIMUM="3.5" .. | Tee-Object @teeArgs

Write-Host "Building tools..."
$teeArgs.Append = $true
cmake --build . --config MinSizeRel --target prj2cart | Tee-Object @teeArgs
cmake --build . --config MinSizeRel --target bin2txt | Tee-Object @teeArgs
cmake --build . --config MinSizeRel --target localplayer-sdl | Tee-Object @teeArgs

Set-Location -Path ".."
Write-Host "Processing assets..."
.\assets.bat

Set-Location -Path $BuildDir
Write-Host "Building main project..."
cmake --build . --config MinSizeRel --parallel | Tee-Object @teeArgs

Write-Host "Build finished successfully."
