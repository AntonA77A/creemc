#include "physics.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <set>
#include <tuple>

#include "buffer.hpp"
#include "data.hpp"
#include "entity.hpp"
#include "player.hpp"
#include "server.hpp"
#include "vanilla.hpp"
#include "version.hpp"

namespace physics {
namespace {

const int HORIZ[4][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}};
const int DIRS6[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}};

int WATER() { static int s = data::blockState("water"); return s; }
int LAVA() { static int s = data::blockState("lava"); return s; }

struct Fluid {
    bool water;   // sonst Lava
    int level;
};

std::optional<Fluid> fluid(int state) {
    static const auto w = data::BLOCKS.at("water"), l = data::BLOCKS.at("lava");
    if (state >= w.lo && state <= w.hi) return Fluid{true, state - w.lo};
    if (state >= l.lo && state <= l.hi) return Fluid{false, state - l.lo};
    return std::nullopt;
}
int amount(int level) { return level == 0 || level >= 8 ? 8 : 8 - level; }
int make(bool water, int level) { return (water ? data::BLOCKS.at("water").lo : data::BLOCKS.at("lava").lo) + level; }
int tickDelay(bool water) { return water ? 5 : 30; }
int dropOff(bool water) { return water ? 1 : 2; }
int slopeLimit(bool water) { return water ? 4 : 2; }

bool isGravity(int state) {
    static const std::set<std::string> G = {"minecraft:sand", "minecraft:red_sand", "minecraft:gravel", "minecraft:anvil",
                                            "minecraft:chipped_anvil", "minecraft:damaged_anvil", "minecraft:dragon_egg"};
    if (state == 0) return false;
    std::string n = data::blockName(state);
    return G.count(n) || endsWith(n, "_concrete_powder");
}

bool replaceable(int state) {
    // Darf ein fallender Block / eine FlÃ¼ssigkeit hier hinein (Luft, Pflanzen, Fackeln ...)?
    if (state == 0) return true;
    std::string n = data::blockName(state);
    if (n == "minecraft:cave_air" || n == "minecraft:void_air" || n == "minecraft:water" || n == "minecraft:lava" ||
        n == "minecraft:fire" || n == "minecraft:soul_fire")
        return true;
    if (data::SOLID[size_t(state)]) return false;
    for (const char* s : {"_sign", "ladder", "_door", "_portal", "end_gateway", "_banner", "scaffolding", "lever", "_button"})
        if (endsWith(n, s)) return false;
    return true;
}

class FallingBlock : public Entity {
public:
    FallingBlock(Server& s, double px, double py, double pz, int st) : Entity(s, "falling_block", px, py, pz), state(st) {
        gravity = 0.04;
        width = height = 0.98;
    }
    int state;
    std::vector<std::pair<std::string, Bytes>> spawnPackets(const Version& v) const override {
        std::vector<std::pair<std::string, Bytes>> out;
        auto t = v.entityType("falling_block");
        if (!t) return out;
        out.emplace_back("spawn_entity", v.spawnEntity(entityId, uuid, *t, x, y, z, 0, 0, vx, vy, vz, v.remap[size_t(state)]));
        return out;
    }
    void tick() override {
        Entity::tick();
        if (removed) return;
        int bx = int(std::floor(x)), by = int(std::floor(y)), bz = int(std::floor(z));
        int here = server.world().getBlock(bx, by, bz);
        std::string n = data::blockName(state);
        if (endsWith(n, "_concrete_powder") && here == WATER()) {   // Betonpulver im Wasser -> Beton
            remove();
            server.setBlock(bx, by, bz, n.substr(0, n.size() - 7));
            return;
        }
        if (onGround || age > 600) {
            remove();
            if (replaceable(here) && age <= 600) {
                server.setBlockState(bx, by, bz, state);
                if (n.find("anvil") != std::string::npos) server.playSound("minecraft:block.anvil.land", x, y, z, 0.3f, 1.0f, 4);
            } else {   // landet auf Fackel/Platte o.Ã¤. -> als Item
                if (server.gameRules().b("doTileDrops"))
                    for (auto& [item, c] : vanilla::blockDrops(n, "", true)) server.dropItem(x, y, z, item, c);
            }
        }
    }
};

