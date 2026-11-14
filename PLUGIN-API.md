# CreeMC-Plugin-API

CreeMC-Plugins sind Java-Plugins (`.jar` in `plugins/`, braucht Java 21+). Es gibt zwei Wege, die sich auch
mischen lassen:

| | Eigenes CreeMC-Plugin | Bukkit-/Paper-Plugin |
|---|---|---|
| Beschreibung | `creemc.yml` | `plugin.yml` / `paper-plugin.yml` |
| Hauptklasse | `extends creemc.api.CreeMCPlugin` | `extends JavaPlugin` |
| API | `creemc.api.*` **und** die Bukkit-Teilmenge | Bukkit-Teilmenge **und** `creemc.api.*` |
| Läuft auf | nur CreeMC | CreeMC (und Paper/Spigot, solange es kein `creemc.api` benutzt) |

Die Bukkit-/Paper-Teilmenge (welche Klassen und Methoden es gibt) steht in
[PLUGIN-API-BUKKIT.md](PLUGIN-API-BUKKIT.md).

## Schnellstart

1. `creemc-api.jar` holen: liegt in `dist/` bzw. im GitHub-Release
   (selbst bauen: `powershell -ExecutionPolicy Bypass -File tools\build-bridge.ps1`).
   `creemc-api-sources.jar` dazu, damit die IDE Erklärungen anzeigt.
2. Projekt wie in [`beispiel-plugin/`](beispiel-plugin/) anlegen:

```
mein-plugin/
├── src/meinplugin/MeinPlugin.java
├── res/creemc.yml
└── res/config.yml        (optional, wird mit saveDefaultConfig() angelegt)
```

`res/creemc.yml`:

```yaml
name: MeinPlugin
version: 1.0
main: meinplugin.MeinPlugin
description: Was das Plugin macht
author: Du
depend: []          # optional: andere Plugins, die vorher geladen sein müssen
```

`src/meinplugin/MeinPlugin.java`:

```java
package meinplugin;

import creemc.api.CreeMC;
import creemc.api.CreeMCPlugin;
import org.bukkit.entity.Player;
import org.bukkit.event.player.PlayerJoinEvent;

public class MeinPlugin extends CreeMCPlugin {
    @Override
    public void onEnable() {
        on(PlayerJoinEvent.class, e -> msg(e.getPlayer(), "&aWillkommen, " + e.getPlayer().getName() + "!"));

        command("heilen", "Heilt dich", (sender, args) -> {
            if (sender instanceof Player p) CreeMC.heal(p);
        });

        every(20 * 60, () -> broadcast("&eWieder eine Minute vorbei"));
    }
}
```

3. Bauen (wie `beispiel-plugin/build.ps1` bzw. `build.sh`):

```
javac --release 21 -cp creemc-api.jar -d classes src/meinplugin/*.java
cp -r res/* classes/
jar --create --file MeinPlugin.jar -C classes .
```

4. `MeinPlugin.jar` in den `plugins/`-Ordner des Servers legen und den Server neu starten.

Farbcodes: überall, wo die CreeMC-API Text nimmt, gehen `&a`, `&l` usw. (und `§`).
`&&` schreibt ein echtes `&`.

## `CreeMCPlugin` (Basisklasse)

Erbt von `JavaPlugin`: `getConfig()`, `saveDefaultConfig()`, `getLogger()`, `getDataFolder()`, `onDisable()` usw. gehen wie gewohnt.

| Methode | Was sie tut |
|---|---|
| `command(name, beschreibung, (sender, args) -> ...)` | Befehl ohne yml-Eintrag |
| `command(name, beschreibung, nurOp, handler)` | Befehl, optional nur für OPs |
| `command(name, beschreibung, nurOp, handler, (sender, args) -> vorschläge)` | mit Tab-Vervollständigung |
| `on(EventKlasse.class, e -> ...)` | auf ein Event hören |
| `on(EventKlasse.class, EventPriority.HIGH, e -> ...)` | mit Priorität |
| `later(ticks, () -> ...)` | einmal nach `ticks` (20 = 1 Sekunde) |
| `every(ticks, () -> ...)` | immer wieder alle `ticks` |
| `msg(sender, "&aText")` | Nachricht mit Farbcodes |
| `broadcast("&eText")` | Nachricht an alle |

Im Befehl: `throw new IllegalArgumentException("Benutzung: /befehl <x>")` schickt den Text rot an den Absender.

Events: alle aus [PLUGIN-API-BUKKIT.md](PLUGIN-API-BUKKIT.md), z.B. `PlayerJoinEvent`, `PlayerQuitEvent`,
`AsyncPlayerChatEvent`, `BlockBreakEvent`, `BlockPlaceEvent`, `PlayerInteractEvent`, `PlayerMoveEvent`,
`PlayerDeathEvent`, `PlayerRespawnEvent`, `EntityDamageEvent`, `PlayerCommandPreprocessEvent`.

## `CreeMC` (statische Funktionen)

### Server

