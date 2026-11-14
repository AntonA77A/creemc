#include "player.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <map>

#include "api.hpp"
#include "blocks.hpp"
#include "buffer.hpp"
#include "connection.hpp"
#include "data.hpp"
#include "enchanting.hpp"
#include "physics.hpp"
#include "placement.hpp"
#include "server.hpp"
#include "text.hpp"
#include "vanilla.hpp"
#include "version.hpp"

namespace {
const int ABILITY_FLAGS[4] = {0x00, 0x0D, 0x00, 0x07};

bool replaceable(int state) {
    static std::vector<int> list = [] {
        std::vector<int> v;
        for (const char* n : {"air", "cave_air", "void_air", "water", "lava", "short_grass", "tall_grass", "fern",
                              "large_fern", "dead_bush", "snow", "seagrass", "vine"}) {
            int s = data::blockState(n);
            if (s >= 0) v.push_back(s);
        }
        return v;
    }();
    return std::find(list.begin(), list.end(), state) != list.end();
}

std::string sec(const char* code) { return std::string(text::SECTION) + code; }
}  // namespace

Player::Player(Server& s, std::shared_ptr<Connection> c, std::string n, Uuid u)
    : server(s), conn(std::move(c)), version(conn->version), name(std::move(n)), uuid(u) {
    entityId = server.nextEntityId();
    gamemode = server.config().gamemode;
    server.world().spawnPoint(x, y, z);
    window = std::make_unique<PlayerWindow>(*this);
    lastKeepAlive_ = nowSeconds();
    loadData();
}

Player::~Player() = default;

// ---- Eigenschaften ----------------------------------------------------------

bool Player::isOp() const { return server.isOp(name); }
int Player::viewDistance() const {
    int c = conn->settings.viewDistance > 0 ? conn->settings.viewDistance : 2;
    return std::max(2, std::min(server.config().viewDistance, c));
}
std::string Player::heldItem() const {
    auto it = inventory.find(36 + heldSlot);
    return it == inventory.end() ? "" : it->second.first;
}
std::string Player::address() const { return conn->address; }

// ---- API --------------------------------------------------------------------

void Player::send(const std::string& packet, const Bytes& payload) { conn->send(packet, payload); }

void Player::sendMessage(const std::string& msg) {
    Bytes p = text::toNbt(msg);
    p += '\0';
    send("system_chat", p);
}

void Player::sendActionBar(const std::string& msg) { send("action_bar", text::toNbt(msg)); }

void Player::sendTitle(const std::string& title, const std::string& subtitle, int fadeIn, int stay, int fadeOut) {
    Writer w;
    w.i32(fadeIn).i32(stay).i32(fadeOut);
    send("title_times", w.out);
    if (!subtitle.empty()) send("subtitle", text::toNbt(subtitle));
    send("title", text::toNbt(title));
}

void Player::teleport(double nx, double ny, double nz, std::optional<float> nyaw, std::optional<float> npitch) {
    x = nx;
    y = ny;
    z = nz;
    if (nyaw) yaw = *nyaw;
    if (npitch) pitch = *npitch;
    ++teleportId_;
    awaitingTeleport_ = teleportId_;
    send("sync_player_position", version->syncPlayerPosition(teleportId_, x, y, z, yaw, pitch));
    broadcastPosition();
    requestChunkUpdate();
}

bool Player::setGamemode(int mode) {
    if (mode < 0 || mode > 3) return false;
    if (server.bridge().listens("player_gamemode_change")) {
        json::Object d;
        d["player"] = server.bridge().snap(this);
        d["old"] = gamemode;
        d["new"] = mode;
        if (server.bridge().event("player_gamemode_change", std::move(d))["cancelled"].asBool()) return false;
    }
    gamemode = mode;
    Writer w;
    w.u8(3).f32(float(gamemode));
    send("game_event", w.out);
    sendAbilities();
    Writer info;
    info.u8(0x04).varint(1).uuid(uuid).varint(gamemode);
    server.broadcastPacket("player_info_update", info.out);
    return true;
}

int Player::addItem(const std::string& rawItem, int count, bool sync) {
    std::string item = ns(rawItem);
    int maxStack = data::stackSize(item);
    std::vector<int> order;
    for (int s = 36; s < 45; ++s) order.push_back(s);
    for (int s = 9; s < 36; ++s) order.push_back(s);
    for (bool wantEmpty : {false, true}) {
        for (int slot : order) {
            if (count <= 0) break;
            auto it = inventory.find(slot);
            if (wantEmpty && it == inventory.end()) {
                int move = std::min(count, maxStack);
                inventory[slot] = {item, move};
                count -= move;
            } else if (!wantEmpty && it != inventory.end() && it->second.first == item && it->second.second < maxStack) {
                int move = std::min(count, maxStack - it->second.second);
                it->second.second += move;
                count -= move;
            }
        }
    }
    if (sync) syncInventory();
    return count;
}

int Player::removeItem(const std::string& rawItem, int count) {
    std::string item = ns(rawItem);
    int removed = 0;
    for (auto it = inventory.begin(); it != inventory.end() && removed < count;) {
        if (it->second.first == item) {
            int take = std::min(it->second.second, count - removed);
            removed += take;
            it->second.second -= take;
            if (it->second.second <= 0) {
                it = inventory.erase(it);
                continue;
            }
        }
        ++it;
    }
    syncInventory();
    return removed;
}

void Player::store(int slot, const Stack& st) {
    if (!st.first.empty() && st.second > 0) inventory[slot] = st;
    else inventory.erase(slot);
}

void Player::setSlot(int slot, const std::string& item, int count) {
    store(slot, {item, count});
    syncInventory();
}

void Player::syncInventory() {
    window->updateResult();
    stateId_ = (stateId_ + 1) & 0x7FFF;
    send("window_items", version->windowItems(window->id, stateId_, window->allSlots(), cursor));
    broadcastEquipment();
}

void Player::openWindow(std::unique_ptr<Window> w) {
    if (api::inventoryOpen(*this, *w)) return;   // von einem Plugin verhindert
    closeWindow(false);
    window = std::move(w);
    Writer p;
    p.varint(window->id).varint(window->typeId).raw(text::toNbt(window->title));
    send("open_window", p.out);
    syncInventory();
}

