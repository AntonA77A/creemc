# Baut die Linux-Version per Docker: dist/creemc-linux-x64
# Benutzung (im Ordner creemc-cpp):  powershell -ExecutionPolicy Bypass -File tools\build-linux.ps1
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
New-Item -ItemType Directory -Force "$root\dist" | Out-Null
docker build -t creemc-build -f "$root\tools\Dockerfile.linux" "$root\tools"
docker run --rm -v "${root}:/src" creemc-build sh -c "cmake -S /src -B /tmp/b -DCMAKE_BUILD_TYPE=Release -DCREEMC_STATIC=ON && cmake --build /tmp/b -j && strip /tmp/b/creemc && cp /tmp/b/creemc /src/dist/creemc-linux-x64"
Get-Item "$root\dist\creemc-linux-x64" | Select-Object FullName, Length
