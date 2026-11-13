#include "placement.hpp"

#include <cmath>
#include <map>

#include "data.hpp"
#include "server.hpp"
#include "util.hpp"
#include "world.hpp"

namespace placement {

const int FACE_OFFSETS[6][3] = {{0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}};

namespace {
const char* HORIZONTAL[4] = {"south", "west", "north", "east"};   // yaw 0 = Süden
const std::map<std::string, std::string> OPP = {{"north", "south"}, {"south", "north"}, {"east", "west"},
                                                {"west", "east"}, {"up", "down"}, {"down", "up"}};
const std::map<std::string, std::tuple<int, int, int>> OFFSET = {
    {"north", {0, 0, -1}}, {"south", {0, 0, 1}}, {"west", {-1, 0, 0}}, {"east", {1, 0, 0}}, {"up", {0, 1, 0}}, {"down", {0, -1, 0}}};
const char* FACE_DIR[6] = {"down", "up", "north", "south", "west", "east"};
const std::map<std::string, std::string> CLOCKWISE = {{"north", "east"}, {"east", "south"}, {"south", "west"}, {"west", "north"}};
const std::map<std::string, std::string> COUNTER = {{"east", "north"}, {"south", "east"}, {"west", "south"}, {"north", "west"}};
const std::map<std::string, std::string> ITEM_BLOCKS = {
    {"minecraft:redstone", "minecraft:redstone_wire"}, {"minecraft:string", "minecraft:tripwire"},
    {"minecraft:wheat_seeds", "minecraft:wheat"}, {"minecraft:carrot", "minecraft:carrots"},
    {"minecraft:potato", "minecraft:potatoes"}, {"minecraft:beetroot_seeds", "minecraft:beetroots"},
    {"minecraft:melon_seeds", "minecraft:melon_stem"}, {"minecraft:pumpkin_seeds", "minecraft:pumpkin_stem"},
    {"minecraft:sweet_berries", "minecraft:sweet_berry_bush"}, {"minecraft:glow_berries", "minecraft:cave_vines"},
    {"minecraft:cocoa_beans", "minecraft:cocoa"}};

const std::vector<std::string>& values(int state, const std::string& prop) {
    static const std::vector<std::string> none;
    const auto* defs = data::propDefs(state);
    if (defs)
        for (const auto& d : *defs)
            if (d.name == prop) return d.values;
    return none;
}

bool has(const std::vector<std::string>& v, const std::string& s) {
    for (const auto& x : v)
        if (x == s) return true;
    return false;
}

std::string wallVariant(const std::string& name) {
    std::string s = removePrefix(name, "minecraft:");
    std::string cand;
    if (s == "torch" || s == "soul_torch" || s == "redstone_torch" || s == "copper_torch") {
        cand = s.substr(0, s.size() - 5) + "wall_torch";
    } else {
        size_t u = s.rfind('_');
        if (u == std::string::npos) return "";
        cand = s.substr(0, u) + "_wall" + s.substr(u);
    }
    return data::BLOCKS.count(cand) ? "minecraft:" + cand : "";
}

bool isStairs(int s) { return endsWith(data::blockName(s), "_stairs"); }
bool isChest(int s) {
    auto n = data::blockName(s);
    return n == "minecraft:chest" || n == "minecraft:trapped_chest";
}
}  // namespace

std::string blockForItem(const std::string& item) {
    std::string base = data::baseItem(item);
    auto it = ITEM_BLOCKS.find(base);
    std::string name = it != ITEM_BLOCKS.end() ? it->second : base;
    return data::BLOCKS.count(removePrefix(name, "minecraft:")) ? name : "";
}

std::string lookDirection(float yaw) {
    double y = std::fmod(double(yaw), 360.0);
    if (y < 0) y += 360.0;
    return HORIZONTAL[int((y + 45.0) / 90.0) % 4];
}

std::vector<Part> place(std::string name, int x, int y, int z, int face, float yaw, float pitch, float cursorY) {
    if (face >= 2 && face <= 5) {
        std::string wall = wallVariant(name);
        if (!wall.empty()) name = wall;
    }
    int state = data::blockState(name);
    auto p = data::props(state);
    std::map<std::string, std::string> ch;
    std::string s = removePrefix(name, "minecraft:");
    std::string look = lookDirection(yaw);

    if (p.count("face") && (p["face"] == "floor" || p["face"] == "wall" || p["face"] == "ceiling"))
        ch["face"] = face == 1 ? "floor" : face == 0 ? "ceiling" : "wall";
    if (p.count("facing")) {
        const auto& vals = values(state, "facing");
        bool wallThing = endsWith(s, "wall_torch") || endsWith(s, "_wall_sign") || endsWith(s, "_wall_banner") ||
                         endsWith(s, "_wall_skull") || endsWith(s, "_wall_head") || endsWith(s, "_wall_hanging_sign");
        if (wallThing && face >= 2 && face <= 5) {
            ch["facing"] = FACE_DIR[face];
        } else if (ch.count("face") && ch["face"] == "wall") {
            ch["facing"] = FACE_DIR[face];
        } else if (ch.count("face")) {
            ch["facing"] = look;
        } else if (s == "hopper") {   // zeigt in den angeklickten Block
            ch["facing"] = (face == 0 || face == 1) ? "down" : OPP.at(FACE_DIR[face]);
        } else if (has(vals, "up") && has(vals, "down") && vals.size() == 6) {
            if (pitch > 45) ch["facing"] = "up";
            else if (pitch < -45) ch["facing"] = "down";
            else ch["facing"] = OPP.at(look);
            if (s == "observer") ch["facing"] = OPP.at(ch["facing"]);   // Gesicht in Blickrichtung
        } else if (endsWith(s, "_stairs") || endsWith(s, "_door") || endsWith(s, "_fence_gate") || endsWith(s, "_bed") ||
                   endsWith(s, "lectern")) {
            ch["facing"] = look;
        } else {
            ch["facing"] = has(vals, OPP.at(look)) ? OPP.at(look) : p["facing"];
        }
    }
    if (p.count("rotation")) {   // stehende Schilder, Banner, Köpfe: 16 Richtungen
        double r = std::fmod(double(yaw) + 180.0, 360.0);
        if (r < 0) r += 360.0;
        ch["rotation"] = std::to_string(int(r * 16 / 360 + 0.5) % 16);
    }
    if (p.count("axis")) ch["axis"] = (face == 0 || face == 1) ? "y" : (face == 2 || face == 3) ? "z" : "x";
    bool upper = face == 0 || (face >= 2 && face <= 5 && cursorY > 0.5f);
    if (p.count("half") && (p["half"] == "top" || p["half"] == "bottom")) ch["half"] = upper ? "top" : "bottom";
    if (p.count("type") && (p["type"] == "top" || p["type"] == "bottom") && has(values(state, "type"), "double"))
        ch["type"] = upper ? "top" : "bottom";

    state = data::withProps(state, ch);
    std::vector<Part> parts{{x, y, z, state}};
    p = data::props(state);
    if (p.count("half") && p["half"] == "lower") {   // Türen, hohe Pflanzen
        parts.push_back({x, y + 1, z, data::withProps(state, {{"half", "upper"}})});
    } else if (endsWith(s, "_bed") && p.count("part")) {   // Bett: Kopfteil in Blickrichtung
        auto [dx, dy, dz] = OFFSET.at(p["facing"]);
        parts = {{x, y, z, data::withProps(state, {{"part", "foot"}})},
                 {x + dx, y, z + dz, data::withProps(state, {{"part", "head"}})}};
    }
    return parts;
}

std::optional<std::tuple<int, int, int>> chestPartner(int x, int y, int z, int state) {
    auto p = data::props(state);
    if (!p.count("type") || !p.count("facing")) return std::nullopt;
    std::string dir;
    if (p["type"] == "left") dir = CLOCKWISE.at(p["facing"]);
    else if (p["type"] == "right") dir = COUNTER.at(p["facing"]);
    else return std::nullopt;
    auto [dx, dy, dz] = OFFSET.at(dir);
    return std::make_tuple(x + dx, y, z + dz);
}

void connectChest(Server& server, int x, int y, int z, bool sneaking) {
    World& world = server.world();
    int state = world.getBlock(x, y, z);
    if (!isChest(state) || sneaking) return;
    std::string facing = data::prop(state, "facing");
    for (auto [side, mine, theirs] : {std::tuple<std::string, const char*, const char*>{CLOCKWISE.at(facing), "left", "right"},
                                      {COUNTER.at(facing), "right", "left"}}) {
        auto [dx, dy, dz] = OFFSET.at(side);
        int other = world.getBlock(x + dx, y, z + dz);
        if (data::blockName(other) == data::blockName(state) && data::prop(other, "type") == "single" &&
            data::prop(other, "facing") == facing) {
            server.setBlockState(x, y, z, data::withProps(state, {{"type", mine}}), false);
            server.setBlockState(x + dx, y, z + dz, data::withProps(other, {{"type", theirs}}), false);
            return;
        }
    }
}

std::string stairShape(World& world, int x, int y, int z, int state) {
    std::string facing = data::prop(state, "facing"), half = data::prop(state, "half");
    auto at = [&](const std::string& d) {
        auto [dx, dy, dz] = OFFSET.at(d);
        return world.getBlock(x + dx, y, z + dz);
    };
    auto canTake = [&](const std::string& d) {
        int n = at(d);
        return !isStairs(n) || data::prop(n, "facing") != facing || data::prop(n, "half") != half;
    };
    int front = at(facing);
    if (isStairs(front) && data::prop(front, "half") == half) {
        std::string d2 = data::prop(front, "facing");
        if (d2 != facing && d2 != OPP.at(facing) && canTake(OPP.at(d2)))
            return d2 == COUNTER.at(facing) ? "outer_left" : "outer_right";
    }
    int back = at(OPP.at(facing));
    if (isStairs(back) && data::prop(back, "half") == half) {
        std::string d3 = data::prop(back, "facing");
        if (d3 != facing && d3 != OPP.at(facing) && canTake(d3)) return d3 == COUNTER.at(facing) ? "inner_left" : "inner_right";
    }
    return "straight";
}

void shapesListener(Server& server, int x, int y, int z) {
    World& world = server.world();
    int changed = world.getBlock(x, y, z);
    const int d[5][2] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (auto [dx, dz] : d) {
        int nx = x + dx, nz = z + dz;
        int s = world.getBlock(nx, y, nz);
        if (isStairs(s)) {
            std::string shape = stairShape(world, nx, y, nz, s);
            if (data::prop(s, "shape") != shape) server.setBlockState(nx, y, nz, data::withProps(s, {{"shape", shape}}), false);
        } else if (isChest(s) && (dx || dz)) {
            auto partner = chestPartner(nx, y, nz, s);
            if (partner && *partner == std::make_tuple(x, y, z) && data::blockName(changed) != data::blockName(s))
                server.setBlockState(nx, y, nz, data::withProps(s, {{"type", "single"}}), false);   // Partner abgebaut
        }
    }
}

std::optional<std::tuple<int, int, int>> otherPart(int x, int y, int z, int state) {
    auto p = data::props(state);
    if (p.count("half") && p["half"] == "lower") return std::make_tuple(x, y + 1, z);
    if (p.count("half") && p["half"] == "upper") return std::make_tuple(x, y - 1, z);
    if (p.count("part") && p.count("facing")) {
        auto [dx, dy, dz] = OFFSET.at(p["facing"]);
        return p["part"] == "foot" ? std::make_tuple(x + dx, y, z + dz) : std::make_tuple(x - dx, y, z - dz);
    }
    return std::nullopt;
}

}  // namespace placement
