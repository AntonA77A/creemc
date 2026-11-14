# Baut die Java-Bridge (bridge/creemc-bridge.jar, wird in den Server eingebaut) und das
# API-Paket für Plugin-Entwickler (dist/creemc-api.jar + dist/creemc-api-sources.jar).
# Benutzung (im Ordner creemc-cpp):  powershell -ExecutionPolicy Bypass -File tools\build-bridge.ps1
# Danach den Server neu bauen (cmake --build build --config Release), damit die neue Bridge eingebaut wird.
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
$out = Join-Path $root "build\bridge-classes"
if (Test-Path $out) { Remove-Item -Recurse -Force $out }
New-Item -ItemType Directory -Force $out | Out-Null
New-Item -ItemType Directory -Force dist | Out-Null

$files = Get-ChildItem bridge\src -Recurse -Filter *.java | ForEach-Object { $_.FullName }
$list = Join-Path $root "build\bridge-files.txt"
[IO.File]::WriteAllLines($list, $files)
& javac --release 21 -nowarn -encoding UTF-8 -d $out "@$list"
if ($LASTEXITCODE -ne 0) { throw "javac fehlgeschlagen" }
Copy-Item -Recurse -Force bridge\res\* $out

# 1) Bridge (alles, startbar)
$manifest = Join-Path $root "build\bridge-manifest.txt"
[IO.File]::WriteAllText($manifest, "Main-Class: creemc.bridge.Bridge`n")
& jar --create --file bridge\creemc-bridge.jar --manifest $manifest -C $out .
if ($LASTEXITCODE -ne 0) { throw "jar (Bridge) fehlgeschlagen" }

# 2) API zum Kompilieren eigener Plugins: Bukkit/Paper/Adventure-Teilmenge + creemc.api (ohne Bridge-Interna)
$api = @("creemc/api", "org/bukkit", "io/papermc", "net/kyori", "org/slf4j")
$args1 = @("--create", "--file", "dist\creemc-api.jar")
foreach ($p in $api) { $args1 += @("-C", $out, $p) }
& jar @args1
if ($LASTEXITCODE -ne 0) { throw "jar (API) fehlgeschlagen" }
$args2 = @("--create", "--file", "dist\creemc-api-sources.jar")
foreach ($p in $api) { $args2 += @("-C", "bridge\src", $p) }
& jar @args2

Write-Host "Fertig: bridge\creemc-bridge.jar, dist\creemc-api.jar, dist\creemc-api-sources.jar"
