#include "player.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "buffer.hpp"
#include "connection.hpp"
#include "data.hpp"
#include "placement.hpp"
#include "server.hpp"
#include "text.hpp"
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

void Player::closeWindow(bool notifyClient) {
    for (const auto& st : window->returnItems()) dropStack(st.first, st.second);
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
    // Item-Entities folgen mit dem Vanilla-Teil; bis dahin landet es wieder im Inventar
    if (item.empty() || count <= 0) return;
    int rest = addItem(item, count, false);
    (void)rest;
}

void Player::kick(const std::string& reason) {
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

void Player::sendPermissionLevel() {
    // Entity-Status 24-28 = OP-Stufe 0-4 (ab 2 gehen F3+F4, F3+N)
    Writer w;
    w.i32(entityId).i8(int8_t(24 + (isOp() ? 4 : 0)));
    send("entity_status", w.out);
}

void Player::sendAbilities() {
    int flags = ABILITY_FLAGS[gamemode & 3];
    if (flying && (flags & 0x04)) flags |= 0x02;
    Writer w;
    w.i8(int8_t(flags)).f32(0.05f).f32(0.1f);
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
    } else if (n == "c_player_abilities") {
        flying = (r.i8() & 0x02) && (gamemode == 1 || gamemode == 3);
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
        if (slot >= 0 && slot <= 8) {
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
        if (wid == window->id && !dead) window->click(slot, button, mode);
        syncInventory();
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
        interactEvent(hand, it == inventory.end() ? "" : it->second.first, 0, 0, 0, -1, false);
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
    if (same) return;
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
    x = nx;
    y = ny;
    z = nz;
    yaw = nyaw;
    pitch = npitch;
    broadcastPosition();
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
    if (status == 5) return;
    if (status == 6) {   // Hände tauschen (F)
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
    if (it == inventory.end() || gamemode == 2 || gamemode == 3 || face < 0 || face > 5 || dead) {
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
    if (gamemode == 0) setSlot(slot, held.first, held.second - 1);
    ack(seq);
}
