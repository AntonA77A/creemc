# Veröffentlicht CreeMC auf GitHub: Code hochladen + Release mit Quellcode, Egg und Windows-Exe.
# Benutzung (im Ordner creemc-cpp):  powershell -ExecutionPolicy Bypass -File tools\publish.ps1 0.3.0
param([string]$Version = "0.3.0")
$ErrorActionPreference = "Stop"
$gh = "C:\Program Files\GitHub CLI\gh.exe"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
$tag = "v$Version"

# 1) Repo anlegen (nur beim ersten Mal) bzw. Code hochladen
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
    if ($LASTEXITCODE -ne 0) { throw "git push fehlgeschlagen" }
}

# 2) Release mit Quellcode-Paket, Egg und Exe (das Egg lädt immer das neueste Release)
Copy-Item build\Release\creemc.exe dist\creemc.exe -Force
& $gh release create $tag "dist\creemc-source.tar.gz" "dist\egg-creemc.json" "dist\creemc.exe" "dist\creemc-api.jar" "dist\creemc-api-sources.jar" `
    --repo AntonA77A/creemc --title "CreeMC $Version" `
    --notes "Vanilla-Gameplay: Ueberleben (Leben, Hunger, Schaden, Tod), Mobs, Kampf mit Ruestung und Verzauberungen, Block-Drops, TNT, Kisten, Oefen, Trichter, Wasser/Lava, fallender Sand, Pflanzen, Redstone mit Kolben, Zaubertisch und Amboss, Spielregeln (/gamerule), eigene Plugin-API (creemc-api.jar, siehe PLUGIN-API.md). Egg: egg-creemc.json im Panel importieren. Windows: creemc.exe."
if ($LASTEXITCODE -ne 0) { throw "Release konnte nicht erstellt werden" }

Write-Host ""
Write-Host "Fertig! Egg:  https://github.com/AntonA77A/creemc/releases/latest/download/egg-creemc.json"
Write-Host "        Exe:  https://github.com/AntonA77A/creemc/releases/latest/download/creemc.exe"
