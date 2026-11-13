#include "redstone.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <tuple>

#include "blocks.hpp"
#include "data.hpp"
#include "server.hpp"
#include "vanilla.hpp"

namespace redstone {
namespace {

using Pos = BlockPos;
using Props = std::map<std::string, std::string>;

enum Dir { NORTH, SOUTH, WEST, EAST, UP, DOWN };
const int OFF[6][3] = {{0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, -1, 0}};
const char* DNAME[6] = {"north", "south", "west", "east", "up", "down"};
const int OPP[6] = {SOUTH, NORTH, EAST, WEST, DOWN, UP};
const int HORIZ[4] = {NORTH, SOUTH, WEST, EAST};
const int MAX_PUSH = 12;

int dirOf(const std::string& n) {
    for (int i = 0; i < 6; ++i)
        if (n == DNAME[i]) return i;
    return -1;
}
Pos add(const Pos& p, int d) { return {p.x + OFF[d][0], p.y + OFF[d][1], p.z + OFF[d][2]}; }

const std::string WIRE = "minecraft:redstone_wire";
const std::string HEAD = "minecraft:piston_head";
bool isTorch(const std::string& n) { return n == "minecraft:redstone_torch" || n == "minecraft:redstone_wall_torch"; }
bool isDiode(const std::string& n) { return n == "minecraft:repeater" || n == "minecraft:comparator"; }
bool isPiston(const std::string& n) { return n == "minecraft:piston" || n == "minecraft:sticky_piston"; }
bool endsAny(const std::string& n, std::initializer_list<const char*> l) {
    for (const char* s : l)
        if (endsWith(n, s)) return true;
    return false;
}

const std::set<std::string> NON_CONDUCTORS = {
    "minecraft:glass", "minecraft:tinted_glass", "minecraft:glowstone", "minecraft:sea_lantern", "minecraft:redstone_block",
    "minecraft:observer", "minecraft:piston", "minecraft:sticky_piston", "minecraft:piston_head", "minecraft:moving_piston",
    "minecraft:slime_block", "minecraft:honey_block", "minecraft:ice", "minecraft:packed_ice", "minecraft:blue_ice",
    "minecraft:beacon"};
const std::set<std::string> IMMOVABLE = {
    "minecraft:obsidian", "minecraft:crying_obsidian", "minecraft:bedrock", "minecraft:barrier", "minecraft:end_portal_frame",
    "minecraft:end_portal", "minecraft:end_gateway", "minecraft:nether_portal", "minecraft:reinforced_deepslate",
    "minecraft:respawn_anchor", "minecraft:enchanting_table", "minecraft:ender_chest", "minecraft:jukebox",
    "minecraft:lectern", "minecraft:spawner", "minecraft:beacon", "minecraft:command_block", "minecraft:chain_command_block",
    "minecraft:repeating_command_block", "minecraft:structure_block", "minecraft:jigsaw", "minecraft:light",
    "minecraft:piston_head", "minecraft:moving_piston", "minecraft:chest", "minecraft:trapped_chest", "minecraft:barrel",
    "minecraft:furnace", "minecraft:blast_furnace", "minecraft:smoker", "minecraft:hopper", "minecraft:dispenser",
    "minecraft:dropper", "minecraft:brewing_stand", "minecraft:daylight_detector", "minecraft:conduit", "minecraft:bell",
    "minecraft:grindstone"};
const std::map<std::string, int> CONTAINERS = {
    {"minecraft:chest", 27}, {"minecraft:trapped_chest", 27}, {"minecraft:barrel", 27}, {"minecraft:dispenser", 9},
    {"minecraft:dropper", 9}, {"minecraft:hopper", 5}, {"minecraft:furnace", 3}, {"minecraft:smoker", 3},
    {"minecraft:blast_furnace", 3}};

struct State {
    std::map<Pos, std::vector<int64_t>> torchFlips;   // Fackel -> letzte Umschalt-Ticks (Durchbrennen)
    std::set<std::tuple<int, int, int, int>> scheduled;
    std::set<Pos> fresh;                               // gerade neu berechnetes Staub-Netz
    std::map<Pos, int64_t> moving;                     // Kolben -> Tick, bis zu dem er sich bewegt
    std::map<Pos, int> cmp;                            // Komparator-Ausgang
};
State g;
Server* S = nullptr;

int get(const Pos& p) { return S->world().getBlock(p.x, p.y, p.z); }
void set(const Pos& p, int st, bool update = true) { S->setBlockState(p.x, p.y, p.z, st, update); }
std::string name(int st) { return data::blockName(st); }
Props props(int st) { return data::props(st); }
std::string prop(int st, const char* k) { return data::prop(st, k); }

void later(int ticks, int kind, const Pos& p, std::function<void()> f) {
    auto key = std::make_tuple(kind, p.x, p.y, p.z);
    if (g.scheduled.count(key)) return;
    g.scheduled.insert(key);
    S->runLater(std::max(1, ticks), [key, f] {
        g.scheduled.erase(key);
        f();
    });
}

int attachedDir(int st) {
    // In welche Richtung hängt ein Hebel/Knopf/eine Fackel an ihrem Block?
    std::string n = name(st);
    Props p = props(st);
    if (n == "minecraft:redstone_wall_torch") return OPP[dirOf(p["facing"])];
    if (n == "minecraft:redstone_torch") return DOWN;
    const std::string& face = p["face"];
    if (face == "floor") return DOWN;
    if (face == "ceiling") return UP;
    if (face == "wall") return OPP[dirOf(p["facing"])];
    return -1;
}

bool isFull(int st) {
    if (!data::SOLID[size_t(st)]) return false;
    std::string n = name(st);
    return !endsAny(n, {"_slab", "_stairs", "_fence", "_fence_gate", "_wall", "_pane", "_trapdoor", "_door", "_bed", "_sign",
                        "_carpet", "_button", "_pressure_plate", "_head", "_skull", "_candle", "candle", "_leaves", "glass",
                        "_banner", "_coral", "_coral_fan", "_bud", "_cluster", "_rod", "lantern", "chain", "iron_bars",
                        "repeater", "comparator", "chest", "hopper", "anvil", "cake", "daylight_detector", "enchanting_table",
                        "brewing_stand", "cauldron", "lectern", "stonecutter", "bell", "campfire", "flower_pot", "scaffolding",
                        "snow", "farmland", "dirt_path", "cactus", "bamboo", "dragon_egg", "turtle_egg", "sea_pickle", "cocoa",
                        "conduit", "grindstone", "composter", "pointed_dripstone", "big_dripleaf", "azalea", "ladder",
                        "end_portal_frame", "lily_pad", "honey_block", "slime_block", "piston_head", "moving_piston",
                        "decorated_pot", "heavy_core", "shulker_box", "bed", "beacon", "vault", "trial_spawner"});
}
bool isConductor(int st) { return isFull(st) && !NON_CONDUCTORS.count(name(st)); }
bool isEmitter(int st) {
    std::string n = name(st);
    return n == WIRE || isTorch(n) || isDiode(n) || n == "minecraft:redstone_block" || n == "minecraft:lever" ||
           n == "minecraft:observer" || endsAny(n, {"_button", "_pressure_plate"});
}

int weak(int st, int toward) {
    if (st == 0) return 0;
    std::string n = name(st);
    Props p = props(st);
    if (n == "minecraft:redstone_block") return 15;
    if (n == "minecraft:lever" || endsWith(n, "_button")) return p["powered"] == "true" ? 15 : 0;
    if (endsWith(n, "_weighted_pressure_plate")) return std::atoi(p["power"].c_str());
    if (endsWith(n, "_pressure_plate")) return p["powered"] == "true" ? 15 : 0;
    if (isTorch(n)) return p["lit"] == "true" && toward != attachedDir(st) ? 15 : 0;
    if (n == WIRE) {
        int power = std::atoi(p["power"].c_str());
        if (!power || toward == UP) return 0;
        if (toward == DOWN) return power;
        bool any = false, toSide = false;
        for (int d : HORIZ)
            if (!p[DNAME[d]].empty() && p[DNAME[d]] != "none") {
                any = true;
                if (d == toward) toSide = true;
            }
        return !any || toSide ? power : 0;
    }
    if (n == "minecraft:repeater" || n == "minecraft:observer")
        return p["powered"] == "true" && toward == OPP[dirOf(p["facing"])] ? 15 : 0;
    return 0;   // Komparator: Stärke in g.cmp
}

int strong(int st, int toward) {
    std::string n = name(st);
    Props p = props(st);
    if (n == "minecraft:lever" || endsWith(n, "_button")) return p["powered"] == "true" && toward == attachedDir(st) ? 15 : 0;
    if (endsWith(n, "_pressure_plate")) return toward == DOWN ? weak(st, toward) : 0;
    if (isTorch(n)) return p["lit"] == "true" && toward == UP ? 15 : 0;
    if (n == WIRE || n == "minecraft:repeater" || n == "minecraft:observer") return weak(st, toward);
    return 0;
}

int comparatorOut(const Pos& p) {
    auto it = g.cmp.find(p);
    return it == g.cmp.end() ? 0 : it->second;
}

int weakAt(const Pos& p, int toward) {
    int st = get(p);
    if (name(st) == "minecraft:comparator") return toward == OPP[dirOf(prop(st, "facing"))] ? comparatorOut(p) : 0;
    return weak(st, toward);
}
int strongAt(const Pos& p, int toward) {
    int st = get(p);
    if (name(st) == "minecraft:comparator") return weakAt(p, toward);
    return strong(st, toward);
}

int directInto(const Pos& p, bool forWire = false) {
    int best = 0;
    for (int d = 0; d < 6 && best < 15; ++d) {
        Pos m = add(p, d);
        if (forWire && name(get(m)) == WIRE) continue;
        best = std::max(best, strongAt(m, OPP[d]));
    }
    return best;
}

int signalFrom(const Pos& np, int toward, bool forWire = false) {
    int st = get(np);
    if (st == 0) return 0;
    if (forWire && name(st) == WIRE) return 0;
    if (isConductor(st)) return directInto(np, forWire);
    return weakAt(np, toward);
}

bool hasSignal(const Pos& p, int skip = -1) {
    for (int d = 0; d < 6; ++d) {
        if (d == skip) continue;
        if (signalFrom(add(p, d), OPP[d]) > 0) return true;
    }
    return false;
}
bool qcPowered(const Pos& p, int skip = -1) {
    // Kolben/Spender: Strom von den Seiten ODER am Block darüber (Quasi-Connectivity)
    return hasSignal(p, skip) || hasSignal(add(p, UP), DOWN);
}

// ---- Staub ------------------------------------------------------------------

bool connects(int d, int other) {
    std::string n = name(other);
    if (n == WIRE || isTorch(n) || n == "minecraft:redstone_block" || n == "minecraft:lever" ||
        endsAny(n, {"_button", "_pressure_plate"}) || n == "minecraft:trapped_chest" || n == "minecraft:target")
        return true;
    if (n == "minecraft:repeater") {
        int f = dirOf(prop(other, "facing"));
        return f == d || f == OPP[d];
    }
    if (n == "minecraft:comparator") return true;
    if (n == "minecraft:observer") return dirOf(prop(other, "facing")) == d;
    return false;
}

std::vector<Pos> wireNeighbors(const Pos& p) {
    std::vector<Pos> out;
    bool aboveFree = !isConductor(get({p.x, p.y + 1, p.z}));
    for (int d : HORIZ) {
        Pos n = add(p, d);
        int side = get(n);
        if (name(side) == WIRE) out.push_back(n);
        else if (isConductor(side)) {
            Pos up{n.x, p.y + 1, n.z};
            if (aboveFree && name(get(up)) == WIRE) out.push_back(up);
        } else {
            Pos down{n.x, p.y - 1, n.z};
            if (name(get(down)) == WIRE) out.push_back(down);
        }
    }
    return out;
}

std::map<std::string, std::string> shape(const Pos& p) {
    std::map<std::string, std::string> conn;
    bool aboveFree = !isConductor(get({p.x, p.y + 1, p.z}));
    std::vector<int> sides;
    for (int d : HORIZ) {
        Pos n = add(p, d);
        int side = get(n);
        std::string v = "none";
        if (connects(d, side)) v = "side";
        else if (isConductor(side) && aboveFree && name(get({n.x, p.y + 1, n.z})) == WIRE) v = "up";
        else if (!isConductor(side) && name(get({n.x, p.y - 1, n.z})) == WIRE) v = "side";
        conn[DNAME[d]] = v;
        if (v != "none") sides.push_back(d);
    }
    if (sides.size() == 1) conn[DNAME[OPP[sides[0]]]] = "side";   // eine Verbindung -> gerade Linie
    else if (sides.empty())
        for (int d : HORIZ) conn[DNAME[d]] = "side";   // allein -> Kreuz (Vanilla-Standard)
    return conn;
}

void updateWire(const Pos& start) {
    std::set<Pos> network;
    std::vector<Pos> stack{start};
    while (!stack.empty() && network.size() < 4096) {
        Pos p = stack.back();
        stack.pop_back();
        if (!network.insert(p).second) continue;
        for (const auto& n : wireNeighbors(p))
            if (!network.count(n)) stack.push_back(n);
    }
    std::map<Pos, int> power;
    for (const auto& p : network) {
        int best = 0;
        for (int d = 0; d < 6; ++d) best = std::max(best, signalFrom(add(p, d), OPP[d], true));
        power[p] = best;
    }
    std::vector<Pos> queue(network.begin(), network.end());
    std::sort(queue.begin(), queue.end(), [&](const Pos& a, const Pos& b) { return power[a] > power[b]; });
    for (size_t i = 0; i < queue.size(); ++i) {
        Pos p = queue[i];
        for (const auto& n : wireNeighbors(p))
            if (network.count(n) && power[p] - 1 > power[n]) {
                power[n] = power[p] - 1;
                queue.push_back(n);
            }
    }
    g.fresh = network;
    std::vector<Pos> order(network.begin(), network.end());
    std::sort(order.begin(), order.end(), [&](const Pos& a, const Pos& b) { return power[a] > power[b]; });
    for (const auto& p : order) {
        int st = get(p);
        if (name(st) != WIRE) continue;
        auto changes = shape(p);
        changes["power"] = std::to_string(power[p]);
        int nw = data::withProps(st, changes);
        if (nw != st) set(p, nw);
    }
}

// ---- Bauteile ---------------------------------------------------------------

void updateComponent(const Pos& p, bool skipWire = false);

void playNote(const Pos& p, int st) {
    Props pr = props(st);
    int note = std::atoi(pr["note"].c_str());
    std::string inst = pr["instrument"].empty() ? "harp" : pr["instrument"];
    if (endsWith(inst, "_head")) return;
    S->playSound("minecraft:block.note_block." + inst, p.x + .5, p.y + .5, p.z + .5, 3.0f, float(std::pow(2.0, (note - 12) / 12.0)), 2);
}

void lampOff(const Pos& p) {
    int st = get(p);
    if (name(st) == "minecraft:redstone_lamp" && !hasSignal(p)) set(p, data::withProps(st, {{"lit", "false"}}));
}

void doorUpdate(const Pos& p, int st) {
    std::string n = name(st);
    Props pr = props(st);
    bool on = hasSignal(p);
    std::optional<Pos> other;
    if (endsWith(n, "_door")) {
        Pos o{p.x, pr["half"] == "lower" ? p.y + 1 : p.y - 1, p.z};
        if (name(get(o)) == n) {
            other = o;
            on = on || hasSignal(o);
        }
    }
    std::string val = on ? "true" : "false";
    if (pr["powered"] == val) return;
    set(p, data::withProps(st, {{"powered", val}, {"open", val}}), false);
    if (other) set(*other, data::withProps(get(*other), {{"powered", val}, {"open", val}}), false);
    std::string kind = n == "minecraft:iron_door" ? "iron_door" : n == "minecraft:iron_trapdoor" ? "iron_trapdoor"
                       : endsWith(n, "fence_gate") ? "fence_gate" : endsWith(n, "trapdoor") ? "wooden_trapdoor" : "wooden_door";
    S->playSound("minecraft:block." + kind + (on ? ".open" : ".close"), p.x + .5, p.y + .5, p.z + .5, 1, 1, 4);
}

void torchTick(const Pos& p) {
    int st = get(p);
    if (!isTorch(name(st))) return;
    int a = attachedDir(st);
    std::string lit = signalFrom(add(p, a), OPP[a]) > 0 ? "false" : "true";
    if (prop(st, "lit") == lit) return;
    int64_t now = S->tick();
    auto& flips = g.torchFlips[p];
    flips.erase(std::remove_if(flips.begin(), flips.end(), [now](int64_t t) { return now - t >= 60; }), flips.end());
    if (lit == "true" && flips.size() >= 8) {   // zu schnell umgeschaltet -> durchgebrannt
        S->playSound("minecraft:block.redstone_torch.burnout", p.x + .5, p.y + .5, p.z + .5, 0.5f, 2.6f, 4);
        S->runLater(160, [p] { updateComponent(p); });
        return;
    }
    flips.push_back(now);
    set(p, data::withProps(st, {{"lit", lit}}));
}

int diodeInput(const Pos& p, int st) {
    int f = dirOf(prop(st, "facing"));
    Pos rear = add(p, f);
    int sig = signalFrom(rear, OPP[f]);
    int rs = get(rear);
    if (sig < 15 && name(rs) == WIRE) sig = std::max(sig, std::atoi(prop(rs, "power").c_str()));
    return sig;
}

int sideInput(const Pos& p, int st, bool diodesOnly) {
    int f = dirOf(prop(st, "facing"));
    int best = 0;
    for (int d : HORIZ) {
        if (d == f || d == OPP[f]) continue;
        Pos n = add(p, d);
        int ns = get(n);
        std::string nn = name(ns);
        if (isDiode(nn)) best = std::max(best, weakAt(n, OPP[d]));
        else if (!diodesOnly) {
            if (nn == WIRE) best = std::max(best, std::atoi(prop(ns, "power").c_str()));
            else if (nn == "minecraft:redstone_block") best = 15;
        }
    }
    return best;
}

void repeaterTick(const Pos& p);
void repeaterUpdate(const Pos& p, int st) {
    std::string locked = sideInput(p, st, true) > 0 ? "true" : "false";
    if (prop(st, "locked") != locked) {
        st = data::withProps(st, {{"locked", locked}});
        set(p, st, false);
    }
    if (locked == "true") return;
    bool on = diodeInput(p, st) > 0;
    if (on != (prop(st, "powered") == "true"))
        later(std::atoi(prop(st, "delay").c_str()) * 2, 0, p, [p] { repeaterTick(p); });
}
void repeaterTick(const Pos& p) {
    int st = get(p);
    if (name(st) != "minecraft:repeater" || prop(st, "locked") == "true") return;
    bool powered = prop(st, "powered") == "true";
    bool on = diodeInput(p, st) > 0;
    if (powered && !on) {
        set(p, data::withProps(st, {{"powered", "false"}}));
    } else if (!powered) {
        set(p, data::withProps(st, {{"powered", "true"}}));
        if (!on) repeaterUpdate(p, get(p));   // Puls kürzer als die Verzögerung: trotzdem voll weitergeben
    }
}

std::optional<int> fillLevel(const Pos& p) {
    std::string n = name(get(p));
    auto c = CONTAINERS.find(n);
    if (c == CONTAINERS.end() && !endsWith(n, "shulker_box")) return std::nullopt;
    int size = c != CONTAINERS.end() ? c->second : 27;
    auto st = blocks::blockStorage(*S, p, size);
    double full = 0;
    for (int i = 0; i < st->size(); ++i) {
        Stack s = st->get(i);
        if (!s.first.empty()) full += double(s.second) / data::stackSize(s.first);
    }
    if (full <= 0) return 0;
    return int(1 + full / st->size() * 14);
}

int comparatorCalc(const Pos& p, int st) {
    int f = dirOf(prop(st, "facing"));
    Pos rear = add(p, f);
    int sig = diodeInput(p, st);
    auto lvl = fillLevel(rear);
    if (lvl) sig = *lvl;
    else if (sig < 15 && isConductor(get(rear))) {
        lvl = fillLevel(add(rear, f));
        if (lvl) sig = *lvl;
    }
    int side = sideInput(p, st, false);
    if (prop(st, "mode") == "subtract") return std::max(sig - side, 0);
    return sig >= side ? sig : 0;
}

void comparatorTick(const Pos& p) {
    int st = get(p);
    if (name(st) != "minecraft:comparator") return;
    int out = comparatorCalc(p, st);
    int old = comparatorOut(p);
    g.cmp[p] = out;
    std::string powered = out > 0 ? "true" : "false";
    if (prop(st, "powered") != powered) {
        set(p, data::withProps(st, {{"powered", powered}}));
    } else if (out != old) {   // Stärke geändert, Zustand gleich: Ausgang trotzdem aktualisieren
        Pos front = add(p, OPP[dirOf(prop(st, "facing"))]);
        updateComponent(front);
        for (int d = 0; d < 6; ++d) updateComponent(add(front, d));
    }
}

void observerTick(const Pos& p) {
    int st = get(p);
    if (name(st) != "minecraft:observer") return;
    if (prop(st, "powered") == "true") {
        set(p, data::withProps(st, {{"powered", "false"}}));
    } else {
        set(p, data::withProps(st, {{"powered", "true"}}));
        later(2, 3, p, [p] { observerTick(p); });
    }
}
void observe(const Pos& p) {
    int st = get(p);
    if (name(st) == "minecraft:observer" && prop(st, "powered") != "true") later(2, 3, p, [p] { observerTick(p); });
}

// Kolben
enum Reaction { EMPTY, MOVE, DESTROY, BLOCK };

Reaction pushReaction(const Pos& p) {
    int st = get(p);
    if (st == 0) return EMPTY;
    std::string n = name(st);
    if (n == "minecraft:water" || n == "minecraft:lava") return DESTROY;
    if (n == "minecraft:cave_air" || n == "minecraft:void_air") return EMPTY;
    if (endsWith(n, "shulker_box")) return DESTROY;
    auto bd = S->world().blockData.find(p);
    bool hasItems = bd != S->world().blockData.end() && bd->second["items"].isArray();
    if (IMMOVABLE.count(n) || hasItems) return BLOCK;
    if (isPiston(n)) return prop(st, "extended") == "true" ? BLOCK : MOVE;
    if (p.y < MIN_Y || p.y >= MIN_Y + HEIGHT) return BLOCK;
    if (isFull(st) ||
        endsAny(n, {"_slab", "_stairs", "_fence", "_wall", "_pane", "iron_bars", "_trapdoor", "anvil", "chain", "_fence_gate",
                    "lantern", "hopper", "_glass", "glass", "end_rod", "lightning_rod"}) ||
        n == "minecraft:observer" || n == "minecraft:redstone_block" || n == "minecraft:slime_block" || n == "minecraft:honey_block")
        return MOVE;
    return DESTROY;
}

void destroy(const Pos& p) {
    int st = get(p);
    std::string n = name(st);
    blocks::dropContents(*S, p.x, p.y, p.z);
    set(p, 0);
    if (n == "minecraft:water" || n == "minecraft:lava") return;
    for (auto& [item, c] : vanilla::blockDrops(n, "", true)) S->dropItem(p.x + .5, p.y + .3, p.z + .5, item, c);
}

void pistonUpdate(const Pos& p, int st);
void pistonRecheck(const Pos& p) {
    int st = get(p);
    if (isPiston(name(st))) pistonUpdate(p, st);
}

bool extend(const Pos& p, int st) {
    int f = dirOf(prop(st, "facing"));
    std::vector<Pos> line;
    Pos cur = add(p, f);
    while (true) {
        Reaction r = pushReaction(cur);
        if (r == EMPTY) break;
        if (r == BLOCK) return false;
        if (r == DESTROY) {
            destroy(cur);
            break;
        }
        line.push_back(cur);
        if (int(line.size()) > MAX_PUSH) return false;
        cur = add(cur, f);
    }
    std::vector<int> states;
    for (const auto& b : line) states.push_back(get(b));
    for (int i = int(line.size()) - 1; i >= 0; --i) set(add(line[size_t(i)], f), states[size_t(i)]);
    int head = data::withProps(data::blockState("piston_head"),
                               {{"facing", DNAME[f]}, {"type", name(st) == "minecraft:sticky_piston" ? "sticky" : "normal"}, {"short", "false"}});
    set(p, data::withProps(st, {{"extended", "true"}}), false);
    set(add(p, f), head);
    g.moving[p] = S->tick() + 2;
    S->playSound("minecraft:block.piston.extend", p.x + .5, p.y + .5, p.z + .5, 0.5f, float(randDouble(0.6, 0.85)), 4);
    S->runLater(2, [p] { pistonRecheck(p); });
    return true;
}

void retract(const Pos& p, int st) {
    int f = dirOf(prop(st, "facing"));
    Pos front = add(p, f);
    set(p, data::withProps(st, {{"extended", "false"}}), false);
    if (name(get(front)) == HEAD) set(front, 0);
    if (name(st) == "minecraft:sticky_piston") {
        Pos pulled = add(front, f);
        int fs = get(pulled);
        if (pushReaction(pulled) == MOVE && !endsWith(name(fs), "_glazed_terracotta")) {
            set(pulled, 0);
            set(front, fs);
        }
    }
    g.moving[p] = S->tick() + 2;
    S->playSound("minecraft:block.piston.contract", p.x + .5, p.y + .5, p.z + .5, 0.5f, float(randDouble(0.6, 0.75)), 4);
    S->runLater(2, [p] { pistonRecheck(p); });
}

void pistonUpdate(const Pos& p, int st) {
    auto mv = g.moving.find(p);
    if (mv != g.moving.end() && mv->second > S->tick()) {
        S->runLater(int(mv->second - S->tick()), [p] { pistonRecheck(p); });
        return;
    }
    bool powered = qcPowered(p, dirOf(prop(st, "facing")));
    bool extended = prop(st, "extended") == "true";
    if (powered && !extended) extend(p, st);
    else if (!powered && extended) retract(p, st);
}

void updateComponent(const Pos& p, bool skipWire) {
    int st = get(p);
    if (st == 0) return;
    std::string n = name(st);
    if (n == WIRE) {
        if (!skipWire) updateWire(p);
    } else if (isTorch(n)) {
        later(2, 1, p, [p] { torchTick(p); });
    } else if (n == "minecraft:repeater") {
        repeaterUpdate(p, st);
    } else if (n == "minecraft:comparator") {
        later(2, 2, p, [p] { comparatorTick(p); });
    } else if (isPiston(n)) {
        pistonUpdate(p, st);
    } else if (n == "minecraft:redstone_lamp") {
        bool on = hasSignal(p);
        if (on && prop(st, "lit") != "true") set(p, data::withProps(st, {{"lit", "true"}}));
        else if (!on && prop(st, "lit") == "true") later(4, 4, p, [p] { lampOff(p); });
    } else if (endsAny(n, {"_door", "_trapdoor", "_fence_gate"}) && data::hasProp(st, "powered")) {
        doorUpdate(p, st);
    } else if (n == "minecraft:tnt") {
        if (hasSignal(p)) vanilla::primeTnt(*S, p.x, p.y, p.z);
    } else if (n == "minecraft:dispenser" || n == "minecraft:dropper") {
        bool on = qcPowered(p);
        std::string val = on ? "true" : "false";
        if (prop(st, "triggered") != val) {
            set(p, data::withProps(st, {{"triggered", val}}), false);
            if (on) S->runLater(4, [p] { blocks::dispense(*S, p.x, p.y, p.z); });
        }
    } else if (n == "minecraft:hopper") {
        std::string val = hasSignal(p) ? "false" : "true";
        if (prop(st, "enabled") != val) set(p, data::withProps(st, {{"enabled", val}}), false);
    } else if (n == "minecraft:note_block") {
        bool on = hasSignal(p);
        if (on != (prop(st, "powered") == "true")) {
            set(p, data::withProps(st, {{"powered", on ? "true" : "false"}}), false);
            if (on) playNote(p, st);
        }
    }
}

void onBlockChange(int x, int y, int z) {
    Pos pos{x, y, z};
    int nw = get(pos);
    auto prev = S->prevStates.find(std::make_tuple(x, y, z));
    int old = prev == S->prevStates.end() ? 0 : prev->second;
    bool ownWire = g.fresh.count(pos) && name(nw) == WIRE;

    // Beobachter, die auf diesen Block schauen
    for (int d = 0; d < 6; ++d) {
        Pos o = add(pos, d);
        int os = get(o);
        if (name(os) == "minecraft:observer" && dirOf(prop(os, "facing")) == OPP[d]) observe(o);
    }
    // Kolbenkopf/Kolben entfernt -> anderes Teil auch weg
    if (isPiston(name(old)) && prop(old, "extended") == "true" && !isPiston(name(nw))) {
        Pos h = add(pos, dirOf(prop(old, "facing")));
        if (name(get(h)) == HEAD) set(h, 0);
    } else if (name(old) == HEAD && nw == 0) {
        Pos base = add(pos, OPP[dirOf(prop(old, "facing"))]);
        int bs = get(base);
        auto mv = g.moving.find(base);
        if (isPiston(name(bs)) && prop(bs, "extended") == "true" && (mv == g.moving.end() || mv->second <= S->tick())) destroy(base);
    }
    // Wer bekommt ein Update? (Vanilla: je nach Bauteil auch die Nachbarn der Nachbarn)
    std::set<Pos> targets;
    for (int d = 0; d < 6; ++d) targets.insert(add(pos, d));
    for (int st : {old, nw}) {
        std::string n = name(st);
        if (n == WIRE || isTorch(n)) {
            std::vector<Pos> base(targets.begin(), targets.end());
            for (const auto& t : base)
                for (int d = 0; d < 6; ++d) targets.insert(add(t, d));
        } else if (n == "minecraft:lever" || endsAny(n, {"_button", "_pressure_plate"})) {
            int a = attachedDir(st);
            Pos ap = add(pos, a < 0 ? DOWN : a);
            for (int d = 0; d < 6; ++d) targets.insert(add(ap, d));
        } else if (isDiode(n) || n == "minecraft:observer") {
            int f = dirOf(prop(st, "facing"));
            if (f < 0) continue;
            Pos out = add(pos, OPP[f]);
            targets.insert(out);
            for (int d = 0; d < 6; ++d) targets.insert(add(out, d));
        }
    }
    targets.erase(pos);
    if (isEmitter(nw) || isPiston(name(nw)) || name(nw) == "minecraft:redstone_lamp") targets.insert(pos);
    for (const auto& t : targets) updateComponent(t, ownWire && g.fresh.count(t));
}

}  // namespace

void init(Server& server) {
    S = &server;
    server.addBlockListener([](int x, int y, int z) { onBlockChange(x, y, z); });
}

void tick(Server& server) {
    // Behälter ändern sich ohne Block-Update: Komparatoren regelmäßig nachsehen
    if (server.tick() % 2 != 0) return;
    for (auto it = g.cmp.begin(); it != g.cmp.end();) {
        Pos p = it->first;
        int st = get(p);
        if (name(st) != "minecraft:comparator") {
            it = g.cmp.erase(it);
            continue;
        }
        if (comparatorCalc(p, st) != it->second) later(2, 2, p, [p] { comparatorTick(p); });
        ++it;
    }
}

}  // namespace redstone