struct State {
    std::set<std::tuple<int, int, int, int>> pending;   // (Art, x, y, z)
    std::set<BlockPos> growing;
    bool loaded = false;
};
State g;

void later(Server& s, int ticks, int kind, int x, int y, int z, std::function<void()> f) {
    auto key = std::make_tuple(kind, x, y, z);
    if (g.pending.count(key)) return;
    g.pending.insert(key);
    s.runLater(ticks, [key, f] {
        g.pending.erase(key);
        f();
    });
}

void track(Server& s, int x, int y, int z) {
    BlockPos p{x, y, z};
    if (g.growing.count(p)) return;
    g.growing.insert(p);
    json::Value& v = s.world().blockData[p];
    if (!v.isObject()) v = json::Object();
    v.asObject()["grow"] = true;
}

void untrack(Server& s, const BlockPos& p) {
    g.growing.erase(p);
    auto it = s.world().blockData.find(p);
    if (it == s.world().blockData.end() || !it->second.isObject()) return;
    json::Object& o = it->second.asObject();
    if (o.size() <= 1 && o.contains("grow")) s.world().blockData.erase(it);
    else if (o.contains("grow")) o["grow"] = false;
}

// ---- FlÃ¼ssigkeiten ----------------------------------------------------------

bool lavaMeetsWater(Server& s, int x, int y, int z, int state) {
    for (const auto& d : DIRS6) {
        if (d[1] == -1) continue;
        auto f = fluid(s.world().getBlock(x + d[0], y + d[1], z + d[2]));
        if (f && f->water) {
            int lvl = fluid(state)->level;
            s.setBlock(x, y, z, lvl == 0 ? "minecraft:obsidian" : "minecraft:cobblestone");
            s.playSound("minecraft:block.lava.extinguish", x + .5, y + .5, z + .5, 0.5f, 2.6f, 4);
            return true;
        }
    }
    return false;
}

std::optional<int> newLevel(Server& s, int x, int y, int z, bool water) {
    // Vanilla getNewLiquid: Pegel aus den Nachbarn (leer = trocknet aus)
    World& w = s.world();
    auto above = fluid(w.getBlock(x, y + 1, z));
    if (above && above->water == water) return 8;
    int best = 0, sources = 0;
    for (const auto& d : HORIZ) {
        auto f = fluid(w.getBlock(x + d[0], y, z + d[2]));
        if (f && f->water == water) {
            if (f->level == 0) ++sources;
            best = std::max(best, amount(f->level));
        }
    }
    if (sources >= 2 && s.gameRules().b(water ? "waterSourceConversion" : "lavaSourceConversion")) {   // unendliche Quelle
        int below = w.getBlock(x, y - 1, z);
        auto bf = fluid(below);
        if ((data::SOLID[size_t(below)] && !replaceable(below)) || (bf && bf->water && bf->level == 0)) return 0;
    }
    int amt = best - dropOff(water);
    if (amt <= 0) return std::nullopt;
    return 8 - amt;
}

bool canFlow(Server& s, int x, int y, int z, bool water) {
    if (y < MIN_Y || y >= MIN_Y + HEIGHT) return false;
    int st = s.world().getBlock(x, y, z);
    auto f = fluid(st);
    if (f) return f->level != 0 || f->water != water;   // Quellen derselben Art nicht Ã¼berschreiben
    return replaceable(st);
}

bool isSource(Server& s, int x, int y, int z, bool water) {
    auto f = fluid(s.world().getBlock(x, y, z));
    return f && f->water == water && f->level == 0;
}

