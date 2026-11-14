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
| `CreeMC.API_VERSION` | Version der API (jetzt `2`) |
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

### GUIs, Seitenleiste, Bossbar, Partikel (API 2)

```java
// Menü mit Klick-Aktionen (in CreeMCPlugin: gui(...), sonst CreeMC.gui(plugin, ...))
gui("&6&lMenü", 3)
        .item(11, CreeMC.item("golden_apple").name("&aHeilen"), (p, click) -> CreeMC.heal(p))
        .item(15, CreeMC.item("feather").name("&bFliegen"), (p, click) -> p.setAllowFlight(true))
        .fill(CreeMC.item("gray_stained_glass_pane").name(" "))
        .onClose(p -> msg(p, "&7Menü zu"))
        .open(player);

// Seitenleiste rechts (bis 15 Zeilen, gleiche Texte erlaubt)
Sidebar sb = CreeMC.sidebar(player, "&6&lMein Server");
sb.lines("&7Spieler: &f" + Bukkit.getOnlinePlayers().size(), "", "&eviel Spaß!");
sb.line(0, "&7Spieler: &f5");   // eine Zeile ändern

// Bossbar
BossBar bar = CreeMC.bossBar(player, "&bEvent startet gleich", 0.5, "blue");
bar.setProgress(0.8);
bar.removePlayer(player);

// Partikel
CreeMC.particle("heart", x, y, z, 5);
CreeMC.dust(x, y, z, 0xFF00FF, 1.5f, 20);   // farbiger Staub
```

| Methode | Was sie tut |
|---|---|
| `gui(titel, reihen)` / `CreeMC.gui(plugin, titel, reihen)` | Menü-GUI (1-6 Reihen); Spieler können nichts herausnehmen |
| `Gui.item(slot, item, (spieler, klick) -> ...)` | Item mit Klick-Aktion (`klick` = Bukkit-`ClickType`) |
| `Gui.fill(item)`, `clear()`, `onClose(...)`, `open(spieler)` | füllen, leeren, beim Schließen, öffnen |
| `CreeMC.sidebar(spieler, titel)` | Seitenleiste; `lines(...)`, `line(i, text)`, `title(...)`, `hide()` |
| `CreeMC.bossBar(spieler, text, fortschritt, farbe)` | Bossbar (Farbe: pink, blue, red, green, yellow, purple, white) |
| `CreeMC.particle(name, x, y, z, anzahl)` | Partikel für alle in der Nähe |
| `CreeMC.dust(x, y, z, rgb, größe, anzahl)` | farbiger Staub |
| `CreeMC.particles()` | alle Partikel-Namen |
| `CreeMC.stack(itemBuilder, anzahl)` | ItemBuilder als Bukkit-`ItemStack` |
| `CreeMC.allowFlight(p, an)` | Fliegen im Überleben erlauben |
| `CreeMC.hide(betrachter, ziel)` / `show(...)` | Spieler unsichtbar machen (auch in der Tab-Liste) |

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

## Bukkit-/Paper-API: was geht

Vollständige Liste: [PLUGIN-API-BUKKIT.md](PLUGIN-API-BUKKIT.md). Die wichtigsten Teile:

- **Eigene Inventare/GUIs:** `Bukkit.createInventory(holder, größe, titel)` (Kiste 9-54, Trichter, Spender),
  `player.openInventory(inv)`, `InventoryClickEvent` (abbrechbar, `getSlot`, `getRawSlot`, `getClick`,
  `getCurrentItem`, `getView().getTitle()`), `InventoryOpenEvent`, `InventoryCloseEvent`, `InventoryHolder`
- **Items:** `ItemMeta` mit Name, Lore, Verzauberungen (`Enchantment.SHARPNESS` ...), unzerstörbar,
  Custom-Model-Data, Glanz, `ItemFlag`; `ItemStack.addEnchantment`, `editMeta`, `lore(...)`
- **Scoreboards:** `Bukkit.getScoreboardManager()`, Ziele in Seitenleiste/Tab/unter dem Namen, Werte,
  Teams mit Präfix/Suffix/Farbe/Namensschild/Kollision, `player.setScoreboard(...)`
- **Bossbars:** `Bukkit.createBossBar(...)` und Adventure `BossBar.bossBar(...)` mit `player.showBossBar(...)`
- **Partikel:** `world.spawnParticle(...)`, `player.spawnParticle(...)` mit allen Vanilla-Partikeln,
  `Particle.DustOptions`, Block-Partikel mit `Material`
- **Spieler:** Fliegen (`setAllowFlight`, `setFlying`), `setFlySpeed`/`setWalkSpeed`, `hidePlayer`/`showPlayer`
- **Welt:** `GameRule` (`world.setGameRule(GameRule.KEEP_INVENTORY, true)`), `getEntities()`, `spawnEntity(...)`,
  `createExplosion(...)`
- **Events:** Join, Quit, Chat, Befehle, Bewegung, Bauen, Abbauen, Interagieren (Block und Entity), Schleichen,
  Sprinten, Fliegen, Spielmodus, Schaden (Spieler und Mobs, mit Angreifer), Tod (Spieler und Mobs, Drops
  änderbar), Respawn (Ort änderbar), Item werfen, aufsammeln, essen, Hotbar-Slot, Hände tauschen, Hunger,
  Kick, Explosionen, Mob-Spawn, Inventar öffnen/klicken/schließen, Login, Server-Liste

## Grenzen

- Plugins laufen in einem eigenen Java-Prozess neben dem C++-Server. Abfragen an den Server (z.B. `block()`,
  `gameRule()`) warten auf die Antwort, also nicht tausende davon pro Tick stellen.
- NMS/CraftBukkit-Interna gibt es nicht. Teile von Paper, die Spielinhalte voraussetzen, die CreeMC noch nicht hat
  (Nether/End, Dorfbewohner, Tränke, Wetter, Rüstungsständer/Hologramme ...), fehlen noch.
- Die API-Version steigt mit neuen Funktionen; bestehende Funktionen bleiben erhalten.