void Player::sendWindowProperty(int prop, int value) {
    Writer w;
    w.varint(window->id).i16(int16_t(prop)).i16(int16_t(value));
    send("window_property", w.out);
}

void Player::closeWindow(bool notifyClient) {
    for (const auto& st : window->returnItems()) dropStack(st.first, st.second);
    if (window->id != 0) {
        blocks::onClose(*this, *window);
        api::inventoryClose(*this, *window);
    }
    if (window->id != 0) {
        if (notifyClient) {
            Writer w;
            w.varint(window->id);
            send("close_window", w.out);
        }
        window = std::make_unique<PlayerWindow>(*this);
    }
    syncInventory();
}

void Player::dropStack(const std::string& item, int count) {
    // Wurf in Blickrichtung (wie Vanilla)
    if (item.empty() || count <= 0) return;
    json::Object dd;
    dd["item"] = item;
    dd["count"] = count;
    if (api::fire(server, "player_drop_item", this, std::move(dd))) {   // abgebrochen: zurück ins Inventar
        addItem(item, count);
        return;
    }
    double ry = yaw * 3.14159265358979 / 180.0, rp = pitch * 3.14159265358979 / 180.0;
    double vx = -std::sin(ry) * std::cos(rp) * 0.3, vz = std::cos(ry) * std::cos(rp) * 0.3;
    double vy = -std::sin(rp) * 0.3 + 0.1;
    server.dropItemVelocity(x, y + 1.32, z, item, count, 40, vx, vy, vz);
}

void Player::kick(const std::string& rawReason) {
    std::string reason = rawReason;
    if (server.bridge().listens("player_kick")) {   // Plugins dürfen den Kick verhindern oder den Grund ändern
        json::Object d;
        d["player"] = server.bridge().snap(this);
        d["reason"] = reason;
        json::Value r = server.bridge().event("player_kick", std::move(d));
        if (r["cancelled"].asBool()) return;
        if (r["reason"].isString()) reason = r["reason"].asString();
    }
    logx::info(name + " wurde gekickt: " + text::stripColors(reason));
    conn->kick(reason);
}

void Player::resyncBlock(int bx, int by, int bz) {
    int state = server.world().getBlock(bx, by, bz);
    Writer w;
    w.position(bx, by, bz).varint(version->remap[size_t(state)]);
    send("block_update", w.out);
}

void Player::ack(int seq) {
    Writer w;
    w.varint(seq);
    send("ack_block_change", w.out);
}

// ---- Beitritt ---------------------------------------------------------------

void Player::join() {
    const Config& cfg = server.config();
    send("login", version->login(entityId, gamemode, cfg.maxPlayers, cfg.viewDistance, cfg.generator == "flat", cfg.onlineMode));
    Writer diff;
    diff.u8(uint8_t(cfg.difficulty)).boolean(false);
    send("difficulty", diff.out);
    sendAbilities();
    sendPermissionLevel();
    Writer held;
    held.varint(heldSlot);
    send("held_item", held.out);
    double sx, sy, sz;
    server.world().spawnPoint(sx, sy, sz);
    send("spawn_position", version->spawnPosition(int(sx), int(sy), int(sz)));
    server.sendCommands(this);
    Writer ev;
    ev.u8(13).f32(0);   // "Chunks laden" beenden
    send("game_event", ev.out);
    server.sendTime(this);
    Writer hp;
    hp.f32(health).varint(food).f32(saturation);
    send("update_health", hp.out);
    sendExperience();
    syncInventory();
    sendRuleFlags();
    teleport(x, y, z, yaw, pitch);   // startet auch das Chunk-Laden
}

int xpForLevel(int level) {
    // Gesamtpunkte für eine Stufe (Vanilla-Formel)
    if (level <= 16) return level * level + 6 * level;
    if (level <= 31) return int(2.5 * level * level - 40.5 * level + 360);
    return int(4.5 * level * level - 162.5 * level + 2220);
}

void levelForXp(int total, int& level, float& progress) {
    level = 0;
    while (xpForLevel(level + 1) <= total) ++level;
    progress = float(total - xpForLevel(level)) / float(xpForLevel(level + 1) - xpForLevel(level));
}

void Player::sendHealth() {
    Writer w;
    w.f32(health).varint(food).f32(saturation);
    send("update_health", w.out);
}

void Player::push(double vx, double vy, double vz) { send("entity_velocity", version->velocity(entityId, vx, vy, vz)); }

// ---- Überleben --------------------------------------------------------------

void Player::setHealth(float h) {
    health = std::max(0.0f, std::min(maxHealth, h));
    sendHealth();
}

void Player::setFood(int f, float sat) {
    food = std::max(0, std::min(20, f));
    if (sat >= 0) saturation = std::max(0.0f, std::min(float(food), sat));
    sendHealth();
}

void Player::heal(float amount) {
    if (!dead && health < maxHealth) setHealth(health + amount);
}

void Player::addExhaustion(float amount) {
    // 4.0 Erschöpfung = 1 Sättigung bzw. 1 Hungerpunkt
    if (!isSurvival() || !server.config().hunger) return;
    exhaustion += amount;
    while (exhaustion >= 4.0f) {
        exhaustion -= 4.0f;
        if (saturation > 0) {
            saturation = std::max(0.0f, saturation - 1);
        } else if (food > 0) {
            json::Object d;
            d["food"] = food - 1;
            if (api::fire(server, "food_level_change", this, std::move(d))) continue;   // von einem Plugin verhindert
            food = food - 1;
        }
        sendHealth();
    }
}

void Player::setAir(int a) {
    if (a == air) return;
    air = a;
    server.broadcastPacketV("entity_metadata", [this, a](const Version& v) { return v.airMetadata(entityId, a); });
}

int Player::level() const {
    int l;
    float p;
    levelForXp(xpTotal, l, p);
    return l;
}

void Player::giveXp(int points) {
    if (points <= 0) return;
    int before = level();
    xpTotal += points;
    sendExperience();
    if (level() > before && level() % 5 == 0) playSound("minecraft:entity.player.levelup", 0.75f, 1.0f);
    else playSound("minecraft:entity.experience_orb.pickup", 0.1f, float(0.55 + randDouble() * 0.4));
}

void Player::setLevel(int l) {
    xpTotal = xpForLevel(std::max(0, l));
    sendExperience();
}

