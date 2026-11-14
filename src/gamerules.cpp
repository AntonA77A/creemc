#include "gamerules.hpp"

#include <algorithm>

#include "config.hpp"
#include "json.hpp"
#include "util.hpp"

namespace {

std::string toSnake(const std::string& camel) {
    std::string out;
    for (char c : camel) {
        if (c >= 'A' && c <= 'Z') {
            out += '_';
            out += char(c - 'A' + 'a');
        } else {
            out += c;
        }
    }
    return out;
}

GameRules::Def B(const char* n, bool def, const char* desc, const char* snake = nullptr) {
    return {n, snake ? snake : toSnake(n), true, def ? 1 : 0, desc};
}
GameRules::Def I(const char* n, int def, const char* desc, const char* snake = nullptr) {
    return {n, snake ? snake : toSnake(n), false, def, desc};
}

const std::vector<GameRules::Def>& table() {
    static const std::vector<GameRules::Def> t = {
        B("announceAdvancements", true, "Fortschritte im Chat ankündigen"),
        B("blockExplosionDropDecay", true, "Block-Explosionen droppen nur einen Teil"),
        B("commandBlockOutput", true, "Befehlsblöcke melden Ausgaben"),
        B("disableElytraMovementCheck", false, "Elytra-Geschwindigkeit nicht prüfen"),
        B("disableRaids", false, "Keine Überfälle"),
        B("doDaylightCycle", true, "Tag/Nacht-Wechsel", "advance_time"),
        B("doEntityDrops", true, "Entities (Loren, Rahmen ...) droppen Items", "entity_drops"),
        B("doFireTick", true, "Feuer breitet sich aus / brennt ab", "fire_spread_radius_around_player"),
        B("doImmediateRespawn", false, "Sofort wiederbeleben ohne Todesbildschirm", "immediate_respawn"),
        B("doInsomnia", false, "Phantome erscheinen", "spawn_phantoms"),
        B("doLimitedCrafting", false, "Nur freigeschaltete Rezepte", "limited_crafting"),
        B("doMobLoot", true, "Mobs droppen Items und Erfahrung", "mob_drops"),
        B("doMobSpawning", true, "Mobs spawnen von selbst", "spawn_mobs"),
        B("doPatrolSpawning", true, "Plünderer-Patrouillen", "spawn_patrols"),
        B("doTileDrops", true, "Abgebaute Blöcke droppen Items", "block_drops"),
        B("doTraderSpawning", true, "Fahrende Händler", "spawn_wandering_traders"),
        B("doVinesSpread", true, "Ranken wachsen", "spread_vines"),
        B("doWardenSpawning", true, "Wärter spawnen", "spawn_wardens"),
        B("doWeatherCycle", true, "Wetterwechsel", "advance_weather"),
        B("drowningDamage", true, "Schaden durch Ertrinken"),
        B("enderPearlsVanishOnDeath", true, "Enderperlen verschwinden beim Tod", "ender_pearls_vanish_on_death"),
        B("fallDamage", true, "Fallschaden"),
        B("fireDamage", true, "Schaden durch Feuer und Lava"),
        B("forgiveDeadPlayers", true, "Wütende Mobs vergessen tote Spieler"),
        B("freezeDamage", true, "Schaden durch Pulverschnee"),
        B("globalSoundEvents", true, "Boss-Sounds überall hörbar"),
        B("keepInventory", false, "Inventar und Erfahrung beim Tod behalten"),
        B("lavaSourceConversion", false, "Lava bildet unendliche Quellen"),
        B("logAdminCommands", true, "Admin-Befehle ins Log"),
        B("mobExplosionDropDecay", true, "Mob-Explosionen droppen nur einen Teil"),
        B("mobGriefing", true, "Mobs zerstören Blöcke (Creeper)"),
        B("naturalRegeneration", true, "Leben regeneriert sich mit vollem Hunger"),
        B("projectilesCanBreakBlocks", true, "Geschosse zerstören Blöcke"),
        B("pvp", true, "Spieler können sich gegenseitig verletzen"),
        B("reducedDebugInfo", false, "F3 zeigt weniger Infos"),
        B("sendCommandFeedback", true, "Befehle melden ihr Ergebnis"),
        B("showDeathMessages", true, "Todesnachrichten im Chat"),
        B("spawnMonsters", true, "Monster spawnen von selbst"),
        B("spectatorsGenerateChunks", true, "Zuschauer laden neue Chunks"),
        B("tntExplodes", true, "TNT kann explodieren"),
        B("tntExplosionDropDecay", false, "TNT-Explosionen droppen nur einen Teil"),
        B("universalAnger", false, "Wütende Mobs greifen alle an"),
        B("waterSourceConversion", true, "Wasser bildet unendliche Quellen"),
        I("maxCommandChainLength", 65536, "Maximale Länge von Befehlsketten"),
        I("maxEntityCramming", 24, "Ab so vielen Entities auf einem Fleck gibt es Schaden"),
        I("playersSleepingPercentage", 100, "Prozent der Spieler, die zum Überspringen der Nacht schlafen müssen"),
        I("randomTickSpeed", 3, "Wachstums-Geschwindigkeit (Pflanzen)"),
        I("snowAccumulationHeight", 1, "Schneehöhe bei Schneefall"),
        I("spawnChunkRadius", 2, "Radius der Spawn-Chunks"),
        I("spawnRadius", 10, "Radius um den Spawnpunkt"),
    };
    return t;
}

}  // namespace

