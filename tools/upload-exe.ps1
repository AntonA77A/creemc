# Hängt die Windows-Datei creemc.exe an das GitHub-Release v0.2.0 an.
# Benutzung (im Ordner creemc-cpp):  powershell -ExecutionPolicy Bypass -File tools\upload-exe.ps1
$gh = "C:\Program Files\GitHub CLI\gh.exe"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
Copy-Item build\Release\creemc.exe dist\creemc.exe -Force
& $gh release upload v0.2.0 "dist\creemc.exe" --repo AntonA77A/creemc --clobber
if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "Fertig! Exe-Download: https://github.com/AntonA77A/creemc/releases/latest/download/creemc.exe"
}
