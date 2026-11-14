# Baut Beispiel.jar (braucht Java 21+ JDK). Danach Beispiel.jar in den plugins/-Ordner des Servers legen.
# Benutzung:  powershell -ExecutionPolicy Bypass -File build.ps1
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
$api = if (Test-Path "creemc-api.jar") { "creemc-api.jar" } else { "..\dist\creemc-api.jar" }
if (Test-Path classes) { Remove-Item -Recurse -Force classes }
$src = Get-ChildItem src -Recurse -Filter *.java | ForEach-Object { $_.FullName }
& javac --release 21 -encoding UTF-8 -cp $api -d classes $src
if ($LASTEXITCODE -ne 0) { throw "javac fehlgeschlagen" }
Copy-Item -Recurse res\* classes
& jar --create --file Beispiel.jar -C classes .
Write-Host "Fertig: Beispiel.jar"