void Player::takeLevels(int levels) {
    int l;
    float p;
    levelForXp(xpTotal, l, p);
    int n = std::max(0, l - levels);
    xpTotal = xpForLevel(n) + int(p * float(xpForLevel(n + 1) - xpForLevel(n)));
    sendExperience();
}

void Player::playSound(const std::string& n, float volume, float pitch, int category) {
    send("sound_effect", soundPacket(ns(n), x, y, z, volume, pitch, category));
}

void Player::sendMetadata() {
    server.broadcastPacketV("entity_metadata", [this](const Version& v) { return metadata(v); });
}

namespace {
std::string deathMessage(Server& server, const Player& p, const std::string& cause, int attacker) {
    if (attacker) {
        std::string who;
        if (Player* a = server.playerById(attacker)) who = a->name;
        else if (Entity* e = server.getEntity(attacker)) {
            who = e->type;
            for (auto& c : who)
                if (c == '_') c = ' ';
            if (!who.empty()) who[0] = char(std::toupper((unsigned char)who[0]));
        }
        if (!who.empty()) return p.name + " wurde von " + who + " getötet";
    }
    static const std::map<std::string, std::string> causes = {
        {"fall", " ist aus zu großer Höhe gefallen"}, {"lava", " wollte in Lava schwimmen"},
        {"fire", " ist verbrannt"},                    {"drown", " ist ertrunken"},
        {"starve", " ist verhungert"},                 {"void", " ist aus der Welt gefallen"},
        {"explosion", " wurde in die Luft gesprengt"},
    };
    auto it = causes.find(cause);
    return p.name + (it == causes.end() ? std::string(" ist gestorben") : it->second);
}
}  // namespace

bool Player::damage(float amount, const std::string& cause, int attacker) {
    if (dead || !isSurvival() || amount <= 0) return false;
    const GameRules& rules = server.gameRules();
    if ((cause == "fall" && !rules.b("fallDamage")) || ((cause == "fire" || cause == "lava") && !rules.b("fireDamage")) ||
        (cause == "drown" && !rules.b("drowningDamage")))
        return false;
    // Vanilla: während der kurzen Unverwundbarkeit zählt nur, was über den letzten Treffer hinausgeht
    if (invulnerableTicks > 0 && amount <= lastDamage) return false;
    amount = vanilla::reduceDamage(*this, amount, cause, breach);
    if (server.bridge().listens("player_damage")) {
        json::Object d;
        d["player"] = server.bridge().snap(this);
        d["amount"] = double(amount);
        d["cause"] = cause;
        if (attacker) {   // Angreifer (Spieler oder Mob) für EntityDamageByEntityEvent
            if (Player* a = server.playerById(attacker)) d["attacker"] = server.bridge().snap(a);
            else if (Entity* e = server.getEntity(attacker)) d["attacker"] = api::entitySnap(e);
        }
        json::Value r = server.bridge().event("player_damage", std::move(d));
        if (r["cancelled"].asBool()) return false;
        if (r["amount"].isNumber()) amount = float(r["amount"].asDouble());
    }
    if (amount <= 0) return false;
    float applied = invulnerableTicks > 0 ? amount - lastDamage : amount;
    if (invulnerableTicks <= 0) invulnerableTicks = 10;
    lastDamage = amount;
    if (attacker) lastAttacker = attacker;
    addExhaustion(0.1f);
    setHealth(health - applied);
    Writer hurt;
    hurt.varint(entityId).f32(yaw);
    server.broadcastPacket("hurt_animation", hurt.out);
    server.playSound("minecraft:entity.player.hurt", x, y, z, 1.0f, 1.0f, 7);
    double ax, ay, az;
    if (attacker && server.entityPosition(attacker, ax, ay, az)) {
        double dx = x - ax, dz = z - az;
        double d = std::hypot(dx, dz);
        if (d < 1e-6) d = 1;
        push(dx / d * 0.4, 0.36, dz / d * 0.4);
    }
    if (health <= 0) die(deathMessage(server, *this, cause, attacker));
    return true;
}

void Player::kill(const std::string& message) {
    if (dead) return;
    setHealth(0);
    die(message.empty() ? name + " ist gestorben" : message);
}

void Player::die(const std::string& rawMessage) {
    dead = true;
    usingItem.reset();
    closeWindow();
    std::string message = rawMessage;
    bool keep = server.gameRules().b("keepInventory");
    std::vector<Stack> drops;
    for (const auto& [s, st] : inventory)
        if (s != 0) drops.push_back(st);
    if (server.bridge().listens("player_death")) {
        json::Object d;
        d["player"] = server.bridge().snap(this);
        d["message"] = message;
        d["keep_inventory"] = keep;
        json::Array da;
        for (const auto& st : drops) da.push_back(json::Array{st.first, st.second});
        d["drops"] = json::Value(std::move(da));
        json::Value r = server.bridge().event("player_death", std::move(d));
        if (r["message"].isString()) message = r["message"].asString();
        if (r["keep_inventory"].isBool()) keep = r["keep_inventory"].asBool();
        if (r["drops"].isArray()) {   // Plugins dürfen die Drops ändern
            drops.clear();
            for (const auto& v : r["drops"].asArray())
                if (v.isArray() && v.size() >= 2 && v[0].isString()) drops.push_back({v[0].asString(), int(v[1].asInt(1))});
        }
    }
    if (!keep) {
        inventory.clear();
        syncInventory();
        int dropXp = std::min(100, level() * 7);   // Vanilla: 7 Punkte pro Stufe, höchstens 100
        xpTotal = 0;
        sendExperience();
        for (const auto& st : drops) server.dropItem(x, y + 1, z, st.first, st.second, 40, true);
        (void)dropXp;
    }
    Writer w;
    w.varint(entityId).raw(text::toNbt(message));
    send("death_combat", w.out);
    Writer st;
    st.i32(entityId).i8(3);
    server.broadcastPacket("entity_status", st.out, this);
    if (!message.empty() && server.gameRules().b("showDeathMessages")) server.broadcast(sec("7") + message);
    if (server.gameRules().b("doImmediateRespawn")) {
        Server* s = &server;
        Uuid id = uuid;
        s->runLater(1, [s, id] {   // über die UUID suchen: der Spieler könnte inzwischen weg sein
            if (Player* p = s->playerByUuid(id)) p->respawn();
        });
    }
}

