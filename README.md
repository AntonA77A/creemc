# CreeMC (C++)

Minecraft-Server für **1.21 bis 26.3** gleichzeitig, als **eine einzige Datei** (alle Minecraft-Daten
und die Java-Bridge sind eingebettet, keine DLLs, kein Python).

## Bauen

**Windows** (Visual Studio 2022 Build Tools mit C++, CMake):

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

→ `build/Release/creemc.exe`

**Linux** (per Docker, auch von Windows aus; ergibt eine statische Datei für jede Distribution):

```
powershell -ExecutionPolicy Bypass -File tools\build-linux.ps1
```

→ `dist/creemc-linux-x64`. Direkt unter Linux geht auch `tools/build-linux.sh`
(braucht g++, cmake und die OpenSSL-Entwicklerdateien).

## Aufbau

| Ordner | Inhalt |
|---|---|
| `src/` | Server (Protokoll, Versionen, Welt, Spieler, Befehle, Online-Modus, Java-Bridge) |
| `data/` | Vanilla-Daten aller Versionen (Paket-IDs, Blöcke, Items, Registries, Rezepte ...) |
| `bridge/` | Java-Teil der Paper-/Bukkit-Bridge (`creemc-bridge.jar` + Quellen) |
| `tools/` | Linux-Build (Docker) |
| `egg-creemc.json` | Pterodactyl-/Pelican-Egg |

Windows nutzt für Verschlüsselung und Mojang-Login die eingebauten Windows-Funktionen (CNG, WinHTTP),
Linux OpenSSL (statisch gelinkt).

## Paper-/Spigot-Plugins

Liegen `.jar`-Dateien in `plugins/`, startet CreeMC automatisch Java mit der eingebetteten
`creemc-bridge.jar` (entpackt nach `.creemc/`). Server und Bridge reden über stdin/stdout, eine
JSON-Nachricht pro Zeile. Braucht Java 21 oder neuer.

Weitergeleitet werden: Beitreten/Verlassen, Chat, Befehle, Bewegung, Abbauen, Bauen, Rechtsklick,
Schleichen/Sprinten, Spielmodus-Wechsel, Login-Sperre, Serverliste. Plugins können Spieler,
Inventar, Blöcke, Zeit, Titel, Tab-Liste usw. ändern und eigene Befehle anmelden.

Die Bridge neu bauen (nur nach Änderungen an `bridge/src`):

```
javac --release 21 -encoding UTF-8 -d bridge/classes @(Get-ChildItem -Recurse bridge/src -Filter *.java | % FullName)
Copy-Item -Recurse bridge/res/* bridge/classes/
jar --create --file bridge/creemc-bridge.jar --main-class creemc.bridge.Bridge -C bridge/classes .
```

## Stand

Fertig: Serverliste, Login, Online-Modus, Welt (flach/normal), Chunks, Bewegung, andere Spieler,
Bauen/Abbauen mit richtiger Ausrichtung, Inventar und Crafting-Klicks, Chat, Befehle
(`help list version gamemode tp spawn say msg op deop kick time setblock give plugins save tps stop`),
Paper-/Spigot-Plugins.

Noch nicht portiert: Überleben (Leben, Hunger, Schaden), herumliegende Items, Mobs, Behälter/Öfen,
Redstone, Wasser/Lava, Verzaubern (die Python-Fassung davon liegt in `../creemc-python-backup.zip`).