void flowInto(Server& s, int x, int y, int z, bool water, int level, bool down = false) {
    int st = s.world().getBlock(x, y, z);
    auto f = fluid(st);
    if (f && f->water != water) {
        if (!water && f->water && down) {   // Lava fÃ¤llt in Wasser -> Stein
            s.setBlock(x, y, z, "minecraft:stone");
            s.playSound("minecraft:block.lava.extinguish", x + .5, y + .5, z + .5, 0.5f, 2.6f, 4);
        }
        return;
    }
    if (f && (f->level == 0 || amount(f->level) >= amount(level)) && !(level >= 8 && f->level < 8)) return;
    if (st != 0 && !f) {
        std::string n = data::blockName(st);
        if (n != "minecraft:cave_air" && n != "minecraft:void_air")   // Pflanzen, Fackeln ... werden weggespÃ¼lt
            if (s.gameRules().b("doTileDrops"))
                for (auto& [item, c] : vanilla::blockDrops(n, "", true)) s.dropItem(x + .5, y + .3, z + .5, item, c);
    }
    s.setBlockState(x, y, z, make(water, level));
}

int slope(Server& s, int x, int y, int z, int fromX, int fromZ, int depth, int limit, bool water) {
    int best = 1000;
    for (const auto& d : HORIZ) {
        if (d[0] == fromX && d[2] == fromZ) continue;
        int nx = x + d[0], nz = z + d[2];
        if (!canFlow(s, nx, y, nz, water) || isSource(s, nx, y, nz, water)) continue;
        if (canFlow(s, nx, y - 1, nz, water)) return depth;
        if (depth < limit) best = std::min(best, slope(s, nx, y, nz, -d[0], -d[2], depth + 1, limit, water));
    }
    return best;
}

std::vector<int> slopeDirs(Server& s, int x, int y, int z, bool water) {
    // Richtungen zum nÃ¤chsten Loch (Wasser bis 4, Lava bis 2 BlÃ¶cke); keins -> alle
    int best = 1000;
    std::vector<int> out;
    for (int i = 0; i < 4; ++i) {
        const auto& d = HORIZ[i];
        int nx = x + d[0], nz = z + d[2];
        if (!canFlow(s, nx, y, nz, water) || isSource(s, nx, y, nz, water)) continue;
        int dist = canFlow(s, nx, y - 1, nz, water) ? 0 : slope(s, nx, y, nz, -d[0], -d[2], 1, slopeLimit(water), water);
        if (dist < best) {
            best = dist;
            out = {i};
        } else if (dist == best) {
            out.push_back(i);
        }
    }
    return out;
}

void spread(Server& s, int x, int y, int z, bool water, int lvl) {
    if (canFlow(s, x, y - 1, z, water)) {
        flowInto(s, x, y - 1, z, water, 8, true);
        int sources = 0;
        for (const auto& d : HORIZ)
            if (isSource(s, x + d[0], y, z + d[2], water)) ++sources;
        if (sources < 3) return;   // groÃŸe Becken laufen auch zur Seite
    }
    int amt = lvl >= 8 ? 7 : amount(lvl) - dropOff(water);
    if (!water && lvl >= 8) amt = 8 - dropOff(water);
    if (amt <= 0) return;
    for (int i : slopeDirs(s, x, y, z, water)) {
        int nx = x + HORIZ[i][0], nz = z + HORIZ[i][2];
        if (canFlow(s, nx, y, nz, water)) flowInto(s, nx, y, nz, water, 8 - amt);
    }
}

void fluidTick(Server& s, int x, int y, int z) {
    int st = s.world().getBlock(x, y, z);
    auto f = fluid(st);
    if (!f) return;
    if (!f->water && lavaMeetsWater(s, x, y, z, st)) return;
    int lvl = f->level;
    if (lvl != 0) {
        auto nl = newLevel(s, x, y, z, f->water);
        if (!nl) {
            s.setBlockState(x, y, z, 0);
            return;
        }
        if (*nl != lvl) {
            s.setBlockState(x, y, z, make(f->water, *nl));
            lvl = *nl;
        }
    }
    spread(s, x, y, z, f->water, lvl);
}

// ---- Fallende BlÃ¶cke --------------------------------------------------------