void Player::respawn() {
    if (!dead) return;
    double sx, sy, sz;
    server.world().spawnPoint(sx, sy, sz);
    if (server.bridge().listens("player_respawn")) {   // Plugins dürfen den Respawn-Ort ändern
        json::Object d;
        d["player"] = server.bridge().snap(this);
        d["location"] = json::Array{sx, sy, sz, double(yaw), double(pitch)};
        json::Value r = server.bridge().event("player_respawn", std::move(d));
        if (r["location"].isArray() && r["location"].size() >= 3) {
            sx = r["location"][0].asDouble(sx);
            sy = r["location"][1].asDouble(sy);
            sz = r["location"][2].asDouble(sz);
        }
    }
    dead = false;
    health = 20;
    food = 20;
    saturation = 5;
    exhaustion = 0;
    fallDistance = 0;
    fireTicks = 0;
    air = 300;
    invulnerableTicks = 0;
    lastDamage = 0;
    lastAttacker = 0;
    const Config& cfg = server.config();
    send("respawn", version->respawn(gamemode, cfg.generator == "flat"));
    Writer ev;
    ev.u8(13).f32(0);
    send("game_event", ev.out);
    sendAbilities();
    sendPermissionLevel();   // der Client vergisst sie beim Respawn
    sendRuleFlags();
    sendHealth();
    sendExperience();
    loadedChunks.clear();
    center.reset();
    chunkQueue_.clear();
    x = sx;
    y = sy;
    z = sz;
    Writer rm;
    rm.varint(1).varint(entityId);
    server.broadcastPacket("remove_entities", rm.out, this);
    for (Player* p : server.players()) {
        if (p == this) continue;
        if (!p->dead && canSee(p))
            for (auto& [n, pl] : p->spawnPackets(*version)) send(n, pl);
        if (p->canSee(this))
            for (auto& [n, pl] : spawnPackets(*p->version)) p->send(n, pl);
    }
    for (Entity* e : server.entities())
        for (auto& [n, pl] : e->spawnPackets(*version)) send(n, pl);
    send("entity_metadata", metadata(*version));
    syncInventory();
    teleport(x, y, z);
}

void Player::finishUsingItem() {
    usingItem.reset();
    Writer w;
    w.i32(entityId).i8(9);
    send("entity_status", w.out);
}

void Player::setHeldItem(const std::string& item, int count) { setSlot(36 + heldSlot, item, count); }

void Player::trackFall(double dy) {
    static int water = data::blockState("water");
    int feet = server.world().getBlock(int(std::floor(x)), int(std::floor(y)), int(std::floor(z)));
    if (flying || !isSurvival() || feet == water || dead) {
        fallDistance = 0;
        return;
    }
    if (!onGround && dy < 0) {
        fallDistance -= dy;
    } else if (onGround && fallDistance > 0) {
        double fall = fallDistance;
        fallDistance = 0;
        vanilla::onLand(*this, fall);
    }
}

void Player::interactEntity(int target, bool attack, int hand) {
    if (target == entityId || dead || gamemode == 3) return;
    double tx, ty, tz;
    if (!server.entityPosition(target, tx, ty, tz)) return;
    if ((tx - x) * (tx - x) + (ty - y) * (ty - y) + (tz - z) * (tz - z) > 36) return;
    if (attack) {
        addExhaustion(0.1f);
        vanilla::onAttack(*this, target);
    } else {
        json::Object d;
        if (Player* tp = server.playerById(target)) d["target"] = server.bridge().snap(tp);
        else if (Entity* e = server.getEntity(target)) d["target"] = api::entitySnap(e);
        d["hand"] = hand;
        if (hand == 0 && api::fire(server, "player_interact_entity", this, std::move(d))) return;
        vanilla::onInteractEntity(*this, target, hand);
    }
}

void Player::setTabName(const std::string& n) {
    tabName = n;
    Writer w;
    w.u8(0x20).varint(1).uuid(uuid);
    if (!n.empty()) w.boolean(true).raw(text::toNbt(n));
    else w.boolean(false);
    server.broadcastPacket("player_info_update", w.out);
}

void Player::sendExperience() {
    int level;
    float progress;
    levelForXp(xpTotal, level, progress);
    Writer w;
    w.f32(progress).varint(level).varint(xpTotal);
    send("set_experience", w.out);
}

void Player::sendRuleFlags() {
    // Spielregeln, die der Client selbst kennen muss
    const GameRules& rules = server.gameRules();
    Writer ev;
    ev.u8(11).f32(rules.b("doImmediateRespawn") ? 1.0f : 0.0f);   // Todesbildschirm an/aus
    send("game_event", ev.out);
    Writer lc;
    lc.u8(12).f32(rules.b("doLimitedCrafting") ? 1.0f : 0.0f);
    send("game_event", lc.out);
    Writer dbg;
    dbg.i32(entityId).i8(rules.b("reducedDebugInfo") ? 22 : 23);   // weniger F3-Infos
    send("entity_status", dbg.out);
}

void Player::sendPermissionLevel() {
    // Entity-Status 24-28 = OP-Stufe 0-4 (ab 2 gehen F3+F4, F3+N)
    Writer w;
    w.i32(entityId).i8(int8_t(24 + (isOp() ? 4 : 0)));
    send("entity_status", w.out);
}

void Player::sendAbilities() {
    int flags = ABILITY_FLAGS[gamemode & 3];
    if (allowFlight) flags |= 0x04;
    if (flying && (flags & 0x04)) flags |= 0x02;
    Writer w;
    w.i8(int8_t(flags)).f32(flySpeed).f32(walkSpeed);
    send("abilities", w.out);
}

// ---- Spielerdaten (gleiches Format wie die Python-Version) ------------------

std::string Player::dataFile() const { return joinPath(joinPath(server.root(), "playerdata"), uuid.str() + ".json"); }

