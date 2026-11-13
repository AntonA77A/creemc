#include "blocks.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>

#include "buffer.hpp"
#include "data.hpp"
#include "entity.hpp"
#include "placement.hpp"
#include "player.hpp"
#include "server.hpp"
#include "text.hpp"
#include "vanilla.hpp"
#include "version.hpp"

namespace blocks {
namespace {

// Fenstertypen ("menu"-Registry, seit 1.20.3 in allen Versionen gleich)
enum Menu { GENERIC_9X3 = 2, GENERIC_9X6 = 5, GENERIC_3X3 = 6, BLAST_FURNACE = 10, FURNACE = 14, HOPPER = 16, SHULKER = 20, SMOKER = 22 };

struct ContainerInfo {
    int menu, size;
    const char* title;
};
const std::map<std::string, ContainerInfo> CONTAINERS = {
    {"minecraft:chest", {GENERIC_9X3, 27, "Kiste"}},       {"minecraft:trapped_chest", {GENERIC_9X3, 27, "Fallenkiste"}},
    {"minecraft:barrel", {GENERIC_9X3, 27, "Fass"}},       {"minecraft:dispenser", {GENERIC_3X3, 9, "Spender"}},
    {"minecraft:dropper", {GENERIC_3X3, 9, "Werfer"}},     {"minecraft:hopper", {HOPPER, 5, "Trichter"}},
};
struct FurnaceInfo {
    int menu;
    const char* kind;
    const char* title;
    int speed;
};
const std::map<std::string, FurnaceInfo> FURNACES = {
    {"minecraft:furnace", {FURNACE, "smelting", "Ofen", 1}},
    {"minecraft:smoker", {SMOKER, "smoking", "RÃ¤ucherofen", 2}},
    {"minecraft:blast_furnace", {BLAST_FURNACE, "blasting", "Schmelzofen", 2}},
};
const std::set<std::string> LIDS = {"minecraft:chest", "minecraft:trapped_chest", "minecraft:ender_chest"};

bool isChest(const std::string& b) { return b == "minecraft:chest" || b == "minecraft:trapped_chest"; }
bool isShulker(const std::string& b) { return endsWith(b, "shulker_box"); }

// ---- Speicher ---------------------------------------------------------------

json::Object& blockObj(Server& s, const BlockPos& pos) {
    json::Value& v = s.world().blockData[pos];
    if (!v.isObject()) v = json::Object();
    return v.asObject();
}

json::Array& itemsArray(json::Object& o, int size) {
    json::Value& items = o["items"];
    if (!items.isArray() || int(items.size()) != size) items = json::Array(size_t(size), json::Value());
    return items.asArray();
}

Stack fromJson(const json::Value& v) {
    if (!v.isArray() || v.size() < 2) return {};
    std::string item = v[0].str();
    int n = int(v[1].asInt());
    if (item.empty() || n <= 0) return {};
    return {item, n};
}
json::Value toJson(const Stack& st) {
    if (st.first.empty() || st.second <= 0) return json::Value();
    return json::Array{st.first, st.second};
}

class BlockStorage : public Storage {
public:
    BlockStorage(Server& s, BlockPos p, int n) : s_(s), pos_(p), n_(n) { itemsArray(blockObj(s_, pos_), n_); }
    int size() const override { return n_; }
    Stack get(int i) const override {
        auto it = s_.world().blockData.find(pos_);
        if (it == s_.world().blockData.end() || i < 0 || i >= n_) return {};
        const json::Value& items = it->second["items"];
        return items.isArray() && int(items.size()) == n_ ? fromJson(items[size_t(i)]) : Stack{};
    }
    void set(int i, const Stack& st) override {
        if (i < 0 || i >= n_) return;
        itemsArray(blockObj(s_, pos_), n_)[size_t(i)] = toJson(st);
    }
    bool covers(const BlockPos& p) const override { return p == pos_; }

private:
    Server& s_;
    BlockPos pos_;
    int n_;
};

class CompoundStorage : public Storage {   // Doppelkiste: obere HÃ¤lfte zuerst
public:
    CompoundStorage(std::unique_ptr<Storage> a, std::unique_ptr<Storage> b) : a_(std::move(a)), b_(std::move(b)) {}
    int size() const override { return a_->size() + b_->size(); }
    Stack get(int i) const override { return i < a_->size() ? a_->get(i) : b_->get(i - a_->size()); }
    void set(int i, const Stack& st) override {
        if (i < a_->size()) a_->set(i, st);
        else b_->set(i - a_->size(), st);
    }
    bool covers(const BlockPos& p) const override { return a_->covers(p) || b_->covers(p); }

private:
    std::unique_ptr<Storage> a_, b_;
};

class EnderStorage : public Storage {   // Endertruhe: gehÃ¶rt dem Spieler
public:
    explicit EnderStorage(Player& p) : p_(p) {}
    int size() const override { return 27; }
    Stack get(int i) const override {
        const json::Value& a = p_.pluginData["ender_chest"];
        return a.isArray() && a.size() == 27 ? fromJson(a[size_t(i)]) : Stack{};
    }
    void set(int i, const Stack& st) override {
        if (!p_.pluginData.isObject()) p_.pluginData = json::Object();
        json::Object& o = p_.pluginData.asObject();
        json::Value& a = o["ender_chest"];
        if (!a.isArray() || a.size() != 27) a = json::Array(27, json::Value());
        a.asArray()[size_t(i)] = toJson(st);
    }

private:
    Player& p_;
};

std::unique_ptr<Storage> chestStorage(Server& s, int x, int y, int z, std::vector<BlockPos>& positions) {
    World& w = s.world();
    int state = w.getBlock(x, y, z);
    auto partner = placement::chestPartner(x, y, z, state);
    if (partner) {
        auto [px, py, pz] = *partner;
        if (data::blockName(w.getBlock(px, py, pz)) == data::blockName(state)) {
            BlockPos me{x, y, z}, other{px, py, pz};
            BlockPos right = data::prop(state, "type") == "right" ? me : other;   // rechte HÃ¤lfte = oben
            BlockPos left = right == me ? other : me;
            positions = {right, left};
            return std::make_unique<CompoundStorage>(blockStorage(s, right, 27), blockStorage(s, left, 27));
        }
    }
    positions = {{x, y, z}};
    return blockStorage(s, {x, y, z}, 27);
}

// Inhalt eines BehÃ¤lters fÃ¼r Trichter/Werfer; kind: "chest", "container", "furnace"
std::unique_ptr<Storage> inventoryAt(Server& s, int x, int y, int z, std::string& kind) {
    std::string block = data::blockName(s.world().getBlock(x, y, z));
    if (isChest(block)) {
        std::vector<BlockPos> pos;
        kind = "chest";
        return chestStorage(s, x, y, z, pos);
    }
    auto c = CONTAINERS.find(block);
    if (c != CONTAINERS.end()) {
        kind = "container";
        return blockStorage(s, {x, y, z}, c->second.size);
    }
    if (isShulker(block)) {
        kind = "container";
        return blockStorage(s, {x, y, z}, 27);
    }
    auto f = FURNACES.find(block);
    if (f != FURNACES.end()) {
        kind = "furnace";
        json::Object& o = blockObj(s, {x, y, z});
        if (!o.contains("furnace")) o["furnace"] = f->second.kind;
        return blockStorage(s, {x, y, z}, 3);
    }
    return nullptr;
}

bool insertOne(Storage& st, const std::string& kind, const std::string& item, bool fromAbove) {
    std::vector<int> slots;
    if (kind == "furnace") {
        if (fromAbove) slots = {0};
        else if (fuelTicks(item)) slots = {1};
    } else {
        for (int i = 0; i < st.size(); ++i) slots.push_back(i);
    }
    for (bool wantEmpty : {false, true})
        for (int i : slots) {
            Stack s = st.get(i);
            if (!wantEmpty && !s.first.empty() && s.first == item && s.second < data::stackSize(item)) {
                st.set(i, {item, s.second + 1});
                return true;
            }
            if (wantEmpty && s.first.empty()) {
                st.set(i, {item, 1});
                return true;
            }
        }
    return false;
}

void blockAction(Server& s, int x, int y, int z, int action, int param) {
    std::string name = data::blockName(s.world().getBlock(x, y, z));
    s.broadcastPacketV(
        "block_action",
        [&](const Version& v) {
            Writer w;
            auto it = v.blockIds.find(name);
            w.position(x, y, z).u8(uint8_t(action)).u8(uint8_t(param)).varint(it == v.blockIds.end() ? 0 : it->second);
            return w.out;
        },
        nullptr, [&](Player* p) { return p->hasChunk(x >> 4, z >> 4); });
}

int countViewers(Server& s, const BlockPos& pos) {
    int n = 0;
    for (Player* p : s.players()) {
        auto* cw = dynamic_cast<ContainerWindow*>(p->window.get());
        if (cw && std::find(cw->positions.begin(), cw->positions.end(), pos) != cw->positions.end()) ++n;
    }
    return n;
}

void sound(Server& s, const std::string& name, int x, int y, int z, float vol = 0.5f, float pitch = -1) {
    s.playSound("minecraft:" + name, x + 0.5, y + 0.5, z + 0.5, vol, pitch < 0 ? float(randDouble(0.9, 1.0)) : pitch, 4);
}

// ---- Ã–fen -------------------------------------------------------------------

void sendFurnaceProgress(Player& p, const json::Object& d, int speed) {
    auto num = [&](const char* k, int def) {
        const json::Value* v = d.find(k);
        return v ? int(v->asInt(def)) : def;
    };
    p.sendWindowProperty(0, num("burn", 0));
    p.sendWindowProperty(1, num("burn_max", 0));
    p.sendWindowProperty(2, num("cook", 0));
    p.sendWindowProperty(3, num("cook_max", 200 / speed));
}

void furnaceXp(Server& s, Player& p, const BlockPos& pos) {
    // Gesammelte Ofen-Erfahrung bekommt, wer Ergebnisse herausgenommen hat (Bruchteile per Zufall)
    auto it = s.world().blockData.find(pos);
    if (it == s.world().blockData.end() || !it->second.isObject()) return;
    json::Object& d = it->second.asObject();
    Stack out = d.find("items") && (*d.find("items")).isArray() && d.find("items")->size() == 3 ? fromJson((*d.find("items"))[2]) : Stack{};
    int now = out.second;
    int last = d.find("out_last") ? int(d.find("out_last")->asInt()) : 0;
    double xp = d.find("xp") ? d.find("xp")->asDouble() : 0;
    if (now >= last || xp <= 0) return;
    d["xp"] = 0.0;
    d["out_last"] = now;
    int whole = int(xp);
    p.giveXp(whole + (randDouble() < xp - whole ? 1 : 0));
}

void tickFurnaces(Server& s) {
    World& w = s.world();
    std::vector<BlockPos> furnaces;
    for (auto& [pos, v] : w.blockData)
        if (v.isObject() && v.asObject().contains("furnace")) furnaces.push_back(pos);
    for (const BlockPos& pos : furnaces) {
        auto found = w.blockData.find(pos);
        if (found == w.blockData.end() || !found->second.isObject()) continue;
        json::Object& d = found->second.asObject();
        const json::Value* kindV = d.find("furnace");
        if (!kindV || !kindV->isString()) continue;
        std::string kind = kindV->asString();
        int state = w.getBlock(pos.x, pos.y, pos.z);
        auto f = FURNACES.find(data::blockName(state));
        if (f == FURNACES.end()) continue;
        int speed = f->second.speed;
        BlockStorage items(s, pos, 3);
        Stack inp = items.get(0), fuel = items.get(1), out = items.get(2);
        const RecipeBook::Cooking* recipe = inp.first.empty() ? nullptr : s.recipes().cook(kind, inp.first);
        bool canCook = recipe && (out.first.empty() || (out.first == recipe->result && out.second + recipe->count <= data::stackSize(out.first)));
        auto num = [&](const char* k) { const json::Value* x = d.find(k); return x ? int(x->asInt()) : 0; };
        int burn = num("burn"), cook = num("cook");
        bool changed = false;
        if (burn > 0) --burn;
        if (burn <= 0 && canCook && !fuel.first.empty() && fuelTicks(fuel.first) > 0) {
            burn = fuelTicks(fuel.first) / speed;
            d["burn_max"] = burn;
            if (fuel.first == "minecraft:lava_bucket") items.set(1, {"minecraft:bucket", 1});
            else items.set(1, {fuel.first, fuel.second - 1});
            changed = true;
        }
        if (burn > 0 && canCook) {
            int cookMax = std::max(1, recipe->ticks / speed);
            d["cook_max"] = cookMax;
            if (++cook >= cookMax) {
                cook = 0;
                items.set(0, {inp.first, inp.second - 1});
                Stack res{recipe->result, (out.first.empty() ? 0 : out.second) + recipe->count};
                items.set(2, res);
                const json::Value* xpv = d.find("xp");
                d["xp"] = (xpv ? xpv->asDouble() : 0.0) + recipe->xp;
                d["out_last"] = res.second;
                changed = true;
            }
        } else if (cook > 0) {
            cook = std::max(0, cook - 2);
        }
        d["burn"] = burn;
        d["cook"] = cook;
        bool lit = burn > 0;
        if (data::hasProp(state, "lit") && data::prop(state, "lit") != (lit ? "true" : "false"))
            s.setBlockState(pos.x, pos.y, pos.z, data::withProps(state, {{"lit", lit ? "true" : "false"}}));
        if (!lit && inp.first.empty() && cook == 0 && !changed) continue;
        for (Player* p : s.players()) {
            auto* cw = dynamic_cast<FurnaceWindow*>(p->window.get());
            if (!cw || cw->positions.empty() || !(cw->positions[0] == pos)) continue;
            if (changed) p->syncInventory();
            sendFurnaceProgress(*p, d, speed);
        }
    }
}

// ---- Trichter ---------------------------------------------------------------

bool dirOffset(const std::string& facing, int& dx, int& dy, int& dz) {
    dx = dy = dz = 0;
    if (facing == "down") dy = -1;
    else if (facing == "up") dy = 1;
    else if (facing == "north") dz = -1;
    else if (facing == "south") dz = 1;
    else if (facing == "west") dx = -1;
    else if (facing == "east") dx = 1;
    else return false;
    return true;
}

void tickHoppers(Server& s) {
    World& w = s.world();
    std::vector<BlockPos> hoppers;
    for (auto& [pos, v] : w.blockData)
        if (data::blockName(w.getBlock(pos.x, pos.y, pos.z)) == "minecraft:hopper") hoppers.push_back(pos);
    for (const BlockPos& pos : hoppers) {
        int state = w.getBlock(pos.x, pos.y, pos.z);
        if (data::prop(state, "enabled") == "false") continue;   // durch Redstone gesperrt
        BlockStorage items(s, pos, 5);
        bool changed = false;
        int dx, dy, dz;
        if (!dirOffset(data::prop(state, "facing"), dx, dy, dz)) dy = -1;
        std::string kind;
        auto target = inventoryAt(s, pos.x + dx, pos.y + dy, pos.z + dz, kind);
        if (target) {
            for (int i = 0; i < 5; ++i) {
                Stack st = items.get(i);
                if (!st.first.empty() && insertOne(*target, kind, st.first, dy < 0)) {
                    items.set(i, {st.first, st.second - 1});
                    changed = true;
                    syncViewers(s, nullptr, {{pos.x + dx, pos.y + dy, pos.z + dz}});
                    break;
                }
            }
        }
        std::string skind;
        auto source = inventoryAt(s, pos.x, pos.y + 1, pos.z, skind);
        if (source) {
            for (int i = 0; i < source->size(); ++i) {
                if (skind == "furnace" && i != 2) continue;
                Stack st = source->get(i);
                if (!st.first.empty() && insertOne(items, "container", st.first, true)) {
                    source->set(i, {st.first, st.second - 1});
                    syncViewers(s, nullptr, {{pos.x, pos.y + 1, pos.z}});
                    changed = true;
                    break;
                }
            }
        } else {
            for (Entity* e : s.entities()) {
                auto* it = dynamic_cast<ItemEntity*>(e);
                if (!it || int(std::floor(it->x)) != pos.x || int(std::floor(it->z)) != pos.z || it->y < pos.y + 1 || it->y >= pos.y + 2)
                    continue;
                int before = it->count;
                while (it->count > 0 && insertOne(items, "container", it->item, true)) --it->count;
                if (it->count <= 0) it->remove();
                else if (it->count != before)
                    s.broadcastPacketV("entity_metadata", [it](const Version& v) { return v.itemMetadata(it->entityId, it->item, it->count); });
                if (it->count != before) changed = true;
            }
        }
        if (changed) syncViewers(s, nullptr, {pos});
    }
}

// ---- Druckplatten -----------------------------------------------------------

std::map<BlockPos, int64_t> g_plates;   // Position -> letzter Tick mit etwas darauf

void tickPlates(Server& s) {
    World& w = s.world();
    int64_t tick = s.tick();
    std::map<BlockPos, int> counts;
    auto visit = [&](double ex, double ey, double ez, int n) {
        BlockPos p{int(std::floor(ex)), int(std::floor(ey)), int(std::floor(ez))};
        if (endsWith(data::blockName(w.getBlock(p.x, p.y, p.z)), "_pressure_plate")) counts[p] += n;
    };
    for (Player* p : s.players())
        if (!p->dead && p->gamemode != 3) visit(p->x, p->y, p->z, 1);
    for (Entity* e : s.entities()) {
        auto* it = dynamic_cast<ItemEntity*>(e);
        visit(e->x, e->y, e->z, it ? it->count : 1);
    }
    for (auto& [pos, n] : counts) {
        g_plates[pos] = tick;
        int st = w.getBlock(pos.x, pos.y, pos.z);
        if (data::hasProp(st, "powered") && data::prop(st, "powered") == "false") {
            s.setBlockState(pos.x, pos.y, pos.z, data::withProps(st, {{"powered", "true"}}));
            s.playSound("minecraft:block.wooden_pressure_plate.click_on", pos.x + .5, pos.y + .1, pos.z + .5, .3f, .8f, 4);
        } else if (data::hasProp(st, "power")) {
            bool light = data::blockName(st).find("light") != std::string::npos;
            std::string level = std::to_string(std::min(15, light ? n : (n + 9) / 10));
            if (data::prop(st, "power") != level) s.setBlockState(pos.x, pos.y, pos.z, data::withProps(st, {{"power", level}}));
        }
    }
    for (auto it = g_plates.begin(); it != g_plates.end();) {
        if (tick - it->second > 20 && !counts.count(it->first)) {
            BlockPos pos = it->first;
            it = g_plates.erase(it);
            int st = w.getBlock(pos.x, pos.y, pos.z);
            if (data::prop(st, "powered") == "true") {
                s.setBlockState(pos.x, pos.y, pos.z, data::withProps(st, {{"powered", "false"}}));
                s.playSound("minecraft:block.wooden_pressure_plate.click_off", pos.x + .5, pos.y + .1, pos.z + .5, .3f, .7f, 4);
            } else if (data::hasProp(st, "power") && data::prop(st, "power") != "0") {
                s.setBlockState(pos.x, pos.y, pos.z, data::withProps(st, {{"power", "0"}}));
            }
        } else {
            ++it;
        }
    }
}

// ---- TÃ¼ren, Hebel, KnÃ¶pfe ... -----------------------------------------------

bool toggle(Server& s, const std::string& block, int x, int y, int z) {
    World& w = s.world();
    int state = w.getBlock(x, y, z);
    auto p = data::props(state);
    std::string name = removePrefix(block, "minecraft:");
    bool woodenOpenable = (endsWith(name, "_door") || endsWith(name, "_trapdoor") || endsWith(name, "_fence_gate")) &&
                          !startsWith(name, "iron_");
    if (p.count("open") && woodenOpenable) {
        std::string nw = p["open"] == "true" ? "false" : "true";
        s.setBlockState(x, y, z, data::withProps(state, {{"open", nw}}));
        if (endsWith(name, "_door")) {   // die andere TÃ¼rhÃ¤lfte mitschalten
            int oy = p["half"] == "lower" ? y + 1 : y - 1;
            int other = w.getBlock(x, oy, z);
            if (data::blockName(other) == block) s.setBlockState(x, oy, z, data::withProps(other, {{"open", nw}}));
        }
        std::string kind = endsWith(name, "fence_gate") ? "fence_gate" : endsWith(name, "trapdoor") ? "wooden_trapdoor" : "wooden_door";
        sound(s, "block." + kind + (nw == "true" ? ".open" : ".close"), x, y, z, 1.0f);
        return true;
    }
    if (endsWith(name, "_button") && p["powered"] == "false") {
        s.setBlockState(x, y, z, data::withProps(state, {{"powered", "true"}}));
        bool wooden = !startsWith(name, "stone_") && !startsWith(name, "polished_blackstone_");
        std::string kind = wooden ? "wooden_button" : "stone_button";
        sound(s, "block." + kind + ".click_on", x, y, z, 0.3f, 0.6f);
        s.runLater(wooden ? 30 : 20, [&s, block, kind, x, y, z] {
            int st = s.world().getBlock(x, y, z);
            if (data::blockName(st) == block && data::prop(st, "powered") == "true") {
                s.setBlockState(x, y, z, data::withProps(st, {{"powered", "false"}}));
                sound(s, "block." + kind + ".click_off", x, y, z, 0.3f, 0.5f);
            }
        });
        return true;
    }
    if (name == "repeater") {   // VerzÃ¶gerung 1 -> 2 -> 3 -> 4 -> 1
        int d = std::atoi(p["delay"].c_str());
        s.setBlockState(x, y, z, data::withProps(state, {{"delay", std::to_string(d % 4 + 1)}}));
        return true;
    }
    if (name == "comparator") {
        std::string mode = p["mode"] == "subtract" ? "compare" : "subtract";
        s.setBlockState(x, y, z, data::withProps(state, {{"mode", mode}}));
        sound(s, "block.comparator.click", x, y, z, 0.3f, mode == "subtract" ? 0.55f : 0.5f);
        return true;
    }
    if (name == "note_block") {   // Ton hÃ¶her (0-24), dann spielen
        int note = (std::atoi(p["note"].c_str()) + 1) % 25;
        s.setBlockState(x, y, z, data::withProps(state, {{"note", std::to_string(note)}}), false);
        std::string inst = p["instrument"].empty() ? "harp" : p["instrument"];
        float pitch = float(std::pow(2.0, (note - 12) / 12.0));
        s.playSound("minecraft:block.note_block." + inst, x + 0.5, y + 0.5, z + 0.5, 3.0f, pitch, 2);
        return true;
    }
    if (name == "lever" && p.count("powered")) {
        std::string nw = p["powered"] == "true" ? "false" : "true";
        s.setBlockState(x, y, z, data::withProps(state, {{"powered", nw}}));
        sound(s, "block.lever.click", x, y, z, 0.3f, nw == "true" ? 0.6f : 0.5f);
        return true;
    }
    return false;
}

}  // namespace

std::unique_ptr<Storage> blockStorage(Server& server, const BlockPos& pos, int size) {
    return std::make_unique<BlockStorage>(server, pos, size);
}

// ---- Fenster ----------------------------------------------------------------

ContainerWindow::ContainerWindow(Player& p, int id, std::unique_ptr<Storage> st, int menuType, const std::string& t)
    : Window(p, id, 0, 0), storage(std::move(st)) {
    n = storage->size();
    size = n + 36;
    typeId = menuType;
    title = t;
}

std::optional<int> ContainerWindow::invSlot(int i) const {
    if (i >= n && i < n + 27) return i - n + 9;          // Inventar 9-35
    if (i >= n + 27 && i < n + 36) return i - n - 27 + 36;   // Hotbar 36-44
    return std::nullopt;
}

std::vector<int> ContainerWindow::quickMoveTargets(int i) const {
    std::vector<int> v;
    if (i < n)   // aus der Kiste ins Inventar (Hotbar zuletzt, wie Vanilla)
        for (int k = size - 1; k >= n; --k) v.push_back(k);
    else
        for (int k = 0; k < n; ++k) v.push_back(k);
    return v;
}

std::vector<Stack> ContainerWindow::returnItems() {
    std::vector<Stack> stacks, leftovers;
    if (!player.cursor.first.empty()) stacks.push_back(player.cursor);
    player.cursor = {};
    if (temporary)
        for (int i = 0; i < n; ++i) {
            Stack st = storage->get(i);
            if (!st.first.empty() && i != resultSlot) stacks.push_back(st);
            storage->set(i, {});
        }
    for (const auto& st : stacks) {
        int rest = player.addItem(st.first, st.second, false);
        if (rest) leftovers.push_back({st.first, rest});
    }
    return leftovers;
}

FurnaceWindow::FurnaceWindow(Player& p, int id, std::unique_ptr<Storage> st, int menuType, const std::string& t, std::string k)
    : ContainerWindow(p, id, std::move(st), menuType, t), kind(std::move(k)) {}

bool FurnaceWindow::canPlace(int i, const std::string& item) const {
    if (i == 2) return false;
    if (i == 1) return fuelTicks(item) > 0;
    return true;
}

std::vector<int> FurnaceWindow::quickMoveTargets(int i) const {
    std::vector<int> v;
    if (i < 3) {
        for (int k = size - 1; k >= 3; --k) v.push_back(k);
        return v;
    }
    Stack st = get(i);
    if (!st.first.empty() && player.server.recipes().cook(kind, st.first)) return {0};
    if (!st.first.empty() && fuelTicks(st.first)) return {1};
    if (i < 3 + 27)
        for (int k = 30; k < 39; ++k) v.push_back(k);
    else
        for (int k = 3; k < 30; ++k) v.push_back(k);
    return v;
}

// ---- Ã–ffentliche Funktionen -------------------------------------------------

int fuelTicks(const std::string& item) {
    static const std::map<std::string, int> EXACT = {
        {"lava_bucket", 20000}, {"coal_block", 16000}, {"dried_kelp_block", 4001}, {"blaze_rod", 2400}, {"coal", 1600},
        {"charcoal", 1600}, {"crafting_table", 300}, {"bookshelf", 300}, {"chest", 300}, {"trapped_chest", 300},
        {"barrel", 300}, {"ladder", 300}, {"jukebox", 300}, {"note_block", 300}, {"bow", 300}, {"crossbow", 300},
        {"fishing_rod", 300}, {"lectern", 300}, {"composter", 300}, {"stick", 100}, {"bowl", 100}, {"bamboo", 50},
        {"scaffolding", 50}, {"wooden_sword", 200}, {"wooden_axe", 200}, {"wooden_pickaxe", 200}, {"wooden_shovel", 200},
        {"wooden_hoe", 200},
    };
    static const char* WOODS[] = {"oak_", "spruce_", "birch_", "jungle_", "acacia_", "dark_oak_", "mangrove_", "cherry_",
                                  "pale_oak_", "bamboo_", "poplar_", "stripped_"};
    static const std::pair<const char*, int> WOOD_SUFFIX[] = {
        {"_log", 300}, {"_wood", 300}, {"_planks", 300}, {"_slab", 150}, {"_stairs", 300}, {"_fence_gate", 300},
        {"_fence", 300}, {"_trapdoor", 300}, {"_door", 200}, {"_pressure_plate", 300}, {"_button", 100},
        {"_hanging_sign", 800}, {"_sign", 200}, {"_chest_boat", 1200}, {"_boat", 1200}, {"_raft", 1200}, {"_block", 300}};
    static const std::pair<const char*, int> ANY_SUFFIX[] = {{"_sapling", 100}, {"_wool", 100}, {"_carpet", 67}, {"_banner", 300}};
    std::string n = removePrefix(data::baseItem(item), "minecraft:");
    auto it = EXACT.find(n);
    if (it != EXACT.end()) return it->second;
    for (const char* w : WOODS)
        if (startsWith(n, w)) {   // Netherholz (crimson/warped) brennt in Vanilla nicht
            for (const auto& [suf, t] : WOOD_SUFFIX)
                if (endsWith(n, suf)) return t;
            break;
        }
    for (const auto& [suf, t] : ANY_SUFFIX)
        if (endsWith(n, suf)) return t;
    return 0;
}

bool isInteractive(const std::string& block) {
    std::string n = removePrefix(block, "minecraft:");
    return CONTAINERS.count(block) || FURNACES.count(block) || block == "minecraft:ender_chest" ||
           block == "minecraft:crafting_table" || block == "minecraft:enchanting_table" || endsWith(n, "anvil") ||
           endsWith(n, "shulker_box") || endsWith(n, "_button") || n == "lever" || n == "repeater" || n == "comparator" ||
           n == "note_block" ||
           ((endsWith(n, "_door") || endsWith(n, "_trapdoor") || endsWith(n, "_fence_gate")) && !startsWith(n, "iron_"));
}

bool openBlock(Player& p, const std::string& block, int x, int y, int z) {
    Server& s = p.server;
    if (block == "minecraft:crafting_table") {
        p.openWindow(std::make_unique<CraftingWindow>(p, p.newWindowId()));
        return true;
    }
    if (isChest(block)) {
        std::vector<BlockPos> positions;
        auto st = chestStorage(s, x, y, z, positions);
        bool dbl = positions.size() == 2;
        auto win = std::make_unique<ContainerWindow>(p, p.newWindowId(), std::move(st), dbl ? GENERIC_9X6 : GENERIC_9X3,
                                                     std::string(dbl ? "GroÃŸe " : "") + CONTAINERS.at(block).title);
        win->positions = positions;
        p.openWindow(std::move(win));
        int viewers = countViewers(s, positions[0]);
        for (const auto& pos : positions) blockAction(s, pos.x, pos.y, pos.z, 1, viewers);
        sound(s, "block.chest.open", x, y, z);
        return true;
    }
    std::unique_ptr<ContainerWindow> win;
    auto c = CONTAINERS.find(block);
    auto f = FURNACES.find(block);
    if (c != CONTAINERS.end()) {
        win = std::make_unique<ContainerWindow>(p, p.newWindowId(), blockStorage(s, {x, y, z}, c->second.size), c->second.menu,
                                                c->second.title);
    } else if (isShulker(block)) {
        win = std::make_unique<ContainerWindow>(p, p.newWindowId(), blockStorage(s, {x, y, z}, 27), SHULKER, "Shulkerkiste");
    } else if (block == "minecraft:ender_chest") {
        win = std::make_unique<ContainerWindow>(p, p.newWindowId(), std::make_unique<EnderStorage>(p), GENERIC_9X3, "Endertruhe");
    } else if (f != FURNACES.end()) {
        json::Object& d = blockObj(s, {x, y, z});
        if (!d.contains("furnace")) d["furnace"] = f->second.kind;
        auto fw = std::make_unique<FurnaceWindow>(p, p.newWindowId(), blockStorage(s, {x, y, z}, 3), f->second.menu,
                                                  f->second.title, f->second.kind);
        fw->positions = {{x, y, z}};
        p.openWindow(std::move(fw));
        sendFurnaceProgress(p, blockObj(s, {x, y, z}), f->second.speed);
        return true;
    } else {
        return toggle(s, block, x, y, z);
    }
    win->positions = {{x, y, z}};
    p.openWindow(std::move(win));
    if (LIDS.count(block) || isShulker(block)) {
        blockAction(s, x, y, z, 1, countViewers(s, {x, y, z}));
        sound(s, block == "minecraft:ender_chest" ? "block.ender_chest.open" : isShulker(block) ? "block.shulker_box.open" : "block.chest.open", x, y, z);
    } else if (block == "minecraft:barrel") {
        s.setBlockState(x, y, z, data::withProps(s.world().getBlock(x, y, z), {{"open", "true"}}));
        sound(s, "block.barrel.open", x, y, z, 0.5f, 1.0f);
    }
    return true;
}

void onClose(Player& p, Window& w) {
    auto* cw = dynamic_cast<ContainerWindow*>(&w);
    if (!cw || cw->positions.empty()) return;
    Server& s = p.server;
    BlockPos pos = cw->positions[0];
    std::vector<BlockPos> positions = cw->positions;
    cw->positions.clear();   // zÃ¤hlt nicht mehr als Zuschauer
    std::string block = data::blockName(s.world().getBlock(pos.x, pos.y, pos.z));
    int left = countViewers(s, pos);
    if (FURNACES.count(block)) furnaceXp(s, p, pos);
    if (LIDS.count(block) || isShulker(block)) {
        for (const auto& q : positions) blockAction(s, q.x, q.y, q.z, 1, left);
        if (left == 0)
            sound(s, isShulker(block) ? "block.shulker_box.close" : block == "minecraft:ender_chest" ? "block.ender_chest.close" : "block.chest.close",
                  pos.x, pos.y, pos.z);
    } else if (block == "minecraft:barrel" && left == 0) {
        s.setBlockState(pos.x, pos.y, pos.z, data::withProps(s.world().getBlock(pos.x, pos.y, pos.z), {{"open", "false"}}));
        sound(s, "block.barrel.close", pos.x, pos.y, pos.z, 0.5f, 1.0f);
    }
}

void onPlaced(Server& s, int x, int y, int z, const std::string& block) {
    // Trichter/Spender/Werfer brauchen ihren Speicher sofort, damit sie arbeiten
    auto c = CONTAINERS.find(block);
    if (c != CONTAINERS.end() && !isChest(block)) blockStorage(s, {x, y, z}, c->second.size);
}

void dropContents(Server& s, int x, int y, int z) {
    auto it = s.world().blockData.find({x, y, z});
    if (it == s.world().blockData.end()) return;
    const json::Value& items = it->second["items"];
    if (items.isArray())
        for (const auto& v : items.asArray()) {
            Stack st = fromJson(v);
            if (!st.first.empty()) s.dropItem(x + 0.5, y + 0.5, z + 0.5, st.first, st.second, 10, true);
        }
    for (Player* p : s.players()) {
        auto* cw = dynamic_cast<ContainerWindow*>(p->window.get());
        if (cw && std::find(cw->positions.begin(), cw->positions.end(), BlockPos{x, y, z}) != cw->positions.end())
            p->closeWindow(true);
    }
    s.world().blockData.erase({x, y, z});
}

void syncViewers(Server& s, Player* except, const std::vector<BlockPos>& positions) {
    for (Player* p : s.players()) {
        if (p == except) continue;
        auto* cw = dynamic_cast<ContainerWindow*>(p->window.get());
        if (!cw) continue;
        bool hit = false;
        for (const auto& pos : positions)
            if (cw->storage->covers(pos)) hit = true;
        if (hit) p->syncInventory();
    }
}

void dispense(Server& s, int x, int y, int z) {
    World& w = s.world();
    int state = w.getBlock(x, y, z);
    std::string block = data::blockName(state);
    if (block != "minecraft:dispenser" && block != "minecraft:dropper") return;
    BlockStorage items(s, {x, y, z}, 9);
    std::vector<int> full;
    for (int i = 0; i < 9; ++i)
        if (!items.get(i).first.empty()) full.push_back(i);
    if (full.empty()) {
        s.playSound("minecraft:block.dispenser.fail", x + .5, y + .5, z + .5, 1.0f, 1.2f, 4);
        return;
    }
    int slot = full[size_t(randInt(0, int(full.size()) - 1))];
    Stack st = items.get(slot);
    std::string item = data::baseItem(st.first);
    int dx, dy, dz;
    if (!dirOffset(data::prop(state, "facing"), dx, dy, dz)) dz = -1;
    int fx = x + dx, fy = y + dy, fz = z + dz;
    auto used = [&](const Stack& replace) {
        items.set(slot, !replace.first.empty() ? replace : Stack{st.first, st.second - 1});
        syncViewers(s, nullptr, {{x, y, z}});
    };
    if (block == "minecraft:dropper") {
        std::string kind;
        auto target = inventoryAt(s, fx, fy, fz, kind);
        if (target) {   // Werfer fÃ¼llt BehÃ¤lter vor sich
            if (insertOne(*target, kind, st.first, dy < 0)) {
                used({});
                syncViewers(s, nullptr, {{fx, fy, fz}});
            }
            return;
        }
    } else if (item == "minecraft:tnt") {
        vanilla::spawnTnt(s, fx + 0.5, fy, fz + 0.5);
        used({});
        return;
    } else if (endsWith(item, "_spawn_egg")) {
        std::string kind = removePrefix(item, "minecraft:");
        kind = kind.substr(0, kind.size() - 10);
        if (vanilla::isMobType(kind)) {
            vanilla::spawnMobs(s, kind, fx + 0.5, fy, fz + 0.5, 1);
            used({});
        }
        return;
    } else if (item == "minecraft:water_bucket" || item == "minecraft:lava_bucket") {
        if (w.getBlock(fx, fy, fz) == 0) {
            s.setBlock(fx, fy, fz, item == "minecraft:water_bucket" ? "minecraft:water" : "minecraft:lava");
            used({"minecraft:bucket", 1});
        }
        return;
    }
    s.dropItemVelocity(fx + 0.5, fy + 0.3, fz + 0.5, st.first, 1, 10, dx * 0.3 + randDouble(-.05, .05), dy * 0.3 + 0.1,
                       dz * 0.3 + randDouble(-.05, .05));
    s.playSound("minecraft:block.dispenser.dispense", x + .5, y + .5, z + .5, 1.0f, 1.0f, 4);
    used({});
}

void tick(Server& s) {
    int64_t t = s.tick();
    tickFurnaces(s);
    if (t % 8 == 0) tickHoppers(s);
    if (t % 4 == 0) tickPlates(s);
}

}  // namespace blocks
