#include "vanilla.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

#include "api.hpp"
#include "blocks.hpp"
#include "buffer.hpp"
#include "data.hpp"
#include "entity.hpp"
#include "mobs.hpp"
#include "objects.hpp"
#include "placement.hpp"
#include "physics.hpp"
#include "player.hpp"
#include "server.hpp"
#include "text.hpp"
#include "version.hpp"

namespace vanilla {
namespace {

const double PI = 3.14159265358979;
int fl(double v) { return int(std::floor(v)); }
std::string sec(const char* code) { return std::string(text::SECTION) + code; }

int st(const char* name) { return data::blockState(name); }
int AIR() { return 0; }
int WATER() { static int s = st("water"); return s; }
int LAVA() { static int s = st("lava"); return s; }
int FIRE() { static int s = st("fire"); return s; }
int TNT() { static int s = st("tnt"); return s; }
int GRASS() { static int s = st("grass_block"); return s; }

bool isNight(Server& s) {
    int64_t t = s.world().time % 24000;
    return t >= 13000 && t <= 23000;
}

}  // namespace

// ==== Item-Metadaten =========================================================

json::Value itemMeta(const std::string& item) {
    size_t at = item.find('@');
    if (at == std::string::npos) return json::Object();
    try {
        json::Value v = json::parse(item.substr(at + 1));
        return v.isObject() ? v : json::Value(json::Object());
    } catch (...) {
        return json::Object();
    }
}

int enchantLevel(const std::string& item, const std::string& enchantment) {
    if (item.empty() || item.find('@') == std::string::npos) return 0;
    json::Value m = itemMeta(item);
    return int(m["ench"][ns(enchantment)].asInt(0));
}

namespace {

std::map<std::string, int> enchantments(const std::string& item) {
    std::map<std::string, int> out;
    if (item.find('@') == std::string::npos) return out;
    json::Value m = itemMeta(item);
    const json::Value& e = m["ench"].isObject() ? m["ench"] : m["stored"];
    if (e.isObject())
        for (const auto& [k, v] : e.asObject()) out[k] = int(v.asInt(0));
    return out;
}

// ==== Loot-Tabellen ==========================================================

struct Ctx {
    std::string block, tool;
    std::map<std::string, int> ench;
    std::map<std::string, std::string> props;
    bool killedByPlayer = false, onFire = false;   // Mob-Loot
    Server* server = nullptr;                      // für furnace_smelt
    int depth = 0;                                 // verschachtelte Tabellen
    int level(const std::string& e) const {
        auto it = ench.find(ns(e));
        return it == ench.end() ? 0 : it->second;
    }
};

double number(const json::Value& v) {
    if (v.isNumber()) return v.asDouble();
    if (!v.isObject()) return 1;
    std::string t = removePrefix(v["type"].str("minecraft:uniform"), "minecraft:");
    if (t == "constant") return v["value"].asDouble();
    if (t == "uniform") return randDouble(number(v["min"]), number(v["max"]));
    if (t == "binomial") {
        int n = int(number(v["n"]));
        double p = number(v["p"]);
        int c = 0;
        for (int i = 0; i < n; ++i)
            if (randDouble() < p) ++c;
        return c;
    }
    return 1;
}

std::vector<std::string> strList(const json::Value& v) {
    std::vector<std::string> out;
    if (v.isString()) out.push_back(v.asString());
    else if (v.isArray())
        for (const auto& x : v.asArray())
            if (x.isString()) out.push_back(x.asString());
    return out;
}

bool enchOk(const json::Value& spec, const Ctx& ctx) {
    std::vector<const json::Value*> reqs;
    if (spec.isArray())
        for (const auto& r : spec.asArray()) reqs.push_back(&r);
    else reqs.push_back(&spec);
    for (const json::Value* req : reqs) {
        const json::Value& names = (*req)["enchantments"].isNull() ? (*req)["enchantment"] : (*req)["enchantments"];
        const json::Value& levels = (*req)["levels"];
        int lo = 1, hi = 255;
        if (levels.isObject()) {
            lo = int(levels["min"].asInt(1));
            hi = int(levels["max"].asInt(255));
        } else if (levels.isNumber()) {
            lo = hi = int(levels.asInt());
        }
        bool any = false;
        for (const auto& n : strList(names))
            if (!startsWith(n, "#") && ctx.level(n) >= lo && ctx.level(n) <= hi) any = true;
        if (!any) return false;
    }
    return true;
}

bool itemsMatch(const json::Value& spec, const std::string& tool) {
    if (tool.empty()) return false;
    for (const auto& s : strList(spec))
        if (!startsWith(s, "#") && ns(s) == tool) return true;
    return false;
}

std::string valueText(const json::Value& v) {
    if (v.isString()) return toLower(v.asString());
    if (v.isBool()) return v.asBool() ? "true" : "false";
    if (v.isNumber()) return std::to_string(v.asInt());
    return "";
}

bool cond(const json::Value& c, const Ctx& ctx) {
    if (c.isString()) {   // Verweis auf benanntes Prädikat
        std::string n = removePrefix(c.asString(), "minecraft:");
        if (n == "tool/can_shear") return ctx.tool == "minecraft:shears";
        if (n == "tool/can_silk_touch") return ctx.level("silk_touch") > 0;
        return false;
    }
    if (!c.isObject()) return false;
    std::string t = removePrefix(c["type"].isString() ? c["type"].asString() : c["condition"].str(), "minecraft:");
    if (t == "survives_explosion") return true;
    if (t == "random_chance") return randDouble() < number(c["chance"]);
    if (t == "random_chance_with_enchanted_bonus") {
        int lvl = ctx.level(c["enchantment"].str());
        if (!lvl) return randDouble() < c["unenchanted_chance"].asDouble(0);
        const json::Value& ch = c["enchanted_chance"];
        double chance = ch.isObject() ? ch["base"].asDouble(0) + ch["per_level_above_first"].asDouble(0) * (lvl - 1)
                                      : ch.asDouble(0);
        return randDouble() < chance;
    }
    if (t == "table_bonus") {
        const json::Value& chances = c["chances"];
        if (!chances.isArray() || chances.size() == 0) return false;
        size_t i = std::min<size_t>(size_t(ctx.level(c["enchantment"].str())), chances.size() - 1);
        return randDouble() < chances[i].asDouble();
    }
    if (t == "inverted") return !cond(c["term"], ctx);
    if (t == "any_of" || t == "alternative") {
        for (const auto& x : c["terms"].isArray() ? c["terms"].asArray() : json::Array())
            if (cond(x, ctx)) return true;
        return false;
    }
    if (t == "all_of") {
        for (const auto& x : c["terms"].isArray() ? c["terms"].asArray() : json::Array())
            if (!cond(x, ctx)) return false;
        return true;
    }
    if (t == "match_tool") {
        const json::Value& pred = c["predicate"];
        if (!pred.isObject() || pred.size() == 0) return false;
        const json::Value& ench = !pred["enchantments"].isNull() ? pred["enchantments"] : pred["predicates"]["minecraft:enchantments"];
        if (!ench.isNull() && !enchOk(ench, ctx)) return false;
        if (!pred["items"].isNull() && !itemsMatch(pred["items"], ctx.tool)) return false;
        return true;
    }
    if (t == "block_state_property" || t == "match_block") {   // 26.3 heißt es match_block
        const json::Value& blocks = c["blocks"].isNull() ? c["block"] : c["blocks"];
        if (blocks.isString() && !startsWith(blocks.asString(), "#") && ns(blocks.asString()) != ctx.block) return false;
        const json::Value& state = c["properties"].isNull() ? c["state"] : c["properties"];
        if (state.isObject())
            for (const auto& [k, v] : state.asObject()) {
                if (v.isObject()) continue;
                auto it = ctx.props.find(k);
                if (it == ctx.props.end() || it->second != valueText(v)) return false;
            }
        return true;
    }
    if (t == "reference") return cond(c["name"], ctx);
    if (t == "killed_by_player") return ctx.killedByPlayer;
    if (t == "entity_properties") {   // nur "brennt" wird unterstützt
        const json::Value& flags = c["predicate"]["flags"];
        if (flags.isObject() && flags["is_on_fire"].isBool()) return flags["is_on_fire"].asBool() == ctx.onFire;
        return true;
    }
    if (t == "damage_source_properties") return false;   // z.B. Schallplatten (Creeper von Skelett getötet)
    return false;
}

bool condsOk(const json::Value& obj, const Ctx& ctx) {
    if (!obj["condition"].isNull() && !cond(obj["condition"], ctx)) return false;
    if (obj["conditions"].isArray())
        for (const auto& c : obj["conditions"].asArray())
            if (!cond(c, ctx)) return false;
    return true;
}

std::vector<const json::Value*> functions(const json::Value& obj) {
    std::vector<const json::Value*> out;
    const json::Value& f = obj["functions"].isNull() ? obj["modifier"] : obj["functions"];
    if (f.isArray())
        for (const auto& x : f.asArray()) out.push_back(&x);
    else if (f.isObject()) out.push_back(&f);
    return out;
}

double apply(const std::vector<const json::Value*>& funcs, double count, const Ctx& ctx) {
    for (const json::Value* fp : funcs) {
        const json::Value& f = *fp;
        if (!condsOk(f, ctx)) continue;
        std::string t = removePrefix(f["function"].isString() ? f["function"].asString() : f["type"].str(), "minecraft:");
        if (t == "enchanted_count_increase" || t == "looting_enchant") {   // Plünderung
            int lvl = ctx.level(f["enchantment"].str("minecraft:looting"));
            if (lvl > 0) {
                double per = number(f["count"].isNull() ? json::Value(1) : f["count"]);
                count += std::floor(per * randDouble(0, lvl + 0.999));
                if (f["limit"].isNumber() && f["limit"].asInt() > 0) count = std::min(count, double(f["limit"].asInt()));
            }
        } else if (t == "set_count") {
            double n = number(f["count"]);
            count = f["add"].asBool() ? count + n : n;
        } else if (t == "limit_count") {
            const json::Value& lim = f["limit"];
            if (lim.isObject()) {
                double lo = lim["min"].isNull() ? count : number(lim["min"]);
                double hi = lim["max"].isNull() ? count : number(lim["max"]);
                count = std::max(lo, std::min(hi, count));
            }
        } else if (t == "apply_bonus") {
            int lvl = ctx.level(f["enchantment"].str());
            std::string formula = removePrefix(f["formula"].str(), "minecraft:");
            const json::Value& par = f["parameters"];
            if (formula == "ore_drops" && lvl > 0) {
                count *= std::max(0, randInt(0, lvl + 1) - 1) + 1;
            } else if (formula == "uniform_bonus_count") {
                int mult = int(par["bonusMultiplier"].isNull() ? par["bonus_multiplier"].asInt(1) : par["bonusMultiplier"].asInt(1));
                count += randInt(0, mult * lvl);
            } else if (formula == "binomial_with_bonus_count") {
                int n = lvl + int(par["extra"].asInt(0));
                double p = par["probability"].asDouble(0);
                for (int i = 0; i < n; ++i)
                    if (randDouble() < p) count += 1;
            }
        }
    }
    return count;
}

// -> false = Bedingung nicht erfüllt
bool expand(const json::Value& entry, const Ctx& ctx, std::vector<const json::Value*>& out) {
    if (!condsOk(entry, ctx)) return false;
    std::string t = removePrefix(entry["type"].str(), "minecraft:");
    const json::Array empty;
    const json::Array& children = entry["children"].isArray() ? entry["children"].asArray() : empty;
    if (t == "item" || t == "loot_table") {   // loot_table: verschachtelte Tabelle (z.B. Schafwolle je Farbe)
        out.push_back(&entry);
        return true;
    }
    if (t == "alternatives") {
        for (const auto& c : children) {
            std::vector<const json::Value*> r;
            if (expand(c, ctx, r)) {
                out.insert(out.end(), r.begin(), r.end());
                return true;
            }
        }
        return false;
    }
    if (t == "group") {
        for (const auto& c : children) expand(c, ctx, out);
        return true;
    }
    if (t == "sequence") {
        for (const auto& c : children)
            if (!expand(c, ctx, out)) break;
        return true;
    }
    return true;
}

std::vector<std::pair<std::string, int>> roll(const json::Value& table, const Ctx& ctx) {
    std::vector<std::pair<std::string, int>> drops;
    if (!table["pools"].isArray()) return drops;
    for (const auto& pool : table["pools"].asArray()) {
        if (!condsOk(pool, ctx)) continue;
        int rolls = int(number(pool["rolls"].isNull() ? json::Value(1) : pool["rolls"]));
        for (int r = 0; r < rolls; ++r) {
            std::vector<std::vector<const json::Value*>> choices;
            if (pool["entries"].isArray())
                for (const auto& e : pool["entries"].asArray()) {
                    std::vector<const json::Value*> res;
                    if (expand(e, ctx, res) && !res.empty()) choices.push_back(std::move(res));
                }
            if (choices.empty()) continue;
            for (const json::Value* ie : choices[size_t(randInt(0, int(choices.size()) - 1))]) {
                if (removePrefix((*ie)["type"].str(), "minecraft:") == "loot_table") {
                    const json::Value& v = (*ie)["value"].isNull() ? (*ie)["name"] : (*ie)["value"];
                    if (ctx.depth > 4) continue;
                    Ctx sub = ctx;
                    sub.depth++;
                    std::vector<std::pair<std::string, int>> more;
                    if (v.isString()) {   // "minecraft:entities/sheep/white"
                        std::string key = removePrefix(v.asString(), "minecraft:");
                        const json::Value& nested = startsWith(key, "entities/")
                            ? data::game()["entity_loot"]["minecraft:" + key.substr(9)]
                            : data::game()["loot"]["minecraft:" + removePrefix(key, "blocks/")];
                        if (!nested.isNull()) more = roll(nested, sub);
                    } else if (v.isObject()) {
                        more = roll(v, sub);
                    }
                    drops.insert(drops.end(), more.begin(), more.end());
                    continue;
                }
                double count = apply(functions(*ie), 1, ctx);
                count = apply(functions(pool), count, ctx);
                int n = int(std::floor(count));
                std::string item = ns((*ie)["name"].str());
                for (const json::Value* fn : functions(*ie))   // gebraten, wenn der Mob brannte
                    if (removePrefix((*fn)["function"].isString() ? (*fn)["function"].asString() : (*fn)["type"].str(), "minecraft:") == "furnace_smelt" &&
                        condsOk(*fn, ctx) && ctx.server)
                        if (const auto* c = ctx.server->recipes().cook("smelting", item)) item = c->result;
                if (n > 0) drops.emplace_back(item, n);
            }
        }
    }
    return drops;
}

const json::Value& blockInfo(const std::string& block) { return data::game()["blocks"][block]; }

bool rightTool(const std::string& block, const std::string& tool) {
    const json::Value& tools = blockInfo(block)["tools"];
    if (!tools.isArray() || tools.size() == 0) return true;
    std::string base = data::baseItem(tool);
    for (const auto& t : tools.asArray())
        if (t.str() == base) return true;
    return false;
}

}  // namespace

std::vector<std::pair<std::string, int>> blockDrops(const std::string& block, const std::string& tool, bool ignoreTool) {
    if (!ignoreTool && !rightTool(block, tool)) return {};
    const json::Value& table = data::game()["loot"][block];
    if (table.isNull()) return {};
    Ctx ctx;
    ctx.block = block;
    ctx.tool = tool.empty() ? "" : data::baseItem(tool);
    ctx.ench = enchantments(tool);
    int state = data::blockState(block);
    (void)state;
    return roll(table, ctx);
}

std::vector<std::pair<std::string, int>> entityDrops(Server& s, const std::string& table, const std::string& weapon, bool killedByPlayer,
                                                     bool onFire) {
    const json::Value& t = data::game()["entity_loot"][ns(table)];
    if (t.isNull()) return {};
    Ctx ctx;
    ctx.tool = weapon.empty() ? "" : data::baseItem(weapon);
    ctx.ench = enchantments(weapon);
    ctx.killedByPlayer = killedByPlayer;
    ctx.onFire = onFire;
    ctx.server = &s;
    return roll(t, ctx);
}

int blockXp(const std::string& block, const std::string& tool) {
    static const std::map<std::string, std::pair<int, int>> ORE_XP = {
        {"coal_ore", {0, 2}},          {"deepslate_coal_ore", {0, 2}},     {"nether_gold_ore", {0, 1}},
        {"diamond_ore", {3, 7}},       {"deepslate_diamond_ore", {3, 7}},  {"emerald_ore", {3, 7}},
        {"deepslate_emerald_ore", {3, 7}}, {"lapis_ore", {2, 5}},          {"deepslate_lapis_ore", {2, 5}},
        {"nether_quartz_ore", {2, 5}}, {"redstone_ore", {1, 5}},           {"deepslate_redstone_ore", {1, 5}},
        {"sculk", {1, 1}},             {"spawner", {15, 43}},
    };
    auto it = ORE_XP.find(removePrefix(block, "minecraft:"));
    if (it == ORE_XP.end() || enchantLevel(tool, "silk_touch") || !rightTool(block, tool)) return 0;
    return randInt(it->second.first, it->second.second);
}

// ==== Kampfwerte =============================================================

namespace {

const std::map<std::string, float> WEAPONS = {
    {"wooden_sword", 4}, {"golden_sword", 4}, {"stone_sword", 5}, {"copper_sword", 5}, {"iron_sword", 6},
    {"diamond_sword", 7}, {"netherite_sword", 8}, {"wooden_axe", 7}, {"golden_axe", 7}, {"stone_axe", 9},
    {"copper_axe", 9}, {"iron_axe", 9}, {"diamond_axe", 9}, {"netherite_axe", 10}, {"trident", 9}, {"mace", 6},
};
const std::set<std::string> UNDEAD = {"zombie", "skeleton", "husk", "drowned", "stray", "wither_skeleton", "zombified_piglin", "phantom"};
const std::set<std::string> ARTHROPODS = {"spider", "cave_spider", "bee", "silverfish", "endermite"};
struct ArmorMat {
    int v[4];   // Schuhe, Hose, Brust, Helm
    int tough;
};
const std::map<std::string, ArmorMat> ARMOR = {
    {"leather", {{1, 2, 3, 1}, 0}}, {"chainmail", {{1, 4, 5, 2}, 0}}, {"golden", {{1, 3, 5, 2}, 0}},
    {"copper", {{1, 3, 4, 2}, 0}},  {"iron", {{2, 5, 6, 2}, 0}},      {"diamond", {{3, 6, 8, 3}, 2}},
    {"netherite", {{3, 6, 8, 3}, 3}},
};
const std::map<std::string, int> PIECES = {{"boots", 0}, {"leggings", 1}, {"chestplate", 2}, {"helmet", 3}};
const int ARMOR_SLOTS[4] = {5, 6, 7, 8};
const std::set<std::string> ARMOR_CAUSES = {"attack", "explosion", "lava", "generic_attack", "projectile"};

float weaponDamage(const std::string& item, const std::string& targetType) {
    std::string base = removePrefix(data::baseItem(item), "minecraft:");
    auto it = WEAPONS.find(base);
    float dmg = it == WEAPONS.end() ? 1.0f : it->second;
    if (!item.empty()) {
        int sharp = enchantLevel(item, "sharpness");
        if (sharp) dmg += 0.5f * float(sharp) + 0.5f;
        if (UNDEAD.count(targetType)) dmg += 2.5f * float(enchantLevel(item, "smite"));
        if (ARTHROPODS.count(targetType)) dmg += 2.5f * float(enchantLevel(item, "bane_of_arthropods"));
    }
    return dmg;
}

float maceSmashBonus(double fall, int density) {
    // 4 pro Block für die ersten 3 Blöcke, 2 pro Block für die nächsten 5, danach 1; Wucht +0,5 je Block und Stufe
    if (fall <= 1.5) return 0;
    double bonus = 4 * std::min(fall, 3.0) + 2 * std::max(0.0, std::min(fall, 8.0) - 3) + std::max(0.0, fall - 8);
    return float(bonus + 0.5 * density * fall);
}

void armorValues(Player& p, int& points, int& tough) {
    points = tough = 0;
    for (int slot : ARMOR_SLOTS) {
        auto it = p.inventory.find(slot);
        if (it == p.inventory.end()) continue;
        std::string n = removePrefix(data::baseItem(it->second.first), "minecraft:");
        if (n == "turtle_helmet") {
            points += 2;
            continue;
        }
        size_t us = n.rfind('_');
        if (us == std::string::npos) continue;
        auto m = ARMOR.find(n.substr(0, us));
        auto pc = PIECES.find(n.substr(us + 1));
        if (m != ARMOR.end() && pc != PIECES.end()) {
            points += m->second.v[pc->second];
            tough += m->second.tough;
        }
    }
}

int protectionPoints(Player& p, const std::string& cause) {
    int epf = 0;
    for (int slot : ARMOR_SLOTS) {
        auto it = p.inventory.find(slot);
        if (it == p.inventory.end()) continue;
        auto e = enchantments(it->second.first);
        auto lv = [&](const char* n) { auto f = e.find(ns(n)); return f == e.end() ? 0 : f->second; };
        if (cause != "void" && cause != "starve") epf += lv("protection");
        if (cause == "fire" || cause == "lava") epf += 2 * lv("fire_protection");
        if (cause == "explosion") epf += 2 * lv("blast_protection");
        if (cause == "projectile") epf += 2 * lv("projectile_protection");
        if (cause == "fall") epf += 3 * lv("feather_falling");
    }
    return std::min(20, epf);
}

int armorLevel(Player& p, const std::string& enchantment) {
    int best = 0;
    for (int slot : ARMOR_SLOTS) {
        auto it = p.inventory.find(slot);
        if (it != p.inventory.end()) best = std::max(best, enchantLevel(it->second.first, enchantment));
    }
    return best;
}

}  // namespace

float reduceDamage(Player& p, float amount, const std::string& cause, int breach) {
    // Vanilla: erst Rüstung (mit Härte, Durchbruch senkt sie um 15 % je Stufe), dann Schutz-Verzauberungen
    if (ARMOR_CAUSES.count(cause)) {
        int points, tough;
        armorValues(p, points, tough);
        if (points) {
            double eff = std::min(20.0, std::max(points / 5.0, points - 4.0 * amount / (tough + 8.0)));
            eff *= std::max(0.0, 1.0 - 0.15 * breach);
            amount *= float(1.0 - eff / 25.0);
        }
    }
    int epf = protectionPoints(p, cause);
    if (epf) amount *= 1.0f - float(epf) / 25.0f;
    return amount;
}

namespace {

// ==== TNT ====================================================================

class PrimedTnt : public Entity {
public:
    PrimedTnt(Server& s, double px, double py, double pz, int f) : Entity(s, "tnt", px, py, pz), fuse(f) {
        gravity = 0.04;
        double a = randDouble(0, 2 * PI);
        vx = -std::sin(a) * 0.02;
        vy = 0.2;
        vz = -std::cos(a) * 0.02;
    }
    int fuse;
    void tick() override {
        Entity::tick();
        if (--fuse <= 0 && !removed) {
            remove();
            json::Object ed;
            ed["entity"] = api::entitySnap(this);
            if (!api::fire(server, "entity_explode", nullptr, std::move(ed))) explode(server, x, y + 0.0625, z, 4.0);
        }
    }
};

bool unbreakable(int state) {
    static std::set<int> s = [] {
        std::set<int> out;
        for (const char* n : {"bedrock", "water", "lava", "obsidian", "crying_obsidian", "end_portal_frame", "barrier",
                              "reinforced_deepslate", "end_portal", "nether_portal"}) {
            const auto it = data::BLOCKS.find(n);
            if (it != data::BLOCKS.end())
                for (int i = it->second.lo; i <= it->second.hi; ++i) out.insert(i);
        }
        return out;
    }();
    return s.count(state) > 0;
}

float explosionDamage(double power, double dist) {
    double impact = std::max(0.0, 1 - dist / (power * 2));
    return float((impact * impact + impact) / 2 * 7 * power * 2 + 1);
}

// ==== Spieler-Tick ===========================================================

void tickPlayers(Server& s) {
    int64_t tick = s.tick();
    const Config& cfg = s.config();
    World& w = s.world();
    for (Player* p : s.players()) {
        if (p->dead || !p->isSurvival()) {
            if (p->air < 300 && !p->dead) p->setAir(300);
            continue;
        }
        int fx = fl(p->x), fz = fl(p->z);
        int feet = w.getBlock(fx, fl(p->y), fz);
        int eyes = w.getBlock(fx, fl(p->y + 1.62), fz);

        // Ertrinken
        if (eyes == WATER()) {
            int resp = armorLevel(*p, "respiration");
            int air = p->air - ((resp && randDouble() < double(resp) / (resp + 1)) ? 0 : 1);
            if (air <= -20) {
                air = 0;
                p->damage(2.0f, "drown");
            }
            p->setAir(air);
        } else if (p->air < 300) {
            p->setAir(std::min(300, p->air + 4));
        }
        if (p->dead) continue;

        // Lava und Feuer
        if (feet == LAVA() || w.getBlock(fx, fl(p->y + 0.5), fz) == LAVA()) {
            bool was = p->fireTicks > 0;
            p->fireTicks = std::max(p->fireTicks, 300);
            if (!was) p->sendMetadata();
            p->damage(4.0f, "lava");
        } else if (feet == FIRE()) {
            bool was = p->fireTicks > 0;
            p->fireTicks = std::max(p->fireTicks, 160);
            if (!was) p->sendMetadata();
            p->damage(1.0f, "fire");
        } else if (feet == WATER() && p->fireTicks > 0) {
            p->fireTicks = 1;
        } else if (p->fireTicks > 0 && p->fireTicks % 20 == 0) {
            p->damage(1.0f, "fire");
        }
        if (p->dead) continue;

        // Leere
        if (p->y < -128) p->damage(4.0f, "void");
        if (p->dead) continue;

        // Essen
        if (p->usingItem) {
            std::string base = data::baseItem(p->usingItem->item);
            const json::Value& food = data::game()["foods"][base];
            bool golden = base == "minecraft:golden_apple" || base == "minecraft:enchanted_golden_apple";
            if (food.isArray() && (p->food < 20 || golden || !cfg.hunger)) {
                if (tick - p->usingItem->start >= 32) {
                    int slot = p->usingItem->hand == 0 ? 36 + p->heldSlot : 45;
                    auto it = p->inventory.find(slot);
                    json::Object cd;
                    cd["item"] = p->usingItem->item;
                    if (it != p->inventory.end() && it->second.first == p->usingItem->item &&
                        api::fire(s, "player_item_consume", p, std::move(cd))) {
                        p->finishUsingItem();
                        p->syncInventory();
                        continue;
                    }
                    if (it != p->inventory.end() && it->second.first == p->usingItem->item) {
                        p->setSlot(slot, it->second.first, it->second.second - 1);
                        p->setFood(p->food + int(food[0].asDouble()), p->saturation + float(food[1].asDouble()));
                        if (base == "minecraft:golden_apple") p->heal(4);
                        if (base == "minecraft:enchanted_golden_apple") p->heal(8);
                        p->playSound("minecraft:entity.player.burp", 0.5f, float(randDouble(0.9, 1.0)));
                        if (base == "minecraft:mushroom_stew" || base == "minecraft:rabbit_stew" ||
                            base == "minecraft:beetroot_soup" || base == "minecraft:suspicious_stew")
                            p->addItem("minecraft:bowl", 1);
                    }
                    p->finishUsingItem();
                }
            } else if (base == "minecraft:milk_bucket") {
                if (tick - p->usingItem->start >= 32) {
                    p->fireTicks = 0;
                    if (p->gamemode == 0) p->setHeldItem("minecraft:bucket");
                    p->finishUsingItem();
                }
            } else if (!food.isArray() && base != "minecraft:bow" && base != "minecraft:crossbow" && base != "minecraft:trident" &&
                       base != "minecraft:shield" && base != "minecraft:spyglass") {
                p->usingItem.reset();
            }
        }

        // Hunger und Regeneration
        bool regen = s.gameRules().b("naturalRegeneration");
        if (!cfg.hunger) {
            if (regen && p->health < 20 && tick % 40 == 0) p->heal(1);
            continue;
        }
        if (cfg.difficulty == 0) {
            if (tick % 20 == 0) {
                if (regen) p->heal(1);
                if (p->food < 20) p->setFood(p->food + 1);
            }
            continue;
        }
        int& timer = p->regenTimer;
        ++timer;
        if (!regen && p->food > 0) {
            timer = 0;
        } else if (p->food >= 20 && p->saturation > 0 && p->health < 20) {
            if (timer >= 10) {
                timer = 0;
                float use = std::min(p->saturation, 6.0f);
                p->heal(1);
                p->addExhaustion(use);
            }
        } else if (p->food >= 18 && p->health < 20) {
            if (timer >= 80) {
                timer = 0;
                p->heal(1);
                p->addExhaustion(6.0f);
            }
        } else if (p->food <= 0) {
            if (timer >= 80) {
                timer = 0;
                int floor = cfg.difficulty == 1 ? 10 : cfg.difficulty == 2 ? 1 : 0;
                if (p->health > float(floor)) p->damage(1.0f, "starve");
            }
        } else {
            timer = 0;
        }
    }
}

void maceSmash(Player& p, Entity* targetEntity, Player* targetPlayer, const std::string& item, double fall) {
    Server& s = p.server;
    double tx = targetEntity ? targetEntity->x : targetPlayer->x;
    double ty = targetEntity ? targetEntity->y : targetPlayer->y;
    double tz = targetEntity ? targetEntity->z : targetPlayer->z;
    p.fallDistance = 0;   // der Treffer fängt den Fall ab
    p.push(0.0, 0.01, 0.0);
    bool heavy = fall > 5;
    s.playSound(heavy ? "minecraft:item.mace.smash_ground_heavy" : "minecraft:item.mace.smash_ground", tx, ty, tz, 1.0f, 1.0f, 7);
    // Schockwelle: Wesen in 3,5 Blöcken um das Ziel werden weggestoßen
    auto strengthAt = [&](double ox, double oy, double oz, double& dx, double& dz, double& str) {
        dx = ox - tx;
        dz = oz - tz;
        double dist = std::sqrt(dx * dx + (oy - ty) * (oy - ty) + dz * dz);
        if (dist > 3.5 || dist < 1e-3) return false;
        str = (3.5 - dist) * 0.7 * (heavy ? 2 : 1) / 3.5;
        double d = std::hypot(dx, dz);
        if (d < 1e-6) d = 1;
        dx /= d;
        dz /= d;
        return true;
    };
    for (Entity* e : s.entities()) {
        auto* m = dynamic_cast<Mob*>(e);
        if (!m || m->dead || m == targetEntity) continue;
        double dx, dz, str;
        if (strengthAt(m->x, m->y, m->z, dx, dz, str)) {
            m->vx = dx * str;
            m->vy = 0.7 * str;
            m->vz = dz * str;
        }
    }
    for (Player* o : s.players()) {
        if (o == &p || o == targetPlayer || o->dead || !o->isSurvival()) continue;
        double dx, dz, str;
        if (strengthAt(o->x, o->y, o->z, dx, dz, str)) o->push(dx * str, 0.7 * str, dz * str);
    }
    int burst = enchantLevel(item, "wind_burst");
    if (burst) {   // Windstoß: Spieler fliegt wieder hoch
        p.push(0.0, 0.6 + 0.35 * burst, 0.0);
        s.playSound("minecraft:entity.wind_charge.wind_burst", p.x, p.y, p.z, 1.0f, 1.0f, 7);
    }
}

}  // namespace

// ==== Öffentliche Funktionen =================================================

bool isMobType(const std::string& t) { return mobs::isMob(t); }

std::vector<std::string> mobTypes() { return mobs::types(); }

void spawnMobs(Server& s, const std::string& kind, double x, double y, double z, int count) {
    for (int i = 0; i < count; ++i) {
        double mx = x + (count > 1 ? randDouble(-1, 1) : 0), mz = z + (count > 1 ? randDouble(-1, 1) : 0);
        if (s.bridge().listens("creature_spawn")) {   // Plugins dürfen das Spawnen verhindern
            json::Object cd, ent;
            ent["id"] = -1;
            ent["type"] = kind;
            ent["x"] = mx;
            ent["y"] = y;
            ent["z"] = mz;
            cd["entity"] = json::Value(std::move(ent));
            if (api::fire(s, "creature_spawn", nullptr, std::move(cd))) continue;
        }
        mobs::spawn(s, kind, mx, y, mz);
    }
}

void primeTnt(Server& s, int x, int y, int z, int fuse) {
    if (!s.gameRules().b("tntExplodes")) return;   // Spielregel: TNT bleibt ein Block
    s.setBlockState(x, y, z, AIR());
    spawnTnt(s, x + 0.5, y, z + 0.5, fuse);
    s.playSound("minecraft:entity.tnt.primed", x + 0.5, y + 0.5, z + 0.5, 1.0f, 1.0f, 4);
}

void spawnTnt(Server& s, double x, double y, double z, int fuse) { s.spawnEntity(std::make_unique<PrimedTnt>(s, x, y, z, fuse)); }

void explode(Server& s, double cx, double cy, double cz, double power, bool breakBlocks) {
    World& w = s.world();
    s.playSound("minecraft:entity.generic.explode", cx, cy, cz, 4.0f, float(randDouble(0.7, 0.9)), 4);
    if (breakBlocks) {
        double r = power;
        for (int x = fl(cx - r); x <= fl(cx + r); ++x)
            for (int y = fl(cy - r); y <= fl(cy + r); ++y)
                for (int z = fl(cz - r); z <= fl(cz + r); ++z) {
                    double d2 = (x + 0.5 - cx) * (x + 0.5 - cx) + (y + 0.5 - cy) * (y + 0.5 - cy) + (z + 0.5 - cz) * (z + 0.5 - cz);
                    if (d2 > r * r * randDouble(0.5, 1.0)) continue;
                    int state = w.getBlock(x, y, z);
                    if (state == AIR() || unbreakable(state)) continue;
                    if (state == TNT()) {   // Kettenreaktion
                        primeTnt(s, x, y, z, randInt(10, 30));
                        continue;
                    }
                    std::string name = data::blockName(state);
                    blocks::dropContents(s, x, y, z);
                    s.setBlockState(x, y, z, AIR());
                    if (s.gameRules().b("doTileDrops") && randDouble() < 1 / power)   // wie Vanilla: nur ein Teil droppt
                        for (auto& [item, count] : blockDrops(name, "", true)) s.dropItem(x + 0.5, y + 0.5, z + 0.5, item, count);
                }
    }
    for (Player* p : s.players()) {
        double dx = p->x - cx, dy = p->y + 0.9 - cy, dz = p->z - cz;
        double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (dist >= power * 2) continue;
        p->damage(explosionDamage(power, dist), "explosion");
        double n = dist > 1e-6 ? dist : 1;
        double str = (1 - dist / (power * 2)) * 1.2;
        p->push(dx / n * str, dy / n * str + 0.2, dz / n * str);
    }
    for (Entity* e : s.entities()) {
        double dist = e->distanceTo(cx, cy, cz);
        if (dist >= power * 2) continue;
        if (auto* m = dynamic_cast<Mob*>(e)) {
            m->damage(explosionDamage(power, dist), 0, "explosion");
        } else if (!dynamic_cast<PrimedTnt*>(e)) {   // Items fliegen weg
            double n = dist > 1e-6 ? dist : 1;
            e->vx = (e->x - cx) / n * 0.5;
            e->vy = 0.4;
            e->vz = (e->z - cz) / n * 0.5;
        }
    }
}

void tick(Server& s) {
    tickPlayers(s);
    mobs::tick(s);
}

void onMove(Player& p, double fromX, double fromY, double fromZ) {
    if (!p.isSurvival()) return;
    double dist = std::hypot(p.x - fromX, p.z - fromZ);
    if (p.sprinting) p.addExhaustion(float(0.1 * dist));
    if (p.y - fromY > 0.35 && p.onGround) p.addExhaustion(p.sprinting ? 0.2f : 0.05f);   // Sprung
}

void onLand(Player& p, double fall) {
    if (p.server.gameRules().b("fallDamage") && fall > 3) p.damage(float(std::ceil(fall - 3)), "fall");
}

void onBlockBreak(Player& p, int x, int y, int z, int state) {
    Server& s = p.server;
    if (!s.gameRules().b("doTileDrops")) return;
    std::string block = data::blockName(state);
    std::string tool = p.heldItem();
    for (auto& [item, count] : blockDrops(block, tool)) s.dropItem(x + 0.5, y + 0.3, z + 0.5, item, count);
    p.giveXp(blockXp(block, tool));
    p.addExhaustion(0.005f);
}

void onUseItem(Player& p, int hand, const std::string& item) {
    if (hand != 0 || p.dead || p.gamemode == 3 || item.empty()) return;
    if (objects::useItem(p, item)) return;
    physics::useBucket(p, item);
}

void ignite(Server& s, int x, int y, int z, bool& ok) {
    // Feuer braucht etwas darunter; auf normalem Boden geht es nach ein paar Sekunden wieder aus
    World& w = s.world();
    int below = w.getBlock(x, y - 1, z);
    ok = false;
    if (w.getBlock(x, y, z) != AIR() || below == AIR() || below == WATER() || below == LAVA()) return;
    s.setBlockState(x, y, z, FIRE());
    ok = true;
    s.runLater(randInt(60, 140), [&s, x, y, z] {
        if (s.world().getBlock(x, y, z) == FIRE()) s.setBlockState(x, y, z, AIR());
    });
}

bool useItemOnBlock(Player& p, int hand, const std::string& item, int cx, int cy, int cz, int tx, int ty, int tz) {
    Server& s = p.server;
    std::string base = data::baseItem(item);
    int slot = hand == 0 ? 36 + p.heldSlot : 45;
    if (p.gamemode != 2 && hand == 0) {   // Rüstungsständer, Rahmen, Gemälde, Loren, Boote
        int face = 1;
        for (int f = 0; f < 6; ++f)
            if (placement::FACE_OFFSETS[f][0] == tx - cx && placement::FACE_OFFSETS[f][1] == ty - cy && placement::FACE_OFFSETS[f][2] == tz - cz) face = f;
        if (objects::useItemOnBlock(p, item, cx, cy, cz, face)) return true;
    }
    if (base == "minecraft:flint_and_steel" || base == "minecraft:fire_charge") {
        if (p.gamemode == 2) return true;
        int clicked = s.world().getBlock(cx, cy, cz);
        if (clicked == TNT()) {
            primeTnt(s, cx, cy, cz);
        } else {
            bool ok;
            ignite(s, tx, ty, tz, ok);
            if (!ok) return true;
        }
        if (base == "minecraft:fire_charge") {
            if (p.gamemode == 0) p.removeItem("minecraft:fire_charge", 1);
            s.playSound("minecraft:item.firecharge.use", cx + 0.5, cy + 0.5, cz + 0.5, 1.0f, 1.0f, 4);
        } else {
            s.playSound("minecraft:item.flintandsteel.use", cx + 0.5, cy + 0.5, cz + 0.5, 1.0f, float(randDouble(0.8, 1.2)), 4);
        }
        return true;
    }
    if (base == "minecraft:bucket" || base == "minecraft:water_bucket" || base == "minecraft:lava_bucket")
        return true;   // der Client schickt zusätzlich "Item benutzen" - dort wird gezielt
    if (endsWith(base, "_hoe") && p.gamemode != 2 && physics::till(s, cx, cy, cz)) return true;
    if (base == "minecraft:bone_meal" && physics::isGrowable(s.world().getBlock(cx, cy, cz))) {
        if (physics::grow(s, cx, cy, cz, true)) {
            s.playSound("minecraft:item.bone_meal.use", cx + .5, cy + .5, cz + .5, 1.0f, 1.0f, 7);
            if (p.gamemode != 1) p.removeItem("minecraft:bone_meal", 1);
        }
        return true;
    }
    if (endsWith(base, "_spawn_egg")) {
        std::string kind = removePrefix(base, "minecraft:");
        kind = kind.substr(0, kind.size() - 10);
        if (!isMobType(kind)) return true;
        spawnMobs(s, kind, tx + 0.5, ty, tz + 0.5, 1);
        if (p.gamemode == 0) {
            auto it = p.inventory.find(slot);
            if (it != p.inventory.end()) p.setSlot(slot, it->second.first, it->second.second - 1);
        }
        return true;
    }
    return false;
}

void onAttack(Player& p, int target) {
    Server& s = p.server;
    Player* tp = s.playerById(target);
    if (Entity* obj = s.getEntity(target); objects::isObject(obj)) {
        objects::attack(p, obj);
        return;
    }
    Mob* mob = dynamic_cast<Mob*>(s.getEntity(target));
    if (!tp && !mob) return;
    std::string item = p.heldItem();
    float dmg = weaponDamage(item, mob ? mob->type : "");
    double fall = p.fallDistance;
    if (fall > 0 && !p.onGround && !p.sprinting) dmg *= 1.5f;   // kritischer Treffer
    bool mace = !item.empty() && data::baseItem(item) == "minecraft:mace";
    bool smash = mace && fall > 1.5 && !p.flying;
    if (smash) dmg += maceSmashBonus(fall, enchantLevel(item, "density"));
    int breach = mace ? enchantLevel(item, "breach") : 0;
    if (tp) {
        if (!s.gameRules().b("pvp")) return;
        tp->breach = breach;
        bool hit = tp->damage(dmg, "attack", p.entityId);
        tp->breach = 0;
        if (!hit) return;
    } else if (!mob->damage(dmg, p.entityId, "attack")) {
        return;
    }
    if (smash) maceSmash(p, mob, tp, item, fall);
    int fire = enchantLevel(item, "fire_aspect");
    if (fire) {
        if (tp) {
            bool was = tp->fireTicks > 0;
            tp->fireTicks = std::max(tp->fireTicks, 80 * fire);
            if (!was) tp->sendMetadata();
        } else {
            mob->fireTicks = std::max(mob->fireTicks, 80 * fire);
        }
    }
    int kb = enchantLevel(item, "knockback");
    if (kb) {
        double tx = tp ? tp->x : mob->x, tz = tp ? tp->z : mob->z;
        double dx = tx - p.x, dz = tz - p.z;
        double d = std::hypot(dx, dz);
        if (d < 1e-6) d = 1;
        if (tp) tp->push(dx / d * 0.5 * kb, 0.1, dz / d * 0.5 * kb);
        else mob->knockback(p.x, p.z, 0.4 + 0.5 * kb);
    }
}

void onInteractEntity(Player& p, int target, int hand) {
    Entity* e = p.server.getEntity(target);
    if (objects::isObject(e)) {
        objects::interact(p, e, hand);
        return;
    }
    if (hand != 0) return;
    if (e && e->type == "cow" && data::baseItem(p.heldItem()) == "minecraft:bucket") {
        auto it = p.inventory.find(36 + p.heldSlot);
        if (p.gamemode == 1) {
            p.addItem("minecraft:milk_bucket", 1);
        } else if (it->second.second <= 1) {
            p.setHeldItem("minecraft:milk_bucket");
        } else {
            p.setSlot(36 + p.heldSlot, it->second.first, it->second.second - 1);
            if (p.addItem("minecraft:milk_bucket", 1) > 0) p.dropStack("minecraft:milk_bucket", 1);
        }
        p.playSound("minecraft:entity.cow.milk");
    }
}

void onMobDeath(Server& s, Mob& mob, int killer) { mobs::onDeath(s, mob, killer); }

// ==== Befehle ================================================================

void init(Server& server) {
    Commands& cmds = server.commands();
    auto target = [&server](CommandSender& s, const std::vector<std::string>& a, size_t i) -> Player* {
        Player* t = a.size() > i ? server.getPlayer(a[i]) : s.asPlayer();
        if (!t) throw CommandError("Spieler nicht gefunden");
        return t;
    };
    cmds.add({"heal", "Heilt dich vollständig", "/heal [spieler]", {}, true,
              [target](CommandSender& s, const std::vector<std::string>& a) {
                  Player* t = target(s, a, 0);
                  t->setHealth(20);
                  t->setFood(20, 5.0f);
                  t->fireTicks = 0;
                  t->sendMetadata();
                  s.sendMessage(sec("a") + t->name + " wurde geheilt.");
              },
              nullptr});
    cmds.add({"gamerule", "Zeigt oder ändert eine Spielregel", "/gamerule [regel] [wert]", {}, true,
              [&server](CommandSender& s, const std::vector<std::string>& a) {
                  GameRules& rules = server.gameRules();
                  if (a.empty()) {
                      std::string line;
                      for (const auto& [n, v] : rules.all()) line += (line.empty() ? "" : sec("7") + ", ") + sec("e") + n + sec("7") + "=" + sec("f") + v;
                      s.sendMessage(sec("6") + "Spielregeln: " + line);
                      return;
                  }
                  const GameRules::Def* d = rules.find(a[0]);
                  if (!d) throw CommandError("Unbekannte Spielregel: " + a[0]);
                  if (a.size() == 1) {
                      s.sendMessage(sec("e") + d->name + sec("7") + " = " + sec("f") + rules.get(d->name) + sec("8") + "  (" + d->description + ")");
                      return;
                  }
                  if (!rules.set(d->name, a[1]))
                      throw CommandError(d->isBool ? "Wert muss true oder false sein" : "Wert muss eine ganze Zahl sein");
                  for (Player* p : server.players()) p->sendRuleFlags();
                  if (d->name == "doDaylightCycle") server.sendTime();
                  s.sendMessage(sec("a") + "Spielregel " + d->name + " ist jetzt " + rules.get(d->name));
              },
              [&server](CommandSender&, const std::vector<std::string>& a) {
                  std::vector<std::string> out;
                  if (a.size() <= 1) {
                      for (const auto& d : server.gameRules().defs()) out.push_back(d.name);
                  } else if (a.size() == 2) {
                      const GameRules::Def* d = server.gameRules().find(a[0]);
                      if (d && d->isBool) out = {"true", "false"};
                      else if (d) out = {std::to_string(d->def)};
                  }
                  return out;
              }});
    cmds.add({"clear", "Leert das Inventar", "/clear [spieler] [item]", {}, true,
              [target](CommandSender& s, const std::vector<std::string>& a) {
                  Player* t = target(s, a, 0);
                  int removed = 0;
                  if (a.size() > 1) {
                      removed = t->removeItem(ns(a[1]), 1 << 30);
                  } else {
                      for (const auto& [slot, st] : t->inventory) removed += st.second;
                      t->inventory.clear();
                      t->syncInventory();
                  }
                  s.sendMessage(sec("a") + std::to_string(removed) + " Items von " + t->name + " entfernt.");
              },
              nullptr});
    cmds.add({"kill", "Tötet dich, einen Spieler oder Entities", "/kill [spieler | @e | @e[type=zombie]]", {}, true,
              [target, &server](CommandSender& s, const std::vector<std::string>& a) {
                  if (!a.empty() && startsWith(a[0], "@e")) {   // alle Entities (außer Spieler), optional nur ein Typ
                      std::string type;
                      size_t eq = a[0].find("type=");
                      if (eq != std::string::npos) {
                          type = removePrefix(a[0].substr(eq + 5), "minecraft:");
                          size_t end = type.find_first_of(",]");
                          if (end != std::string::npos) type = type.substr(0, end);
                      }
                      int n = 0;
                      for (Entity* e : server.entities()) {
                          if (!type.empty() && e->type != type) continue;
                          e->remove();
                          ++n;
                      }
                      s.sendMessage(sec("a") + std::to_string(n) + " Entities entfernt.");
                      return;
                  }
                  Player* t = target(s, a, 0);
                  t->kill(t->name + " ist gestorben");
              },
              nullptr});
    cmds.add({"xp", "Gibt Erfahrung oder setzt die Stufe", "/xp <anzahl>[L] [spieler]  |  /xp set <stufe> [spieler]",
              {"experience"}, true,
              [target](CommandSender& s, const std::vector<std::string>& args) {
                  std::vector<std::string> a = args;
                  if (a.empty()) throw CommandError("Benutzung: /xp <anzahl>[L] [spieler]  oder  /xp set <stufe> [spieler]");
                  bool setting = a[0] == "set";
                  if (setting) a.erase(a.begin());
                  if (a.empty()) throw CommandError("Stufe fehlt");
                  std::string amount = a[0];
                  bool levels = !amount.empty() && (amount.back() == 'l' || amount.back() == 'L');
                  int n;
                  try {
                      n = std::stoi(levels ? amount.substr(0, amount.size() - 1) : amount);
                  } catch (...) {
                      throw CommandError("Keine Zahl: " + amount);
                  }
                  Player* t = target(s, a, 1);
                  if (setting) t->setLevel(n);
                  else if (levels) {
                      if (n >= 0) t->setLevel(t->level() + n);
                      else t->takeLevels(-n);
                  } else if (n >= 0) {
                      t->giveXp(n);
                  } else {
                      t->xpTotal = std::max(0, t->xpTotal + n);
                      t->sendExperience();
                  }
                  s.sendMessage(sec("a") + t->name + " hat jetzt Stufe " + std::to_string(t->level()) + " (" +
                                std::to_string(t->xpTotal) + " Punkte).");
              },
              nullptr});
    cmds.add({"summon", "Erzeugt einen Mob oder TNT", "/summon <mob|tnt> [anzahl]", {"spawnmob"}, true,
              [&server](CommandSender& s, const std::vector<std::string>& a) {
                  Player* p = s.asPlayer();
                  if (!p) throw CommandError("Nur Spieler können das benutzen.");
                  int count = 1;
                  if (a.size() > 1) {
                      try {
                          count = std::max(1, std::min(20, std::stoi(a[1])));
                      } catch (...) {
                      }
                  }
                  std::string kind = a.empty() ? "" : toLower(removePrefix(a[0], "minecraft:"));
                  if (kind == "tnt") {
                      for (int i = 0; i < count; ++i) spawnTnt(server, p->x, p->y, p->z);
                      s.sendMessage(sec("a") + std::to_string(count) + "x TNT angezündet.");
                      return;
                  }
                  if (objects::summon(server, kind, p->x, p->y, p->z, p->yaw)) {
                      s.sendMessage(sec("a") + kind + " erzeugt.");
                      return;
                  }
                  if (!isMobType(kind)) {
                      std::string list;
                      for (const auto& m : mobTypes()) list += m + ", ";
                      throw CommandError("Mobs: " + list + "tnt");
                  }
                  spawnMobs(server, kind, p->x, p->y, p->z, count);
                  s.sendMessage(sec("a") + std::to_string(count) + "x " + kind + " erzeugt.");
              },
              [](CommandSender&, const std::vector<std::string>& a) {
                  std::vector<std::string> out;
                  if (a.size() <= 1) {
                      out = mobTypes();
                      out.push_back("tnt");
                  }
                  return out;
              }});
}

}  // namespace vanilla