void Player::loadData() {
    std::string txt;
    if (!readFile(dataFile(), txt)) return;
    json::Value d;
    try {
        d = json::parse(txt);
    } catch (...) {
        return;
    }
    if (d["pos"].isArray()) {
        x = d["pos"][0].asDouble(x);
        y = d["pos"][1].asDouble(y);
        z = d["pos"][2].asDouble(z);
    }
    if (d["rot"].isArray()) {
        yaw = float(d["rot"][0].asDouble());
        pitch = float(d["rot"][1].asDouble());
    }
    gamemode = int(d["gamemode"].asInt(gamemode));
    heldSlot = int(d["held_slot"].asInt(0));
    for (const auto& [slot, st] : d["inventory"].asObject()) {
        std::string item = ns(removePrefix(st[0].asString(), "minecraft:"));
        int s = std::atoi(slot.c_str());
        if (data::isItem(item) && s >= 1 && s <= 45) inventory[s] = {item, int(st[1].asInt(1))};
    }
    health = float(d["health"].asDouble(20));
    food = int(d["food"].asInt(20));
    saturation = float(d["saturation"].asDouble(5));
    xpTotal = int(d["xp"].asInt(0));
    pluginData = d["plugin_data"];
    if (health <= 0) {   // beim Tod ausgeloggt -> frisch wiederbeleben
        health = 20;
        food = 20;
        saturation = 5;
        server.world().spawnPoint(x, y, z);
    }
}

void Player::saveData() {
    if (!cursor.first.empty()) {   // Item am Mauszeiger nicht verlieren
        addItem(cursor.first, cursor.second, false);
        cursor = {};
    }
    json::Object d;
    d["name"] = name;
    d["pos"] = json::Array{x, y, z};
    d["rot"] = json::Array{double(yaw), double(pitch)};
    d["gamemode"] = gamemode;
    d["held_slot"] = heldSlot;
    json::Object inv;
    for (const auto& [s, st] : inventory) inv[std::to_string(s)] = json::Array{st.first, st.second};
    d["inventory"] = json::Value(std::move(inv));
    d["health"] = double(health);
    d["food"] = food;
    d["saturation"] = double(saturation);
    d["xp"] = xpTotal;
    d["plugin_data"] = pluginData.isNull() ? json::Value(json::Object()) : pluginData;
    if (!writeFileAtomic(dataFile(), json::write(json::Value(std::move(d)))))
        logx::error("Spielerdaten von " + name + " konnten nicht gespeichert werden");
}

// ---- Chunks -----------------------------------------------------------------

void Player::sendChunks() {
    if (chunkDirty_) {
        chunkDirty_ = false;
        int cx = int(std::floor(x)) >> 4, cz = int(std::floor(z)) >> 4;
        int vd = viewDistance();
        if (!center || center->x != cx || center->z != cz) {
            center = ChunkPos{cx, cz};
            Writer w;
            w.varint(cx).varint(cz);
            send("set_center_chunk", w.out);
        }
        std::set<ChunkPos> wanted;
        for (int dx = -vd; dx <= vd; ++dx)
            for (int dz = -vd; dz <= vd; ++dz) wanted.insert({cx + dx, cz + dz});
        for (auto it = loadedChunks.begin(); it != loadedChunks.end();) {
            if (!wanted.count(*it)) {
                Writer w;
                w.i32(it->z).i32(it->x);
                send("unload_chunk", w.out);
                it = loadedChunks.erase(it);
            } else {
                ++it;
            }
        }
        chunkQueue_.clear();
        for (const auto& c : wanted)
            if (!loadedChunks.count(c)) chunkQueue_.push_back(c);
        std::sort(chunkQueue_.begin(), chunkQueue_.end(), [&](const ChunkPos& a, const ChunkPos& b) {
            return (a.x - cx) * (a.x - cx) + (a.z - cz) * (a.z - cz) > (b.x - cx) * (b.x - cx) + (b.z - cz) * (b.z - cz);
        });   // nächste zuletzt -> von hinten abarbeiten
    }
    if (chunkQueue_.empty()) return;
    int batch = std::min<int>(16, int(chunkQueue_.size()));
    send("chunk_batch_start");
    for (int i = 0; i < batch; ++i) {
        ChunkPos c = chunkQueue_.back();
        chunkQueue_.pop_back();
        conn->sendFrame(server.chunkFrame(c.x, c.z, *version));
        loadedChunks.insert(c);
    }
    Writer w;
    w.varint(batch);
    send("chunk_batch_finished", w.out);
}

// ---- Sichtbarkeit -----------------------------------------------------------

std::vector<std::pair<std::string, Bytes>> Player::spawnPackets(const Version& v) const {
    std::vector<std::pair<std::string, Bytes>> out;
    out.emplace_back("spawn_entity", v.spawnPlayer(entityId, uuid, x, y, z, yaw, pitch));
    Writer head;
    head.varint(entityId).angle(yaw);
    out.emplace_back("head_rotation", head.out);
    out.emplace_back("entity_metadata", metadata(v));
    out.emplace_back("entity_equipment", equipment(v));
    return out;
}

Bytes Player::infoEntry() const {
    Writer w;
    w.uuid(uuid).string(name).raw(profileProperties(conn->properties)).varint(gamemode).boolean(true).varint(ping);
    return w.out;
}

Bytes Player::metadata(const Version& v) const { return v.playerMetadata(entityId, sneaking, conn->settings.skinParts, fireTicks > 0); }

Bytes Player::equipment(const Version& v) const {
    // Haupthand, Nebenhand, Füße, Beine, Brust, Kopf
    const int slots[6][2] = {{0, 36 + heldSlot}, {1, 45}, {2, 8}, {3, 7}, {4, 6}, {5, 5}};
    Writer w;
    w.varint(entityId);
    for (int i = 0; i < 6; ++i) {
        w.u8(uint8_t(slots[i][0] | (i < 5 ? 0x80 : 0)));
        auto it = inventory.find(slots[i][1]);
        w.raw(it == inventory.end() ? Bytes(1, '\0') : v.slot(it->second.first, it->second.second));
    }
    return w.out;
}

void Player::broadcastPosition() {
    server.broadcastVersioned([this](const Version& v) { return v.entityPosition(entityId, x, y, z, yaw, pitch, onGround); }, this);
    Writer head;
    head.varint(entityId).angle(yaw);
    server.broadcastPacket("head_rotation", head.out, this);
}

void Player::broadcastEquipment() {
    server.broadcastPacketV("entity_equipment", [this](const Version& v) { return equipment(v); }, this);
}

// ---- Tick -------------------------------------------------------------------

void Player::tick() {
    if (invulnerableTicks > 0 && --invulnerableTicks == 0) lastDamage = 0;
    if (fireTicks > 0 && --fireTicks == 0)
        server.broadcastPacketV("entity_metadata", [this](const Version& v) { return metadata(v); });
}