void fall(Server& s, int x, int y, int z) {
    int st = s.world().getBlock(x, y, z);
    if (!isGravity(st) || y <= MIN_Y) return;
    if (!replaceable(s.world().getBlock(x, y - 1, z))) return;
    s.setBlockState(x, y, z, 0);
    s.spawnEntity(std::make_unique<FallingBlock>(s, x + 0.5, y, z + 0.5, st));
}

void onBlockChange(Server& s, int x, int y, int z) {
    World& w = s.world();
    for (int k = 0; k < 7; ++k) {
        int dx = k < 6 ? DIRS6[k][0] : 0, dy = k < 6 ? DIRS6[k][1] : 0, dz = k < 6 ? DIRS6[k][2] : 0;
        int px = x + dx, py = y + dy, pz = z + dz;
        int st = w.getBlock(px, py, pz);
        if (st == 0) continue;
        auto f = fluid(st);
        if (f) {
            if (!f->water && lavaMeetsWater(s, px, py, pz, st)) continue;
            later(s, tickDelay(f->water), 0, px, py, pz, [&s, px, py, pz] { fluidTick(s, px, py, pz); });
        } else if (isGravity(st)) {
            later(s, 2, 1, px, py, pz, [&s, px, py, pz] { fall(s, px, py, pz); });
        }
    }
    if (isGrowable(w.getBlock(x, y, z))) track(s, x, y, z);
}

// ---- Pflanzen ---------------------------------------------------------------

int maxAge(int state) {
    const auto* defs = data::propDefs(state);
    int top = 0;
    if (defs)
        for (const auto& d : *defs)
            if (d.name == "age")
                for (const auto& v : d.values) top = std::max(top, std::atoi(v.c_str()));
    return top;
}

bool growFruit(Server& s, int x, int y, int z, const std::string& stem) {
    std::string fruit = stem.find("pumpkin") != std::string::npos ? "pumpkin" : "melon";
    static const std::pair<const char*, std::pair<int, int>> DIRS[] = {
        {"east", {1, 0}}, {"west", {-1, 0}}, {"south", {0, 1}}, {"north", {0, -1}}};
    const auto& [facing, off] = DIRS[randInt(0, 3)];
    int tx = x + off.first, tz = z + off.second;
    std::string ground = data::blockName(s.world().getBlock(tx, y - 1, tz));
    static const std::set<std::string> OK = {"minecraft:dirt", "minecraft:grass_block", "minecraft:farmland",
                                             "minecraft:coarse_dirt", "minecraft:podzol"};
    if (s.world().getBlock(tx, y, tz) != 0 || !OK.count(ground)) return false;
    s.setBlock(tx, y, tz, "minecraft:" + fruit);
    int attached = data::blockState("attached_" + fruit + "_stem");
    if (attached >= 0) {
        s.setBlockState(x, y, z, data::withProps(attached, {{"facing", facing}}));
        untrack(s, {x, y, z});
    }
    return true;
}

bool growTree(Server& s, int x, int y, int z, const std::string& sapling) {
    std::string wood = removePrefix(sapling, "minecraft:");
    for (const char* suf : {"_sapling", "_propagule"})
        if (endsWith(wood, suf)) wood = wood.substr(0, wood.size() - std::string(suf).size());
    int log = data::blockState(wood + "_log"), leaves = data::blockState(wood + "_leaves");
    if (log < 0 || leaves < 0) {
        log = data::blockState("oak_log");
        leaves = data::blockState("oak_leaves");
    }
    int height = randInt(4, 6) + (wood == "spruce" || wood == "jungle" ? 2 : 0);
    for (int dy = 1; dy < height + 2; ++dy)   // Platz nach oben?
        if (!replaceable(s.world().getBlock(x, y + dy, z))) return false;
    int top = y + height - 1;
    for (int ly = top - 2; ly < top + 2; ++ly) {
        int r = ly < top ? 2 : 1;
        for (int lx = -r; lx <= r; ++lx)
            for (int lz = -r; lz <= r; ++lz) {
                if (std::abs(lx) == r && std::abs(lz) == r && (ly >= top || randDouble() < 0.5)) continue;
                if (s.world().getBlock(x + lx, ly, z + lz) == 0) s.setBlockState(x + lx, ly, z + lz, leaves);
            }
    }
    for (int dy = 0; dy < height; ++dy) s.setBlockState(x, y + dy, z, log);
    untrack(s, {x, y, z});
    return true;
}