const std::vector<GameRules::Def>& GameRules::defs() const { return table(); }

const GameRules::Def* GameRules::find(const std::string& raw) const {
    std::string n = removePrefix(raw, "minecraft:");
    std::string low = toLower(n);
    for (const auto& d : table())
        if (toLower(d.name) == low || d.snake == low || toSnake(d.name) == low) return &d;
    return nullptr;
}

void GameRules::load(const std::string& worldFolder, const Config& cfg) {
    file_ = joinPath(worldFolder, "gamerules.json");
    for (const auto& d : table()) values_[d.name] = d.def;
    // Startwerte für neue Welten aus config.toml
    values_["doDaylightCycle"] = cfg.daylightCycle;
    values_["keepInventory"] = cfg.keepInventory;
    values_["fallDamage"] = cfg.fallDamage;
    values_["doTileDrops"] = cfg.blockDrops;
    values_["pvp"] = cfg.pvp;
    values_["doMobSpawning"] = cfg.passiveMobs || cfg.hostileMobs;
    values_["spawnMonsters"] = cfg.hostileMobs;
    std::string txt;
    if (!readFile(file_, txt)) return;
    try {
        json::Value v = json::parse(txt);
        for (const auto& [k, x] : v.asObject()) {
            const Def* d = find(k);
            if (!d) continue;
            if (x.isBool()) values_[d->name] = x.asBool() ? 1 : 0;
            else if (x.isNumber()) values_[d->name] = int(x.asInt());
        }
    } catch (const std::exception& e) {
        logx::warn("gamerules.json fehlerhaft: " + std::string(e.what()));
    }
}

void GameRules::save() const {
    if (file_.empty()) return;
    json::Object o;
    for (const auto& d : table()) {
        int v = values_.at(d.name);
        o[d.name] = d.isBool ? json::Value(v != 0) : json::Value(v);
    }
    writeFileAtomic(file_, json::write(json::Value(std::move(o)), true));
}

bool GameRules::b(const std::string& name) const {
    auto it = values_.find(name);
    return it != values_.end() && it->second != 0;
}

int GameRules::i(const std::string& name) const {
    auto it = values_.find(name);
    return it == values_.end() ? 0 : it->second;
}

std::string GameRules::get(const std::string& name) const {
    const Def* d = find(name);
    if (!d) return "";
    int v = values_.at(d->name);
    return d->isBool ? (v ? "true" : "false") : std::to_string(v);
}

bool GameRules::set(const std::string& name, const std::string& raw) {
    const Def* d = find(name);
    if (!d) return false;
    std::string v = toLower(trim(raw));
    if (d->isBool) {
        if (v != "true" && v != "false") return false;
        values_[d->name] = v == "true";
    } else {
        try {
            size_t used = 0;
            long long n = std::stoll(v, &used);
            if (used != v.size() || n < -2147483647LL || n > 2147483647LL) return false;
            values_[d->name] = int(n);
        } catch (...) {
            return false;
        }
    }
    save();
    return true;
}

std::vector<std::pair<std::string, std::string>> GameRules::all() const {
    std::vector<std::pair<std::string, std::string>> out;
    for (const auto& d : table()) out.emplace_back(d.name, get(d.name));
    return out;
}