void Player::tickKeepAlive() {
    double now = nowSeconds();
    if (now - lastKeepAlive_ > 30) {
        kick(sec("cZeitüberschreitung"));
        return;
    }
    keepAliveId_ = int64_t(now * 1000);
    keepAliveSent_ = now;
    Writer w;
    w.i64(keepAliveId_);
    send("keep_alive", w.out);
}

// ---- Eingehende Pakete ------------------------------------------------------

void Player::handlePacket(int pid, const Bytes& payload) {
    auto it = version->idNames.find(pid);
    if (it == version->idNames.end()) return;
    const std::string& n = it->second;
    Reader r(payload);
    if (n == "c_confirm_teleport") {
        if (r.varint() == awaitingTeleport_.value_or(-1)) awaitingTeleport_.reset();
    } else if (n == "c_keep_alive") {
        if (r.i64() == keepAliveId_) {
            lastKeepAlive_ = nowSeconds();
            ping = int((lastKeepAlive_ - keepAliveSent_) * 1000);
        }
    } else if (n == "c_settings") {
        int oldVd = conn->settings.viewDistance;
        conn->readSettings(r);
        server.broadcastPacketV("entity_metadata", [this](const Version& v) { return metadata(v); });
        if (conn->settings.viewDistance != oldVd) requestChunkUpdate();
    } else if (n == "c_chat_message") {
        chat(trim(text::stripColors(r.string(256))));
    } else if (n == "c_chat_command" || n == "c_chat_command_signed") {
        onCommand(trim(r.string(32767)));
    } else if (n == "c_tab_complete") {
        int tid = r.varint();
        std::string line = removePrefix(r.string(32500), "/");
        auto matches = server.commands().complete(*this, line);
        size_t sp = line.rfind(' ');
        int start = sp == std::string::npos ? 1 : int(sp) + 2;
        int length = int(line.size()) - (start - 1);
        Writer w;
        w.varint(tid).varint(start).varint(length).varint(int32_t(matches.size()));
        for (const auto& m : matches) w.string(m).boolean(false);
        send("tab_complete", w.out);
    } else if (n == "c_position") {
        double nx = r.f64(), ny = r.f64(), nz = r.f64();
        move(nx, ny, nz, yaw, pitch, r.u8());
    } else if (n == "c_position_rotation") {
        double nx = r.f64(), ny = r.f64(), nz = r.f64();
        float ny2 = r.f32(), np = r.f32();
        move(nx, ny, nz, ny2, np, r.u8());
    } else if (n == "c_rotation") {
        float ny2 = r.f32(), np = r.f32();
        move(x, y, z, ny2, np, r.u8());
    } else if (n == "c_on_ground") {
        onGround = r.u8() & 1;
        trackFall(0);
    } else if (n == "c_client_command") {
        if (r.varint() == 0) respawn();
    } else if (n == "c_use_entity") {
        auto in = version->readInteract(r);
        interactEntity(in.target, in.attack, in.hand);
    } else if (n == "c_attack") {
        interactEntity(r.varint(), true, 0);
    } else if (n == "c_player_abilities") {
        bool want = r.i8() & 0x02;
        bool canFly = gamemode == 1 || gamemode == 3 || allowFlight;
        json::Object fd;
        fd["flying"] = want && canFly;
        if (canFly && want != flying && api::fire(server, "player_toggle_flight", this, std::move(fd))) sendAbilities();
        else flying = want && canFly;
    } else if (n == "c_entity_action") {
        r.varint();
        int action = r.varint();
        if (version->protocol < V1_21_6) {   // bis 1.21.5: 0/1 Schleichen, 3/4 Sprinten
            if (action == 0 || action == 1) setSneaking(action == 0);
            else if (action == 3 || action == 4) setSprinting(action == 3);
        } else if (action == 1 || action == 2) {
            setSprinting(action == 1);
        }
    } else if (n == "c_player_input") {
        if (version->protocol >= V1_21_6) setSneaking(r.u8() & 0x20);
    } else if (n == "c_held_item") {
        int slot = r.i16();
        json::Object hd;
        hd["previous"] = heldSlot;
        hd["new"] = slot;
        if (slot >= 0 && slot <= 8 && slot != heldSlot && api::fire(server, "player_item_held", this, std::move(hd))) {
            Writer back;   // abgebrochen: alten Slot zurück
            back.u8(uint8_t(heldSlot));
            send("held_item", back.out);
        } else if (slot >= 0 && slot <= 8) {
            heldSlot = slot;
            broadcastEquipment();
        }
    } else if (n == "c_creative_slot") {
        int slot = r.i16();
        int count = r.varint();
        std::string item = count > 0 ? version->itemName(r.varint()) : "";
        if (gamemode != 1) return;
        if (slot == -1 && !item.empty()) {
            dropStack(item, count);
            return;
        }
        if (slot < 1 || slot > 45) return;
        store(slot, {item, count});
        if (slot == 36 + heldSlot) broadcastEquipment();
    } else if (n == "c_window_click") {
        int wid = r.varint();
        r.varint();   // State-ID
        int slot = r.i16();
        int button = r.i8();
        int mode = r.varint();
        if (wid == window->id && !dead && api::inventoryClick(*this, slot, button, mode)) {
            syncInventory();   // von einem Plugin abgebrochen: echten Zustand zurückschicken
            return;
        }
        if (wid == window->id && !dead) window->click(slot, button, mode);
        syncInventory();
        api::afterInventoryClick(*this);
        if (auto* cw = dynamic_cast<blocks::ContainerWindow*>(window.get())) blocks::syncViewers(server, this, cw->positions);
    } else if (n == "c_enchant_item") {   // Knopf im Fenster (Zaubertisch: Option 0-2)
        int wid = r.varint();
        enchanting::button(*this, wid, r.varint());
    } else if (n == "c_name_item") {      // Amboss: neuer Name (bei jedem Tastendruck)
        enchanting::rename(*this, r.string(32767));
    } else if (n == "c_close_window") {
        if (r.varint() == window->id) closeWindow();
    } else if (n == "c_change_gamemode") {   // F3+F4 ab 1.21.6
        int mode = r.varint();
        if (isOp() && mode >= 0 && mode <= 3 && mode != gamemode && setGamemode(mode))
            logx::info(name + " wechselt per F3+F4 in Spielmodus " + std::to_string(mode));
    } else if (n == "c_swing" || n == "c_punch") {
        bool off = version->protocol < V26_3 && r.remaining() && r.varint() == 1;   // ab 26.3 ohne Felder
        server.broadcastVersioned([this, off](const Version& v) { return v.swing(entityId, off); }, this);
    } else if (n == "c_dig") {
        onDig(r);
    } else if (n == "c_place_block") {
        onPlace(r);
    } else if (n == "c_use_item") {
        int hand = r.varint();
        int seq = r.varint();
        auto it = inventory.find(hand == 0 ? 36 + heldSlot : 45);
        std::string item = it == inventory.end() ? "" : it->second.first;
        if (!item.empty() && !dead) usingItem = UsingItem{hand, item, server.tick()};
        if (!interactEvent(hand, item, 0, 0, 0, -1, false)) vanilla::onUseItem(*this, hand, item);
        ack(seq);
    }
}