void loadGrowing(Server& s) {
    if (g.loaded) return;
    g.loaded = true;
    for (auto& [pos, v] : s.world().blockData)
        if (v["grow"].asBool()) g.growing.insert(pos);
}

}  // namespace

bool isGrowable(int state) {
    if (state == 0) return false;
    static const std::set<std::string> G = {
        "minecraft:sugar_cane", "minecraft:cactus", "minecraft:mangrove_propagule", "minecraft:wheat", "minecraft:carrots",
        "minecraft:potatoes", "minecraft:beetroots", "minecraft:pumpkin_stem", "minecraft:melon_stem",
        "minecraft:sweet_berry_bush", "minecraft:nether_wart", "minecraft:cocoa", "minecraft:torchflower_crop",
        "minecraft:pitcher_crop"};
    std::string n = data::blockName(state);
    return endsWith(n, "_sapling") || G.count(n);
}

bool grow(Server& s, int x, int y, int z, bool bonemeal) {
    int st = s.world().getBlock(x, y, z);
    if (!isGrowable(st)) {
        untrack(s, {x, y, z});
        return false;
    }
    std::string n = data::blockName(st);
    auto p = data::props(st);
    if (endsWith(n, "_sapling") || n == "minecraft:mangrove_propagule") {
        if (bonemeal && randDouble() > 0.45) return true;
        if (p["stage"] == "0" && !bonemeal) {
            s.setBlockState(x, y, z, data::withProps(st, {{"stage", "1"}}));
            return true;
        }
        return growTree(s, x, y, z, n);
    }
    if (n == "minecraft:sugar_cane" || n == "minecraft:cactus") {
        if (bonemeal) return false;
        int age = std::atoi(p["age"].c_str());
        int height = 1;
        while (data::blockName(s.world().getBlock(x, y - height, z)) == n) ++height;
        if (height >= 3 || s.world().getBlock(x, y + 1, z) != 0) return false;
        if (age < 15) {
            s.setBlockState(x, y, z, data::withProps(st, {{"age", std::to_string(age + 1)}}));
        } else {
            s.setBlockState(x, y, z, data::withProps(st, {{"age", "0"}}));
            s.setBlock(x, y + 1, z, n);
            track(s, x, y + 1, z);
        }
        return true;
    }
    if (p.count("age")) {
        int age = std::atoi(p["age"].c_str()), top = maxAge(st);
        if (age >= top) {
            if ((n == "minecraft:pumpkin_stem" || n == "minecraft:melon_stem") && !bonemeal) return growFruit(s, x, y, z, n);
            return false;
        }
        int step = bonemeal ? randInt(2, 5) : 1;
        if (n == "minecraft:sweet_berry_bush" && bonemeal) step = 1;
        s.setBlockState(x, y, z, data::withProps(st, {{"age", std::to_string(std::min(top, age + step))}}));
        return true;
    }
    return false;
}

bool till(Server& s, int x, int y, int z) {
    std::string n = data::blockName(s.world().getBlock(x, y, z));
    if (s.world().getBlock(x, y + 1, z) != 0) return false;
    if (n == "minecraft:grass_block" || n == "minecraft:dirt" || n == "minecraft:dirt_path") s.setBlock(x, y, z, "minecraft:farmland");
    else if (n == "minecraft:coarse_dirt") s.setBlock(x, y, z, "minecraft:dirt");
    else return false;
    s.playSound("minecraft:item.hoe.till", x + .5, y + .5, z + .5, 1.0f, 1.0f, 7);
    return true;
}

bool canPlace(Server& s, const std::string& block, int x, int y, int z) {
    static const std::set<std::string> CROPS = {"minecraft:wheat", "minecraft:carrots", "minecraft:potatoes",
                                                "minecraft:beetroots", "minecraft:pumpkin_stem", "minecraft:melon_stem",
                                                "minecraft:torchflower_crop", "minecraft:pitcher_crop"};
    if (CROPS.count(block)) return data::blockName(s.world().getBlock(x, y - 1, z)) == "minecraft:farmland";
    return true;
}