| Methode | Was sie tut |
|---|---|
| `CreeMC.API_VERSION` | Version der API (jetzt `1`) |
| `tick()` | aktueller Server-Tick |
| `broadcast(text)` | Nachricht an alle |
| `console(befehl)` | Befehl als Konsole ausführen, z.B. `"time set 0"` |
| `player(name)` | Spieler mit diesem Namen oder `null` |
| `color(text)` | `&`-Codes in `§`-Codes umwandeln |
| `time(zeit)` | Uhrzeit setzen (0 Morgen, 6000 Mittag, 13000 Nacht) |

### Spielregeln

Namen wie in Vanilla, beide Schreibweisen: `fallDamage` oder `fall_damage`.

| Methode | Was sie tut |
|---|---|
| `gameRule(name)` | Wert als Text (`"true"`, `"3"` ...) oder `null` |
| `setGameRule(name, wert)` | setzen; `false`, wenn Regel/Wert ungültig |
| `gameRules()` | alle Regeln als `Map<String, String>` |

### Welt

| Methode | Was sie tut |
|---|---|
| `block(x, y, z)` | Block mit Zuständen, z.B. `"minecraft:lever[face=wall,facing=north,powered=false]"` |
| `setBlock(x, y, z, block)` | Block setzen, auch `"lever[powered=true]"`; löst Redstone/Wasser/Sand-Updates aus |
| `breakBlock(x, y, z, drops)` | Block entfernen, mit oder ohne Drops |
| `highestY(x, z)` | höchster Block an x, z |
| `explode(x, y, z, stärke, blöckeZerstören)` | Explosion (TNT = 4, Creeper = 3) |
| `tnt(x, y, z, zündzeit)` | angezündetes TNT (Vanilla-Zündzeit 80 Ticks) |
| `spawnMob(typ, x, y, z)` | Mob erzeugen → Entity-ID (oder -1) |
| `mobTypes()` | bekannte Mobs (`pig`, `cow`, `sheep`, `chicken`, `zombie`, `spider`, `creeper`) |
| `dropItem(x, y, z, item, anzahl)` | Item fallen lassen (auch `ItemBuilder`) |
| `sound(x, y, z, sound, lautstärke, tonhöhe)` | Sound, z.B. `"entity.player.levelup"` |

### Spieler

| Methode | Was sie tut |
|---|---|
| `heal(p)` | voll heilen (Leben, Hunger, Feuer) |
| `damage(p, menge, ursache)` | Schaden wie Vanilla (Rüstung zählt): `attack`, `fall`, `fire`, `lava`, `drown`, `explosion`, `generic` |
| `kill(p, nachricht)` | töten (`null` = Standard-Todesnachricht) |
| `giveXp(p, punkte)` | Erfahrung mit Sound und Stufenaufstieg |
| `title(p, titel, untertitel)` | Titel in der Bildschirmmitte |
| `actionBar(p, text)` | Text über der Hotbar |
| `tabList(p, kopf, fuß)` | Kopf-/Fußzeile der Tab-Liste |
| `give(p, item, anzahl)` | Item geben → Anzahl, die nicht gepasst hat |
| `take(p, item, anzahl)` | Items wegnehmen → Anzahl, die gefehlt hat |
| `push(p, vx, vy, vz)` | wegstoßen |
| `teleport(p, x, y, z)` / `teleport(p, location)` | teleportieren |
| `info(uuid)` | Map mit `name`, `x`, `y`, `z`, `health`, `food`, `saturation`, `level`, `xp`, `gamemode`, `dead`, `ping`, `version` |

### Items mit Verzauberungen und Namen

```java
String schwert = CreeMC.item("diamond_sword")
        .enchant("sharpness", 5)
        .enchant("fire_aspect", 2)
        .name("&6&lCreeper-Klinge")
        .build();
CreeMC.give(player, schwert, 1);

// Verzaubertes Buch (gespeicherte Verzauberung, für den Amboss)
CreeMC.give(player, CreeMC.item("enchanted_book").enchant("mending", 1), 1);
```

Verzauberungen und Items heißen wie in Vanilla (`minecraft:` kann weggelassen werden).

## Beispiel

[`beispiel-plugin/`](beispiel-plugin/) zeigt alles auf einmal: Begrüßungstitel, `/boom`, `/heilen`,
`/zauberschwert`, `/regel` (Spielregeln mit Tab-Vervollständigung), `/mob`, Erfahrung für Diamanterz
und eine wiederkehrende Nachricht. Bauen mit `build.ps1` (Windows) oder `build.sh` (Linux).

## Grenzen

- Plugins laufen in einem eigenen Java-Prozess neben dem C++-Server. Abfragen an den Server (z.B. `block()`,
  `gameRule()`) warten auf die Antwort, also nicht tausende davon pro Tick stellen.
- NMS/CraftBukkit-Interna, GUIs mit eigenen Inventaren und Scoreboards gibt es nicht.
- Die API-Version steigt mit neuen Funktionen; bestehende Funktionen bleiben erhalten.