void Player::chat(const std::string& rawMsg) {
    if (rawMsg.empty()) return;
    std::string msg = rawMsg;
    std::string line = server.config().chatFormat;
    if (server.bridge().listens("player_chat")) {   // Java-Plugins dürfen Nachricht und Format ändern
        json::Object d;
        d["player"] = server.bridge().snap(this);
        d["message"] = msg;
        d["format"] = bridgeToBukkitFormat(line);
        json::Value r = server.bridge().event("player_chat", std::move(d));
        if (r["cancelled"].asBool()) return;
        if (r["message"].isString()) msg = r["message"].asString();
        if (r["format"].isString() && !r["format"].asString().empty()) line = bridgeFromBukkitFormat(r["format"].asString());
    }
    size_t p = line.find("{name}");
    if (p != std::string::npos) line.replace(p, 6, name);
    p = line.find("{message}");
    if (p != std::string::npos) line.replace(p, 9, msg);
    logx::info("[Chat] " + text::stripColors(line));
    server.broadcast(line, false);
}

void Player::onCommand(const std::string& rawLine) {
    std::string line = rawLine;
    if (server.bridge().listens("player_command")) {
        json::Object d;
        d["player"] = server.bridge().snap(this);
        d["command"] = line;
        json::Value r = server.bridge().event("player_command", std::move(d));
        if (r["cancelled"].asBool()) return;
        if (r["command"].isString() && !r["command"].asString().empty()) line = r["command"].asString();
    }
    logx::info(name + " benutzt Befehl: /" + line);
    server.commands().dispatch(*this, line);
}

void Player::setSneaking(bool s) {
    if (s == sneaking) return;
    sneaking = s;
    server.broadcastPacketV("entity_metadata", [this](const Version& v) { return metadata(v); }, this);
    if (server.bridge().listens("player_toggle_sneak")) {
        json::Object d;
        d["player"] = server.bridge().snap(this);
        d["sneaking"] = s;
        server.bridge().event("player_toggle_sneak", std::move(d));
    }
}

void Player::setSprinting(bool s) {
    if (s == sprinting) return;
    sprinting = s;
    if (server.bridge().listens("player_toggle_sprint")) {
        json::Object d;
        d["player"] = server.bridge().snap(this);
        d["sprinting"] = s;
        server.bridge().event("player_toggle_sprint", std::move(d));
    }
}

bool Player::interactEvent(int hand, const std::string& item, int bx, int by, int bz, int face, bool hasBlock) {
    if (!server.bridge().listens("player_interact")) return false;
    json::Object d;
    d["player"] = server.bridge().snap(this);
    d["hand"] = hand;
    d["item"] = item.empty() ? json::Value() : json::Value(item);
    if (hasBlock) {
        d["block"] = data::blockName(server.world().getBlock(bx, by, bz));
        d["x"] = bx;
        d["y"] = by;
        d["z"] = bz;
        d["face"] = face;
    } else {
        d["block"] = json::Value();
    }
    return server.bridge().event("player_interact", std::move(d))["cancelled"].asBool();
}

void Player::move(double nx, double ny, double nz, float nyaw, float npitch, int flags) {
    if (awaitingTeleport_) return;
    if (!std::isfinite(nx) || !std::isfinite(ny) || !std::isfinite(nz) || !std::isfinite(nyaw) || !std::isfinite(npitch) ||
        std::abs(nx) > 3e7 || std::abs(nz) > 3e7) {
        kick(sec("cUngültige Bewegung"));
        return;
    }
    bool same = nx == x && ny == y && nz == z && nyaw == yaw && npitch == pitch;
    onGround = flags & 1;
    if (same) {
        trackFall(0);
        return;
    }
    if (server.bridge().listens("player_move")) {
        json::Object d;
        d["player"] = server.bridge().snap(this);
        d["from"] = json::Array{x, y, z, double(yaw), double(pitch)};
        d["to"] = json::Array{nx, ny, nz, double(nyaw), double(npitch)};
        json::Value r = server.bridge().event("player_move", std::move(d));
        if (r["cancelled"].asBool()) {
            teleport(x, y, z);
            return;
        }
        if (r["to"].isArray()) {
            const json::Value& t = r["to"];
            teleport(t[0].asDouble(), t[1].asDouble(), t[2].asDouble(), float(t[3].asDouble()), float(t[4].asDouble()));
            return;
        }
    }
    int oldCx = int(std::floor(x)) >> 4, oldCz = int(std::floor(z)) >> 4;
    double ox = x, oy = y, oz = z;
    x = nx;
    y = ny;
    z = nz;
    yaw = nyaw;
    pitch = npitch;
    broadcastPosition();
    vanilla::onMove(*this, ox, oy, oz);
    trackFall(y - oy);
    if ((int(std::floor(x)) >> 4) != oldCx || (int(std::floor(z)) >> 4) != oldCz) requestChunkUpdate();
}