bool useBucket(Player& p, const std::string& item) {
    Server& s = p.server;
    std::string base = data::baseItem(item);
    if (base != "minecraft:bucket" && base != "minecraft:water_bucket" && base != "minecraft:lava_bucket") return false;
    // Blickstrahl
    double ry = p.yaw * 3.14159265358979 / 180, rp = p.pitch * 3.14159265358979 / 180;
    double dx = -std::sin(ry) * std::cos(rp), dy = -std::sin(rp), dz = std::cos(ry) * std::cos(rp);
    double ex = p.x, ey = p.y + (p.sneaking ? 1.27 : 1.62), ez = p.z;
    bool stopAtFluid = base == "minecraft:bucket";
    std::optional<BlockPos> hit, prev;
    for (double t = 0; t <= 5.0; t += 0.05) {
        BlockPos pos{int(std::floor(ex + dx * t)), int(std::floor(ey + dy * t)), int(std::floor(ez + dz * t))};
        if (prev && *prev == pos) continue;
        int st = s.world().getBlock(pos.x, pos.y, pos.z);
        auto f = fluid(st);
        if ((f && stopAtFluid && f->level == 0) || (st != 0 && !f && !replaceable(st))) {
            hit = pos;
            break;
        }
        prev = pos;
    }
    auto swapItem = [&](const std::string& nw) {
        if (p.gamemode == 1) return;   // Kreativ: Eimer bleibt, wie in Vanilla
        int slot = 36 + p.heldSlot;
        auto it = p.inventory.find(slot);
        if (it == p.inventory.end()) return;
        if (it->second.second > 1) {
            p.setSlot(slot, it->second.first, it->second.second - 1);
            if (p.addItem(nw, 1) > 0) p.dropStack(nw, 1);
        } else {
            p.setSlot(slot, nw, 1);
        }
    };
    if (base == "minecraft:bucket") {
        if (!hit) return false;
        auto f = fluid(s.world().getBlock(hit->x, hit->y, hit->z));
        if (!f || f->level != 0) return false;
        s.setBlockState(hit->x, hit->y, hit->z, 0);
        swapItem(f->water ? "minecraft:water_bucket" : "minecraft:lava_bucket");
        s.playSound(f->water ? "minecraft:item.bucket.fill" : "minecraft:item.bucket.fill_lava", hit->x + .5, hit->y + .5, hit->z + .5, 1, 1, 7);
        return true;
    }
    std::optional<BlockPos> target;
    if (hit && replaceable(s.world().getBlock(hit->x, hit->y, hit->z))) target = hit;
    else if (prev && replaceable(s.world().getBlock(prev->x, prev->y, prev->z))) target = prev;
    if (!target) return false;
    bool water = base == "minecraft:water_bucket";
    s.setBlockState(target->x, target->y, target->z, make(water, 0));
    swapItem("minecraft:bucket");
    s.playSound(water ? "minecraft:item.bucket.empty" : "minecraft:item.bucket.empty_lava", target->x + .5, target->y + .5, target->z + .5, 1, 1, 7);
    return true;
}

void init(Server& server) {
    server.addBlockListener([&server](int x, int y, int z) { onBlockChange(server, x, y, z); });
}

void tick(Server& s) {
    loadGrowing(s);
    // Wie Vanillas zufÃ¤llige Ticks (3 pro 4096 BlÃ¶cke und Tick), nur fÃ¼r bekannte Pflanzen
    const double chance = s.gameRules().i("randomTickSpeed") / 4096.0;   // Vanilla: randomTickSpeed (3) pro 4096 Blöcke
    std::vector<BlockPos> list(g.growing.begin(), g.growing.end());
    for (const auto& p : list)
        if (randDouble() < chance) grow(s, p.x, p.y, p.z, false);
}

}  // namespace physics
