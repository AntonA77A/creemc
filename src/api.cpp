#include "api.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>

#include "blocks.hpp"
#include "buffer.hpp"
#include "data.hpp"
#include "entity.hpp"
#include "player.hpp"
#include "server.hpp"
#include "text.hpp"
#include "vanilla.hpp"
#include "version.hpp"

namespace api {
namespace {

// ---- Eigene Inventare (GUIs) ------------------------------------------------
// Inhalt gehört dem Plugin (Java); hier liegt eine gemeinsame Kopie, die alle Zuschauer sehen.

std::map<int, std::shared_ptr<std::vector<Stack>>> g_guis;

class GuiStorage : public blocks::Storage {
public:
    explicit GuiStorage(std::shared_ptr<std::vector<Stack>> d) : d_(std::move(d)) {}
    int size() const override { return int(d_->size()); }
    Stack get(int i) const override { return (*d_)[size_t(i)]; }
    void set(int i, const Stack& st) override { (*d_)[size_t(i)] = st.second > 0 ? st : Stack{}; }

private:
    std::shared_ptr<std::vector<Stack>> d_;
};

class GuiWindow : public blocks::ContainerWindow {
public:
    GuiWindow(Player& p, int id, int guiId, std::shared_ptr<std::vector<Stack>> d, int menu, const std::string& t)
        : ContainerWindow(p, id, std::make_unique<GuiStorage>(std::move(d)), menu, t), gui(guiId) {}
    int gui;
};

Stack stackOf(const json::Value& v) {
    if (!v.isArray() || v.size() < 2 || !v[0].isString()) return {};
    std::string item = v[0].asString();
    int n = int(v[1].asInt(1));
    if (!data::isItem(item) || n <= 0) return {};
    return {ns(item), n};
}
json::Value stackJson(const Stack& st) {
    if (st.first.empty() || st.second <= 0) return json::Value();
    return json::Array{st.first, st.second};
}

void syncGui(Server& s, int gui) {
    for (Player* p : s.players())
        if (auto* w = dynamic_cast<GuiWindow*>(p->window.get()); w && w->gui == gui) p->syncInventory();
}

std::string windowType(Window& w) {
    if (dynamic_cast<GuiWindow*>(&w)) return "gui";
    if (dynamic_cast<blocks::FurnaceWindow*>(&w)) return "furnace";
    if (auto* c = dynamic_cast<blocks::ContainerWindow*>(&w)) {
        if (c->typeId == 16) return "hopper";
        if (c->typeId == 6) return "dispenser";
        if (c->typeId == 8) return "anvil";
        if (c->typeId == 13) return "enchanting";
        if (c->typeId == 20) return "shulker_box";
        return c->title == "Endertruhe" ? "ender_chest" : "chest";
    }
    if (w.id == 0) return "player";
    return "workbench";
}

json::Array windowItems(Window& w) {
    json::Array a;
    int top = w.size - 36;
    for (int i = 0; i < std::max(0, top); ++i) a.push_back(stackJson(w.get(i)));
    return a;
}

// ---- Pakete -----------------------------------------------------------------

Bytes nbtText(const std::string& s) { return text::toNbt(s); }

void sendBossBar(Player& p, const json::Value& m) {
    Writer w;
    w.uuid(Uuid::parse(m["id"].str())).varint(int32_t(m["action"].asInt()));
    switch (int(m["action"].asInt())) {
        case 0:   // hinzufügen
            w.raw(nbtText(m["title"].str())).f32(float(m["progress"].asDouble(1))).varint(int32_t(m["color"].asInt()))
                .varint(int32_t(m["style"].asInt())).u8(uint8_t(m["flags"].asInt()));
            break;
        case 2: w.f32(float(m["progress"].asDouble(1))); break;
        case 3: w.raw(nbtText(m["title"].str())); break;
        case 4: w.varint(int32_t(m["color"].asInt())).varint(int32_t(m["style"].asInt())); break;
        case 5: w.u8(uint8_t(m["flags"].asInt())); break;
        default: break;   // 1 = entfernen
    }
    p.send("boss_bar", w.out);
}

void sendObjective(Player& p, const json::Value& m) {
    Writer w;
    int mode = int(m["mode"].asInt());
    w.string(m["name"].str()).i8(int8_t(mode));
    if (mode == 0 || mode == 2) w.raw(nbtText(m["title"].str())).varint(m["hearts"].asBool() ? 1 : 0).boolean(false);
    p.send("scoreboard_objective", w.out);
}

void sendDisplay(Player& p, const json::Value& m) {
    Writer w;
    w.varint(int32_t(m["slot"].asInt(1))).string(m["name"].str());
    p.send("display_objective", w.out);
}

void sendScore(Player& p, const json::Value& m) {
    Writer w;
    w.string(m["entry"].str()).string(m["objective"].str()).varint(int32_t(m["value"].asInt()));
    if (m["display"].isString()) w.boolean(true).raw(nbtText(m["display"].asString()));
    else w.boolean(false);
    w.boolean(false);   // kein eigenes Zahlenformat
    p.send("update_score", w.out);
}

void sendResetScore(Player& p, const json::Value& m) {
    Writer w;
    w.string(m["entry"].str());
    if (m["objective"].isString()) w.boolean(true).string(m["objective"].asString());
    else w.boolean(false);
    p.send("reset_score", w.out);
}

int ruleIndex(const std::string& v, std::initializer_list<const char*> names) {
    int i = 0;
    for (const char* n : names) {
        if (v == n) return i;
        ++i;
    }
    return 0;
}

void sendTeam(Player& p, const json::Value& m) {
    Writer w;
    int mode = int(m["mode"].asInt());
    w.string(m["name"].str()).i8(int8_t(mode));
    if (mode == 0 || mode == 2) {
        w.raw(nbtText(m["display"].str(m["name"].str()))).i8(int8_t(m["flags"].asInt(0)));
        std::string tag = m["nametag"].str("always"), col = m["collision"].str("always");
        if (p.version->protocol >= V1_21_5) {   // ab 1.21.5 als Zahl
            w.varint(ruleIndex(tag, {"always", "never", "hideForOtherTeams", "hideForOwnTeam"}));
            w.varint(ruleIndex(col, {"always", "never", "pushOtherTeams", "pushOwnTeam"}));
        } else {
            w.string(tag).string(col);
        }
        w.varint(int32_t(m["color"].asInt(21))).raw(nbtText(m["prefix"].str())).raw(nbtText(m["suffix"].str()));
    }
    if (mode == 0 || mode == 3 || mode == 4) {
        const json::Value& e = m["entries"];
        w.varint(e.isArray() ? int32_t(e.size()) : 0);
        if (e.isArray())
            for (const auto& x : e.asArray()) w.string(x.str());
    }
    p.send("teams", w.out);
}

// Partikel; mit Farbe (dust) oder Block (block, falling_dust, block_marker, dust_pillar)
std::pair<std::string, Bytes> particlePacket(const Version& v, const json::Value& m) {
    std::string name = ns(m["name"].str());
    auto it = v.particles.find(name);
    if (it == v.particles.end()) return {"", {}};
    Writer w;
    w.boolean(m["force"].asBool());
    if (v.protocol >= V1_21_4) w.boolean(m["force"].asBool());   // immer zeigen
    w.f64(m["x"].asDouble()).f64(m["y"].asDouble()).f64(m["z"].asDouble());
    w.f32(float(m["dx"].asDouble())).f32(float(m["dy"].asDouble())).f32(float(m["dz"].asDouble()));
    w.f32(float(m["speed"].asDouble())).i32(int32_t(std::max<int64_t>(0, m["count"].asInt(1))));
    w.varint(it->second);
    std::string base = removePrefix(name, "minecraft:");
    if (base == "dust") {
        int rgb = int(m["color"].asInt(0xFF0000));
        if (v.protocol >= V1_21_2) w.i32(rgb);
        else w.f32(float((rgb >> 16) & 255) / 255.0f).f32(float((rgb >> 8) & 255) / 255.0f).f32(float(rgb & 255) / 255.0f);
        w.f32(float(m["size"].asDouble(1)));
    } else if (base == "block" || base == "falling_dust" || base == "block_marker" || base == "dust_pillar") {
        int state = data::blockState(m["block"].str("minecraft:stone"));
        w.varint(v.remap[size_t(std::max(0, state))]);
    } else if (base == "entity_effect" || base == "dust_color_transition" || base == "item" || base == "vibration" ||
               base == "shriek" || base == "sculk_charge" || base == "trail" || base == "tinted_leaves" || base == "dragon_breath" ||
               base == "flash" || base == "effect" || base == "instant_effect") {
        return {"", {}};   // braucht weitere Daten, die (noch) nicht unterstützt werden
    }
    return {"particle", w.out};
}

Player* target(Server& s, const json::Value& m) {
    return m["to"].isString() ? s.playerByUuid(Uuid::parse(m["to"].asString())) : nullptr;
}

void respawnFor(Server& s, Player& viewer, Player& other) {
    for (auto& [n, pl] : other.spawnPackets(*viewer.version)) viewer.send(n, pl);
    (void)s;
}

}  // namespace

json::Object entitySnap(Entity* e) {
    json::Object o;
    o["id"] = e->entityId;
    o["type"] = e->type;
    o["x"] = e->x;
    o["y"] = e->y;
    o["z"] = e->z;
    o["yaw"] = double(e->yaw);
    o["pitch"] = double(e->pitch);
    o["ground"] = e->onGround;
    o["fire"] = e->fireTicks;
    if (auto* m = dynamic_cast<Mob*>(e)) {
        o["health"] = double(m->health);
        o["max_health"] = double(m->maxHealth);
        o["dead"] = m->dead;
    }
    if (auto* it = dynamic_cast<ItemEntity*>(e)) o["item"] = json::Array{it->item, it->count};
    return o;
}

bool fire(Server& s, const std::string& name, Player* p, json::Object data) {
    if (!s.bridge().listens(name)) return false;
    if (p) data["player"] = s.bridge().snap(p);
    return s.bridge().event(name, std::move(data))["cancelled"].asBool();
}

bool inventoryClick(Player& p, int slot, int button, int mode) {
    Server& s = p.server;
    if (!s.bridge().listens("inventory_click")) return false;
    Window& w = *p.window;
    json::Object d;
    d["gui"] = dynamic_cast<GuiWindow*>(&w) ? json::Value(dynamic_cast<GuiWindow*>(&w)->gui) : json::Value(-1);
    d["type"] = windowType(w);
    d["title"] = w.title;
    d["size"] = std::max(0, w.size - 36);
    d["slot"] = slot;
    d["button"] = button;
    d["mode"] = mode;
    d["item"] = slot >= 0 && slot < w.size ? stackJson(w.get(slot)) : json::Value();
    d["cursor"] = stackJson(p.cursor);
    return fire(s, "inventory_click", &p, std::move(d));
}

void afterInventoryClick(Player& p) {
    auto* w = dynamic_cast<GuiWindow*>(p.window.get());
    if (!w) return;
    syncGui(p.server, w->gui);
    if (!p.server.bridge().running()) return;
    json::Object d;
    d["gui"] = w->gui;
    d["items"] = windowItems(*w);
    p.server.bridge().event("inventory_update", std::move(d));
}

bool inventoryOpen(Player& p, Window& w) {
    Server& s = p.server;
    if (!s.bridge().listens("inventory_open")) return false;
    json::Object d;
    d["gui"] = dynamic_cast<GuiWindow*>(&w) ? json::Value(dynamic_cast<GuiWindow*>(&w)->gui) : json::Value(-1);
    d["type"] = windowType(w);
    d["title"] = w.title;
    d["size"] = std::max(0, w.size - 36);
    d["items"] = windowItems(w);
    return fire(s, "inventory_open", &p, std::move(d));
}

void inventoryClose(Player& p, Window& w) {
    Server& s = p.server;
    if (!s.bridge().listens("inventory_close")) return;
    json::Object d;
    d["gui"] = dynamic_cast<GuiWindow*>(&w) ? json::Value(dynamic_cast<GuiWindow*>(&w)->gui) : json::Value(-1);
    d["type"] = windowType(w);
    d["title"] = w.title;
    d["size"] = std::max(0, w.size - 36);
    d["items"] = windowItems(w);
    fire(s, "inventory_close", &p, std::move(d));
}

bool handleCall(Server& s, const json::Value& m) {
    const std::string& op = m["op"].asString();
    Player* p = target(s, m);
    // ---- Entities ----
    if (op == "entity_tp" || op == "remove_entity" || op == "entity_velocity" || op == "entity_fire" || op == "entity_health" ||
        op == "entity_damage") {
        Entity* e = s.getEntity(int(m["id"].asInt()));
        if (!e) return true;
        if (op == "entity_tp") {
            e->x = m["x"].asDouble();
            e->y = m["y"].asDouble();
            e->z = m["z"].asDouble();
        } else if (op == "remove_entity") {
            e->remove();
        } else if (op == "entity_velocity") {
            e->vx = m["x"].asDouble();
            e->vy = m["y"].asDouble();
            e->vz = m["z"].asDouble();
        } else if (op == "entity_fire") {
            e->fireTicks = int(m["ticks"].asInt());
        } else if (auto* mob = dynamic_cast<Mob*>(e)) {
            if (op == "entity_health") {
                mob->health = float(std::max(0.0, m["value"].asDouble()));
                if (mob->health <= 0) mob->die(0);
            } else {
                int attacker = 0;
                if (m["source"].isString())
                    if (Player* src = s.playerByUuid(Uuid::parse(m["source"].asString()))) attacker = src->entityId;
                mob->damage(float(m["amount"].asDouble()), attacker, "generic");
            }
        }
        return true;
    }
    if (op == "particle") {
        double x = m["x"].asDouble(), y = m["y"].asDouble(), z = m["z"].asDouble();
        double range = m["force"].asBool() ? 512 : 32;
        auto build = [&](const Version& v) { return particlePacket(v, m); };
        if (p) {
            auto [n, pl] = build(*p->version);
            if (!n.empty()) p->send(n, pl);
        } else {
            s.broadcastVersioned(build, nullptr, [&](Player* q) {
                return (q->x - x) * (q->x - x) + (q->y - y) * (q->y - y) + (q->z - z) * (q->z - z) < range * range;
            });
        }
        return true;
    }
    if (op == "gui_set") {
        auto it = g_guis.find(int(m["id"].asInt()));
        if (it == g_guis.end()) return true;
        int slot = int(m["slot"].asInt());
        if (slot >= 0 && slot < int(it->second->size())) (*it->second)[size_t(slot)] = stackOf(json::Array{m["item"], m["count"]});
        syncGui(s, it->first);
        return true;
    }
    if (op == "gui_contents") {
        auto it = g_guis.find(int(m["id"].asInt()));
        if (it == g_guis.end()) return true;
        const json::Value& items = m["items"];
        for (size_t i = 0; i < it->second->size(); ++i) (*it->second)[i] = items.isArray() && i < items.size() ? stackOf(items[i]) : Stack{};
        syncGui(s, it->first);
        return true;
    }
    if (op == "gui_forget") {
        g_guis.erase(int(m["id"].asInt()));
        return true;
    }
    if (!p) return false;
    // ---- Spieler ----
    if (op == "bossbar") sendBossBar(*p, m);
    else if (op == "sb_objective") sendObjective(*p, m);
    else if (op == "sb_display") sendDisplay(*p, m);
    else if (op == "sb_score") sendScore(*p, m);
    else if (op == "sb_reset") sendResetScore(*p, m);
    else if (op == "sb_team") sendTeam(*p, m);
    else if (op == "flight") {
        p->allowFlight = m["allow"].asBool(p->allowFlight);
        bool canFly = p->allowFlight || p->gamemode == 1 || p->gamemode == 3;
        p->flying = canFly && m["flying"].asBool(p->flying);
        p->refreshAbilities();
    } else if (op == "speed") {
        if (m["fly"].isNumber()) p->flySpeed = float(std::max(-1.0, std::min(1.0, m["fly"].asDouble())) * 0.5);   // Bukkit 0.1 = Vanilla 0.05
        if (m["walk"].isNumber()) p->walkSpeed = float(std::max(-1.0, std::min(1.0, m["walk"].asDouble())) * 0.5);   // Bukkit 0.2 = 0.1
        p->refreshAbilities();
    } else if (op == "hide_player" || op == "show_player") {
        Player* other = s.playerByUuid(Uuid::parse(m["target"].str()));
        if (!other || other == p) return true;
        if (op == "hide_player") {
            if (!p->hiddenPlayers.insert(other->uuid).second) return true;
            Writer rm;
            rm.varint(1).varint(other->entityId);
            p->send("remove_entities", rm.out);
            Writer info;
            info.varint(1).uuid(other->uuid);
            p->send("player_info_remove", info.out);
        } else {
            if (!p->hiddenPlayers.erase(other->uuid)) return true;
            Writer info;
            info.u8(0x1D).varint(1).raw(other->infoEntry());
            p->send("player_info_update", info.out);
            if (!other->dead) respawnFor(s, *p, *other);
        }
    } else if (op == "gui_open") {
        int id = int(m["id"].asInt());
        int size = std::max(1, std::min(54, int(m["size"].asInt(27))));
        auto& store = g_guis[id];
        if (!store || int(store->size()) != size) store = std::make_shared<std::vector<Stack>>(size_t(size));
        const json::Value& items = m["items"];
        for (int i = 0; i < size; ++i) (*store)[size_t(i)] = items.isArray() && size_t(i) < items.size() ? stackOf(items[size_t(i)]) : Stack{};
        int menu = int(m["menu"].asInt(-1));
        if (menu < 0) menu = std::max(0, std::min(5, size / 9 - 1));   // generic_9x1 .. generic_9x6
        p->openWindow(std::make_unique<GuiWindow>(*p, p->newWindowId(), id, store, menu, m["title"].str("Inventar")));
    } else if (op == "gui_close") {
        p->closeWindow(true);
    } else {
        return false;
    }
    return true;
}

bool handleRequest(Server& s, const json::Value& m, json::Value& out) {
    const std::string& op = m["op"].asString();
    if (op == "entities") {
        json::Array a;
        for (Entity* e : s.entities()) a.push_back(entitySnap(e));
        out = std::move(a);
        return true;
    }
    if (op == "entity") {
        Entity* e = s.getEntity(int(m["id"].asInt()));
        out = e ? json::Value(entitySnap(e)) : json::Value();
        return true;
    }
    if (op == "particles") {
        json::Array a;
        auto players = s.players();
        if (!players.empty())
            for (const auto& [n, i] : players.front()->version->particles) a.push_back(n);
        out = std::move(a);
        return true;
    }
    return false;
}

}  // namespace api