void Player::onDig(Reader& r) {
    int status = r.varint();
    int bx, by, bz;
    r.position(bx, by, bz);
    r.i8();
    int seq = r.varint();
    if (status == 3 || status == 4) {   // Q / Strg+Q
        auto it = inventory.find(36 + heldSlot);
        if (it != inventory.end() && !dead) {
            Stack st = it->second;
            int n = status == 3 ? st.second : 1;
            setSlot(36 + heldSlot, st.first, st.second - n);
            dropStack(st.first, n);
        }
        return;
    }
    if (status == 5) {   // Benutzen losgelassen (z.B. Essen abgebrochen)
        usingItem.reset();
        return;
    }
    if (status == 6) {   // Hände tauschen (F)
        if (api::fire(server, "player_swap_hand_items", this)) {
            syncInventory();
            return;
        }
        Stack main = inventory.count(36 + heldSlot) ? inventory[36 + heldSlot] : Stack{};
        Stack off = inventory.count(45) ? inventory[45] : Stack{};
        store(36 + heldSlot, off);
        store(45, main);
        syncInventory();
        return;
    }
    if (dead) {
        ack(seq);
        return;
    }
    bool instant = (status == 0 && gamemode == 1) || (status == 2 && gamemode == 0);
    if (instant) {
        World& w = server.world();
        int state = w.getBlock(bx, by, bz);
        if (state != 0 && server.bridge().listens("block_break")) {
            json::Object d;
            d["player"] = server.bridge().snap(this);
            d["x"] = bx;
            d["y"] = by;
            d["z"] = bz;
            d["block"] = data::blockName(state);
            if (server.bridge().event("block_break", std::move(d))["cancelled"].asBool()) {
                resyncBlock(bx, by, bz);
                ack(seq);
                return;
            }
        }
        if (state != 0) {
            blocks::dropContents(server, bx, by, bz);   // Kisten-/Ofeninhalt fällt heraus
            if (gamemode == 0) vanilla::onBlockBreak(*this, bx, by, bz, state);
            server.setBlockState(bx, by, bz, 0);
            auto other = placement::otherPart(bx, by, bz, state);   // Tür, Bett, hohe Pflanze
            if (other) {
                auto [ox, oy, oz] = *other;
                if (data::blockName(w.getBlock(ox, oy, oz)) == data::blockName(state)) server.setBlockState(ox, oy, oz, 0);
            }
        }
    }
    ack(seq);
}

void Player::onPlace(Reader& r) {
    int hand = r.varint();
    int cx, cy, cz;
    r.position(cx, cy, cz);
    int face = r.varint();
    r.f32();
    float cursorY = r.f32();
    r.f32();
    r.boolean();   // im Block
    if (version->protocol >= V1_21_2) r.boolean();   // Weltgrenze
    int seq = r.varint();
    World& w = server.world();
    int slot = hand == 0 ? 36 + heldSlot : 45;
    auto it = inventory.find(slot);
    if (face >= 0 && face <= 5 &&
        interactEvent(hand, it == inventory.end() ? "" : it->second.first, cx, cy, cz, face, true)) {
        // abgebrochen: der Client hat evtl. schon vorhergesagt -> echten Zustand zurückschicken
        resyncBlock(cx, cy, cz);
        resyncBlock(cx + placement::FACE_OFFSETS[face][0], cy + placement::FACE_OFFSETS[face][1],
                    cz + placement::FACE_OFFSETS[face][2]);
        ack(seq);
        return;
    }
    if (face < 0 || face > 5 || dead) {
        ack(seq);
        return;
    }
    {
        // Rechtsklick auf Kiste, Ofen, Tür, Hebel ... (Schleichen mit Item in der Hand = bauen)
        std::string clickedName = data::blockName(w.getBlock(cx, cy, cz));
        std::string heldName = it == inventory.end() ? "" : it->second.first;
        bool useBlock = gamemode != 3 && (!sneaking || heldName.empty());
        if (useBlock && hand == 0 && blocks::openBlock(*this, clickedName, cx, cy, cz)) {
            ack(seq);
            return;
        }
        if (useBlock && hand == 1 && blocks::isInteractive(clickedName)) {
            ack(seq);
            return;
        }
        if (gamemode != 3 && !heldName.empty() &&
            vanilla::useItemOnBlock(*this, hand, heldName, cx, cy, cz, cx + placement::FACE_OFFSETS[face][0],
                                    cy + placement::FACE_OFFSETS[face][1], cz + placement::FACE_OFFSETS[face][2])) {
            resyncBlock(cx, cy, cz);
            ack(seq);
            return;
        }
    }
    if (it == inventory.end() || gamemode == 2 || gamemode == 3) {
        ack(seq);
        return;
    }
    Stack held = it->second;
    std::string blockName = placement::blockForItem(held.first);
    if (blockName.empty()) {
        ack(seq);
        return;
    }
    int clicked = w.getBlock(cx, cy, cz);
    int tx = cx, ty = cy, tz = cz;
    if (!replaceable(clicked)) {
        tx += placement::FACE_OFFSETS[face][0];
        ty += placement::FACE_OFFSETS[face][1];
        tz += placement::FACE_OFFSETS[face][2];
    }
    auto parts = placement::place(blockName, tx, ty, tz, face, yaw, pitch, cursorY);
    bool ok = true;
    for (const auto& p : parts)
        if (!replaceable(w.getBlock(p.x, p.y, p.z))) ok = false;
    if (ok)
        for (const auto& p : parts)
            if (data::SOLID[size_t(p.state)] && server.playerAtBlock(p.x, p.y, p.z)) ok = false;   // nicht in Spieler
    if (ok && blockName == "minecraft:redstone_wire" && !data::SOLID[size_t(w.getBlock(tx, ty - 1, tz))]) ok = false;
    if (ok && !physics::canPlace(server, blockName, tx, ty, tz)) ok = false;   // Saat nur auf Ackerboden
    if (ok && server.bridge().listens("block_place")) {
        json::Object d;
        d["player"] = server.bridge().snap(this);
        d["x"] = tx;
        d["y"] = ty;
        d["z"] = tz;
        d["block"] = blockName;
        d["against"] = json::Array{cx, cy, cz};
        d["item"] = held.first;
        if (server.bridge().event("block_place", std::move(d))["cancelled"].asBool()) ok = false;
    }
    if (!ok) {
        for (const auto& p : parts) resyncBlock(p.x, p.y, p.z);
        ack(seq);
        return;
    }
    for (const auto& p : parts) server.setBlockState(p.x, p.y, p.z, p.state);
    placement::connectChest(server, tx, ty, tz, sneaking);
    blocks::onPlaced(server, tx, ty, tz, blockName);
    if (gamemode == 0) setSlot(slot, held.first, held.second - 1);
    ack(seq);
}
