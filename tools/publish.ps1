# Veröffentlicht CreeMC auf GitHub: öffentliches Repo AntonA77A/creemc + Release mit Quellcode und Egg.
# Benutzung (im Ordner creemc-cpp):  powershell -ExecutionPolicy Bypass -File tools\publish.ps1
$ErrorActionPreference = "Stop"
$gh = "C:\Program Files\GitHub CLI\gh.exe"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

# 1) Repo anlegen und Code hochladen (nur beim ersten Mal)
#    Die Prüfung darf fehlschlagen (Repo gibt es noch nicht) -> hier keinen Abbruch bei Fehlern
$ErrorActionPreference = "Continue"
& $gh repo view AntonA77A/creemc 2>&1 | Out-Null
$exists = $LASTEXITCODE -eq 0
$ErrorActionPreference = "Stop"
if (-not $exists) {
    & $gh repo create AntonA77A/creemc --public --source . --remote origin --push `
        --description "CreeMC: Minecraft-Server in C++ fuer 1.21 bis 26.3 mit Paper-/Spigot-Plugins und Pterodactyl-Egg"
    if ($LASTEXITCODE -ne 0) { throw "Repo konnte nicht angelegt werden" }
} else {
    git push origin main
}

# 2) Release mit Quellcode-Paket und Egg (das Egg lädt immer das neueste Release)
Copy-Item build\Release\creemc.exe dist\creemc.exe -Force
& $gh release create v0.2.0 "dist\creemc-source.tar.gz" "dist\egg-creemc.json" "dist\creemc.exe" `
    --repo AntonA77A/creemc --title "CreeMC 0.2.0" `
    --notes "Quellcode für den Pterodactyl-Egg (baut CreeMC bei der Installation). Egg: egg-creemc.json im Panel importieren."

Write-Host ""
Write-Host "Fertig! Egg-Download: https://github.com/AntonA77A/creemc/releases/latest/download/egg-creemc.json"
