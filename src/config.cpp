#include "config.hpp"

#include <algorithm>
#include <map>
#include <type_traits>

#include "text.hpp"
#include "util.hpp"

static const char* DEFAULT_CONFIG = R"(# CreeMC Konfiguration
# Nach Änderungen den Server neu starten.

# Adresse und Port
host = "0.0.0.0"
port = 25565

# Text in der Serverliste (Farbcodes mit &)
motd = "&a&lCreeMC &7- &fC++-Minecraft-Server"
max_players = 20

# Sichtweite in Chunks (2-32). Höher = mehr CPU-Last.
view_distance = 8

# Standard-Spielmodus: 0 = Überleben, 1 = Kreativ, 2 = Abenteuer, 3 = Zuschauer
gamemode = 0
# 0 = Friedlich, 1 = Einfach, 2 = Normal, 3 = Schwer
difficulty = 1

# Weltgenerator: "normal" (Hügel, Wasser, Bäume) oder "flat"
world_name = "world"
generator = "normal"
seed = 12345

# Tag/Nacht-Wechsel
daylight_cycle = true

# Chatformat: {name} und {message} werden ersetzt
chat_format = "&7{name}&8: &f{message}"
join_message = "&e{name} hat den Server betreten"
quit_message = "&e{name} hat den Server verlassen"

# Online-Modus: Spieler werden bei Mojang geprüft und haben ihre echten Skins.
# false = auch Spieler ohne gekauftes Minecraft dürfen rein (Offline-Modus).
online_mode = true

# Spielregeln. pvp, fall_damage, block_drops, keep_inventory, daylight_cycle und die Mob-Schalter sind
# Startwerte für neue Welten - danach im Spiel mit /gamerule ändern (gespeichert in <welt>/gamerules.json).
pvp = true
hunger = true
fall_damage = true
block_drops = true
passive_mobs = true
hostile_mobs = true
max_passive = 16
max_hostile = 12
keep_inventory = false
)";

// Sehr einfacher TOML-Leser: key = "text" | zahl | true/false, Kommentare mit #
static std::map<std::string, std::string> parseToml(const std::string& txt, std::map<std::string, bool>& isString) {
    std::map<std::string, std::string> out;
    for (std::string line : split(txt, '\n')) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == '[') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));
        if (!val.empty() && (val[0] == '"' || val[0] == '\'')) {
            char q = val[0];
            std::string s;
            for (size_t i = 1; i < val.size(); ++i) {
                if (val[i] == '\\' && q == '"' && i + 1 < val.size()) {
                    char e = val[++i];
                    s += e == 'n' ? '\n' : e == 't' ? '\t' : e;
                } else if (val[i] == q) {
                    break;
                } else {
                    s += val[i];
                }
            }
            out[key] = s;
            isString[key] = true;
        } else {
            size_t hash = val.find('#');
            out[key] = trim(hash == std::string::npos ? val : val.substr(0, hash));
            isString[key] = false;
        }
    }
    return out;
}

Config Config::load(const std::string& path) {
    std::string txt;
    if (!readFile(path, txt)) {
        writeFileAtomic(path, DEFAULT_CONFIG);
        txt = DEFAULT_CONFIG;
    }
    if (txt.size() >= 3 && (unsigned char)txt[0] == 0xEF) txt = txt.substr(3);   // BOM
    std::map<std::string, bool> isStr;
    auto raw = parseToml(txt, isStr);
    Config c;
    auto str = [&](const char* k, std::string& dst) {
        if (raw.count(k)) dst = raw[k];
    };
    auto num = [&](const char* k, auto& dst) {
        if (raw.count(k)) {
            try {
                dst = static_cast<std::decay_t<decltype(dst)>>(std::stoll(raw[k]));
            } catch (...) {
                logx::warn(std::string("config.toml: '") + k + "' ist keine Zahl");
            }
        }
    };
    auto flag = [&](const char* k, bool& dst) {
        if (raw.count(k)) dst = raw[k] == "true";
    };
    str("host", c.host);
    num("port", c.port);
    str("motd", c.motd);
    num("max_players", c.maxPlayers);
    num("view_distance", c.viewDistance);
    num("gamemode", c.gamemode);
    num("difficulty", c.difficulty);
    str("world_name", c.worldName);
    str("generator", c.generator);
    num("seed", c.seed);
    flag("daylight_cycle", c.daylightCycle);
    str("chat_format", c.chatFormat);
    str("join_message", c.joinMessage);
    str("quit_message", c.quitMessage);
    flag("online_mode", c.onlineMode);
    flag("pvp", c.pvp);
    flag("hunger", c.hunger);
    flag("fall_damage", c.fallDamage);
    flag("block_drops", c.blockDrops);
    flag("passive_mobs", c.passiveMobs);
    flag("hostile_mobs", c.hostileMobs);
    num("max_passive", c.maxPassive);
    num("max_hostile", c.maxHostile);
    flag("keep_inventory", c.keepInventory);
    c.motd = text::colorize(c.motd);
    c.chatFormat = text::colorize(c.chatFormat);
    c.joinMessage = text::colorize(c.joinMessage);
    c.quitMessage = text::colorize(c.quitMessage);
    c.viewDistance = std::max(2, std::min(32, c.viewDistance));
    if (c.gamemode < 0 || c.gamemode > 3) c.gamemode = 0;
    return c;
}
