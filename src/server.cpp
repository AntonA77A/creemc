#include "server.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <csignal>
#include <iostream>
#include <thread>

#include "blocks.hpp"
#include "buffer.hpp"
#include "data.hpp"
#include "physics.hpp"
#include "placement.hpp"
#include "redstone.hpp"
#include "player.hpp"
#include "text.hpp"
#include "vanilla.hpp"
#include "version.hpp"

#ifdef _WIN32
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSESOCK closesocket
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#define CLOSESOCK ::close
#endif

static constexpr int TPS = 20;
static Server* g_server = nullptr;

static std::string base64(const std::string& in) {
    static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    size_t i = 0;
    for (; i + 2 < in.size(); i += 3) {
        uint32_t v = uint32_t(uint8_t(in[i])) << 16 | uint32_t(uint8_t(in[i + 1])) << 8 | uint8_t(in[i + 2]);
        out += t[v >> 18];
        out += t[(v >> 12) & 63];
        out += t[(v >> 6) & 63];
        out += t[v & 63];
    }
    if (i + 1 == in.size()) {
        uint32_t v = uint32_t(uint8_t(in[i])) << 16;
        out += t[v >> 18];
        out += t[(v >> 12) & 63];
        out += "==";
    } else if (i + 2 == in.size()) {
        uint32_t v = uint32_t(uint8_t(in[i])) << 16 | uint32_t(uint8_t(in[i + 1])) << 8;
        out += t[v >> 18];
        out += t[(v >> 12) & 63];
        out += t[(v >> 6) & 63];
        out += '=';
    }
    return out;
}

Server::Server(std::string root) : root_(std::move(root)), commands_(*this) {
    g_server = this;
    config_ = Config::load(joinPath(root_, "config.toml"));
    world_ = std::make_unique<World>(config_.worldName, joinPath(root_, config_.worldName), config_.generator, config_.seed);
    gameRules_.load(joinPath(root_, config_.worldName), config_);
    loadOps();
    std::string icon;
    if (readFile(joinPath(root_, "server-icon.png"), icon)) favicon = "data:image/png;base64," + base64(icon);
    if (config_.onlineMode) keys_ = std::make_unique<auth::KeyPair>();
    bridge_ = std::make_unique<JavaBridge>(*this);
    addBlockListener([this](int x, int y, int z) { placement::shapesListener(*this, x, y, z); });
}

Server::~Server() { g_server = nullptr; }

// ---- OPs --------------------------------------------------------------------

void Server::loadOps() {
    std::string txt;
    if (!readFile(joinPath(root_, "ops.json"), txt)) return;
    try {
        json::Value list = json::parse(txt);   // nicht direkt in der Schleife: das Temporärobjekt lebt sonst nicht lange genug
        for (const auto& n : list.asArray()) ops_.insert(toLower(n.asString()));
    } catch (const std::exception& e) {
        logx::warn(std::string("ops.json fehlerhaft: ") + e.what());
    }
}

void Server::saveOps() {
    json::Array a;
    for (const auto& n : ops_) a.push_back(n);
    writeFileAtomic(joinPath(root_, "ops.json"), json::write(json::Value(std::move(a)), true));
}

bool Server::isOp(const std::string& name) const { return ops_.count(toLower(name)) > 0; }

void Server::setOp(const std::string& name, bool value) {
    if (value) ops_.insert(toLower(name));
    else ops_.erase(toLower(name));
    saveOps();
    if (Player* p = getPlayer(name)) {
        sendCommands(p);
        p->sendPermissionLevel();
    }
}

// ---- Spieler ----------------------------------------------------------------

std::vector<Player*> Server::players() {
    std::vector<Player*> out;
    for (auto& [u, p] : players_) out.push_back(p.get());
    return out;
}

Player* Server::getPlayer(const std::string& name) {
    std::string n = toLower(name);
    for (auto& [u, p] : players_)
        if (toLower(p->name) == n) return p.get();
    return nullptr;
}

Player* Server::playerByUuid(const Uuid& uuid) {
    auto it = players_.find(uuid);
    return it == players_.end() ? nullptr : it->second.get();
}

Player* Server::playerById(int id) {
    for (auto& [u, p] : players_)
        if (p->entityId == id) return p.get();
    return nullptr;
}

// ---- Entities ---------------------------------------------------------------

Entity* Server::spawnEntity(std::unique_ptr<Entity> owned) {
    Entity* e = owned.get();
    entities_[e->entityId] = std::move(owned);
    for (Player* p : players())
        for (auto& [n, pl] : e->spawnPackets(*p->version)) p->send(n, pl);
    return e;
}

Entity* Server::getEntity(int id) {
    auto it = entities_.find(id);
    return it == entities_.end() || it->second->removed ? nullptr : it->second.get();
}

std::vector<Entity*> Server::entities() {
    std::vector<Entity*> out;
    out.reserve(entities_.size());
    for (auto& [id, e] : entities_)
        if (!e->removed) out.push_back(e.get());
    return out;
}

bool Server::entityPosition(int id, double& x, double& y, double& z) {
    if (Entity* e = getEntity(id)) {
        x = e->x;
        y = e->y;
        z = e->z;
        return true;
    }
    if (Player* p = playerById(id)) {
        x = p->x;
        y = p->y;
        z = p->z;
        return true;
    }
    return false;
}

ItemEntity* Server::dropItemVelocity(double x, double y, double z, const std::string& item, int count, int pickupDelay,
                                     double vx, double vy, double vz) {
    if (item.empty() || count <= 0) return nullptr;
    auto e = std::make_unique<ItemEntity>(*this, x, y, z, item, count, pickupDelay);
    e->vx = vx;
    e->vy = vy;
    e->vz = vz;
    return static_cast<ItemEntity*>(spawnEntity(std::move(e)));
}

ItemEntity* Server::dropItem(double x, double y, double z, const std::string& item, int count, int pickupDelay, bool scatter) {
    double spread = scatter ? 0.3 : 0.1;
    return dropItemVelocity(x, y, z, item, count, pickupDelay, randDouble(-spread, spread), 0.2, randDouble(-spread, spread));
}

Mob* Server::spawnMob(const std::string& type, double x, double y, double z, float health) {
    return static_cast<Mob*>(spawnEntity(std::make_unique<Mob>(*this, type, x, y, z, health)));
}

void Server::addPlayer(std::unique_ptr<Player> owned) {
    Player* player = owned.get();
    players_[player->uuid] = std::move(owned);
    player->join();
    std::vector<Player*> others;
    for (Player* p : players())
        if (p != player) others.push_back(p);
    Writer info;
    info.u8(0x1D).varint(int32_t(others.size() + 1));
    for (Player* p : others) info.raw(p->infoEntry());
    info.raw(player->infoEntry());
    player->send("player_info_update", info.out);
    Writer one;
    one.u8(0x1D).varint(1).raw(player->infoEntry());
    broadcastPacket("player_info_update", one.out, player);
    for (Player* p : others) {
        if (!p->tabName.empty()) {
            Writer t;
            t.u8(0x20).varint(1).uuid(p->uuid).boolean(true).raw(text::toNbt(p->tabName));
            player->send("player_info_update", t.out);
        }
        for (auto& [n, pl] : p->spawnPackets(*player->version)) player->send(n, pl);
        if (p->canSee(player))
            for (auto& [n, pl] : player->spawnPackets(*p->version)) p->send(n, pl);
    }
    player->send("entity_metadata", player->metadata(*player->version));
    for (Entity* e : entities())
        for (auto& [n, pl] : e->spawnPackets(*player->version)) player->send(n, pl);
    logx::info(player->name + " ist beigetreten (" + player->address() + ", Minecraft " + player->version->name + ") [" +
               std::to_string(players_.size()) + "/" + std::to_string(config_.maxPlayers) + "]");
    std::string msg = config_.joinMessage;
    size_t pos = msg.find("{name}");
    if (pos != std::string::npos) msg.replace(pos, 6, player->name);
    if (bridge_->listens("player_join")) {   // Java-Plugins dürfen die Nachricht ändern (null = keine)
        json::Object d;
        d["player"] = bridge_->snap(player);
        d["join_message"] = msg.empty() ? json::Value() : json::Value(msg);
        json::Value r = bridge_->event("player_join", std::move(d));
        if (r.isObject() && r.asObject().contains("join_message")) msg = r["join_message"].str();
    }
    if (!msg.empty()) broadcast(msg, false);
}

void Server::removePlayer(Player* player) {
    auto it = players_.find(player->uuid);
    if (it == players_.end() || it->second.get() != player) return;
    std::unique_ptr<Player> owned = std::move(it->second);
    players_.erase(it);
    owned->closeWindow();
    owned->saveData();
    if (owned->conn) owned->conn->player = nullptr;
    Writer rm;
    rm.varint(1).varint(owned->entityId);
    broadcastPacket("remove_entities", rm.out);
    Writer info;
    info.varint(1).uuid(owned->uuid);
    broadcastPacket("player_info_remove", info.out);
    logx::info(owned->name + " hat den Server verlassen");
    std::string msg = config_.quitMessage;
    size_t pos = msg.find("{name}");
    if (pos != std::string::npos) msg.replace(pos, 6, owned->name);
    if (bridge_->listens("player_quit")) {
        json::Object d;
        d["player"] = bridge_->snap(owned.get());
        d["quit_message"] = msg.empty() ? json::Value() : json::Value(msg);
        json::Value r = bridge_->event("player_quit", std::move(d));
        if (r.isObject() && r.asObject().contains("quit_message")) msg = r["quit_message"].str();
    }
    if (!msg.empty()) broadcast(msg, false);
}

// ---- Senden -----------------------------------------------------------------

void Server::broadcast(const std::string& message, bool logConsole) {
    Bytes payload = text::toNbt(message);
    payload += '\0';   // keine Actionbar
    broadcastPacket("system_chat", payload);
    if (logConsole) logx::info(text::stripColors(message));
}

void Server::broadcastVersioned(const std::function<std::pair<std::string, Bytes>(const Version&)>& build, Player* exclude,
                                const std::function<bool(Player*)>& only) {
    std::map<int, Bytes> frames;
    for (Player* p : players()) {
        if (p == exclude || (only && !only(p))) continue;
        const Version& v = *p->version;
        auto it = frames.find(v.protocol);
        if (it == frames.end()) {
            auto [name, payload] = build(v);
            it = frames.emplace(v.protocol, v.has(name) ? encodeFrame(v.id(name), payload) : Bytes()).first;
        }
        if (!it->second.empty()) p->conn->sendFrame(it->second);
    }
}

void Server::broadcastPacket(const std::string& name, const Bytes& payload, Player* exclude,
                             const std::function<bool(Player*)>& only) {
    broadcastVersioned([&](const Version&) { return std::make_pair(name, payload); }, exclude, only);
}

void Server::broadcastPacketV(const std::string& name, const std::function<Bytes(const Version&)>& payload, Player* exclude,
                              const std::function<bool(Player*)>& only) {
    broadcastVersioned([&](const Version& v) { return std::make_pair(name, payload(v)); }, exclude, only);
}

void Server::playSound(const std::string& name, double x, double y, double z, float volume, float pitch, int category) {
    Bytes payload = soundPacket(ns(name), x, y, z, volume, pitch, category);
    double r = 16 * volume + 16;
    broadcastPacket("sound_effect", payload, nullptr,
                    [&](Player* p) { return (p->x - x) * (p->x - x) + (p->z - z) * (p->z - z) < r * r; });
}

// ---- Blöcke -----------------------------------------------------------------

std::string Server::getBlock(int x, int y, int z) { return data::blockName(world_->getBlock(x, y, z)); }

void Server::setBlock(int x, int y, int z, const std::string& block) {
    int s = data::blockState(block);
    if (s >= 0) setBlockState(x, y, z, s);
}

void Server::setBlockState(int x, int y, int z, int state, bool update) {
    int old = world_->getBlock(x, y, z);
    if (old == state) return;
    if (update) prevStates.emplace(std::make_tuple(x, y, z), old);
    world_->setBlock(x, y, z, state);
    broadcastPacketV(
        "block_update",
        [&](const Version& v) {
            Writer w;
            w.position(x, y, z).varint(v.remap[size_t(state)]);
            return w.out;
        },
        nullptr, [&](Player* p) { return p->hasChunk(x >> 4, z >> 4); });
    if (update) notifyNeighbors(x, y, z);
}

void Server::notifyNeighbors(int x, int y, int z) {
    // Nachbar-Updates der Reihe nach (keine Rekursion, Obergrenze gegen Endlosschleifen)
    updateQueue_.emplace_back(x, y, z);
    if (updating_) return;
    updating_ = true;
    int done = 0;
    while (!updateQueue_.empty() && done < 4096) {
        auto [px, py, pz] = updateQueue_.front();
        updateQueue_.pop_front();
        ++done;
        for (auto& f : blockListeners_) {
            try {
                f(px, py, pz);
            } catch (const std::exception& e) {
                logx::error(std::string("Fehler in Block-Aktualisierung: ") + e.what());
            }
        }
    }
    updateQueue_.clear();
    updating_ = false;
    prevStates.clear();
}

bool Server::playerAtBlock(int x, int y, int z) {
    for (Player* p : players()) {
        if (p->gamemode == 3) continue;
        if (std::abs(p->x - (x + 0.5)) < 0.8 && std::abs(p->z - (z + 0.5)) < 0.8 && p->y - 1 < y && y < p->y + 1.8) return true;
    }
    return false;
}

const Bytes& Server::chunkFrame(int cx, int cz, const Version& v) {
    Chunk& c = world_->chunk(cx, cz);
    auto it = c.frames.find(v.protocol);
    if (it == c.frames.end()) it = c.frames.emplace(v.protocol, encodeFrame(v.id("chunk_data"), c.payload(v))).first;
    return it->second;
}

// ---- Zeit, Befehle ----------------------------------------------------------

void Server::sendTime(Player* p) {
    auto payload = [this](const Version& v) { return v.updateTime(world_->age, world_->time, gameRules_.b("doDaylightCycle")); };
    if (p) p->send("update_time", payload(*p->version));
    else broadcastPacketV("update_time", payload);
}

void Server::sendCommands(Player* p) {
    auto names = commands_.visibleNames(*p);
    Writer w;
    w.varint(int32_t(1 + names.size() * 2));
    w.u8(0x00).varint(int32_t(names.size()));
    for (size_t i = 0; i < names.size(); ++i) w.varint(int32_t(1 + 2 * i));
    for (size_t i = 0; i < names.size(); ++i) {
        w.u8(0x05).varint(1).varint(int32_t(2 + 2 * i)).string(names[i]);   // Literal, ausführbar
        w.u8(0x16).varint(0).string("args").varint(5).varint(2).string("minecraft:ask_server");   // greedy String
    }
    w.varint(0);
    p->send("declare_commands", w.out);
}

void Server::refreshCommands() {
    for (Player* p : players()) sendCommands(p);
}

void Server::dispatchCommand(CommandSender& sender, const std::string& line) {
    commands_.dispatch(sender, removePrefix(line, "/"));
}

void Server::save() {
    world_->save();
    for (Player* p : players()) p->saveData();
}

void Server::wake() {
    {
        std::lock_guard<std::mutex> lock(wakeMutex_);
        woken_ = true;
    }
    wakeCv_.notify_one();
}

// ---- Laufzeit ---------------------------------------------------------------

void Server::acceptLoop() {
    while (!stop_) {
        sockaddr_storage addr{};
        socklen_t len = sizeof addr;
        socket_t s = ::accept(listen_, reinterpret_cast<sockaddr*>(&addr), &len);
        if (s == socket_t(~0)) {
            if (stop_) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        int one = 1;
        setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&one), sizeof one);
        char host[INET6_ADDRSTRLEN] = "?";
        int port = 0;
        if (addr.ss_family == AF_INET) {
            auto* a = reinterpret_cast<sockaddr_in*>(&addr);
            inet_ntop(AF_INET, &a->sin_addr, host, sizeof host);
            port = ntohs(a->sin_port);
        } else if (addr.ss_family == AF_INET6) {
            auto* a = reinterpret_cast<sockaddr_in6*>(&addr);
            inet_ntop(AF_INET6, &a->sin6_addr, host, sizeof host);
            port = ntohs(a->sin6_port);
        }
        auto conn = std::make_shared<Connection>(*this, s, host, port);
        {
            std::lock_guard<std::mutex> lock(newConnMutex_);
            newConns_.push_back(conn);
        }
        conn->start();
        wake();
    }
}

void Server::consoleLoop() {
    std::string line;
    while (!stop_ && std::getline(std::cin, line)) {
        line = trim(line);
        if (line.empty()) continue;
        {
            std::lock_guard<std::mutex> lock(consoleMutex_);
            consoleLines_.push_back(line);
        }
        wake();
    }
}

void Server::doTick() {
    ++tick_;
    for (Player* p : players()) p->tick();
    while (!later_.empty() && later_.begin()->first <= tick_) {
        auto f = std::move(later_.begin()->second);
        later_.erase(later_.begin());
        try {
            f();
        } catch (const std::exception& ex) {
            logx::error(std::string("Fehler in geplanter Aufgabe: ") + ex.what());
        }
    }
    vanilla::tick(*this);
    blocks::tick(*this);
    physics::tick(*this);
    redstone::tick(*this);
    for (Entity* e : entities()) {
        try {
            e->tick();
            if (!e->removed && tick_ % 2 == 0) e->sync();
        } catch (const std::exception& ex) {
            logx::error("Fehler beim Tick von " + e->type + " - Entity wird entfernt: " + ex.what());
            e->remove();
        }
    }
    for (auto it = entities_.begin(); it != entities_.end();) {
        if (it->second->removed) it = entities_.erase(it);
        else ++it;
    }
    World& w = *world_;
    w.age++;
    if (gameRules_.b("doDaylightCycle")) w.time++;
    if (tick_ % 20 == 0) sendTime();
    if (tick_ % 200 == 0)
        for (Player* p : players()) p->tickKeepAlive();
    if (tick_ % 100 == 0 && !players_.empty()) {
        Writer pw;
        pw.u8(0x10).varint(int32_t(players_.size()));
        for (Player* p : players()) pw.uuid(p->uuid).varint(p->ping);
        broadcastPacket("player_info_update", pw.out);
    }
    for (Player* p : players()) p->sendChunks();
    bridge_->tick();
    if (tick_ % (TPS * 300) == 0) {
        save();
        std::vector<ChunkPos> keep;
        for (Player* p : players()) keep.insert(keep.end(), p->loadedChunks.begin(), p->loadedChunks.end());
        w.unloadUnused(keep);
    }
}

int Server::run() {
    double started = nowSeconds();
    logx::info("Starte CreeMC (C++) für Minecraft " + data::rangeText());
    if (config_.onlineMode) logx::info("Online-Modus: AN - nur Spieler mit echtem Minecraft-Konto kommen rein");
    else logx::warn("Online-Modus: AUS - jeder kann mit beliebigem Namen beitreten (online_mode = false)");
    int n = recipes_.loadVanilla();
    logx::info(std::to_string(n) + " Crafting-Rezepte geladen");
    vanilla::init(*this);
    redstone::init(*this);
    physics::init(*this);
    bridge_->start();   // Paper-/Bukkit-Plugins, falls .jar-Dateien in plugins/ liegen

    logx::info("Bereite Spawn vor...");
    double sx, sy, sz;
    world_->spawnPoint(sx, sy, sz);
    const Version* latest = Version::get(data::LATEST);
    for (int dx = -3; dx <= 3; ++dx)
        for (int dz = -3; dz <= 3; ++dz) chunkFrame((int(std::floor(sx)) >> 4) + dx, (int(std::floor(sz)) >> 4) + dz, *latest);

#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    std::string host = config_.host.empty() ? "0.0.0.0" : config_.host;
    if (getaddrinfo(host.c_str(), std::to_string(config_.port).c_str(), &hints, &res) != 0 || !res) {
        logx::error("Adresse " + host + " ungültig");
        return 1;
    }
    listen_ = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    int one = 1;
    setsockopt(listen_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&one), sizeof one);
    if (listen_ == socket_t(~0) || ::bind(listen_, res->ai_addr, int(res->ai_addrlen)) != 0 || ::listen(listen_, 64) != 0) {
        logx::error("Port " + std::to_string(config_.port) + " konnte nicht geöffnet werden (schon belegt?)");
        freeaddrinfo(res);
        return 1;
    }
    freeaddrinfo(res);
    std::thread(&Server::acceptLoop, this).detach();
    std::thread(&Server::consoleLoop, this).detach();
    std::signal(SIGINT, [](int) {
        if (g_server) g_server->requestStop();
    });
    logx::info("Fertig (" + fmtDouble(nowSeconds() - started) + "s)! Server läuft auf " + host + ":" +
               std::to_string(config_.port) + " - tippe 'help' für Befehle");

    using clock = std::chrono::steady_clock;
    auto interval = std::chrono::microseconds(1000000 / TPS);
    auto next = clock::now();
    auto last = next;
    while (!stop_) {
        // Neue Verbindungen übernehmen
        {
            std::lock_guard<std::mutex> lock(newConnMutex_);
            for (auto& c : newConns_) conns_.push_back(c);
            newConns_.clear();
        }
        // Pakete aller Verbindungen verarbeiten
        for (auto& c : conns_) {
            if (!c->closed()) c->process();
            if (c->closed() && c->player) removePlayer(c->player);
        }
        conns_.erase(std::remove_if(conns_.begin(), conns_.end(), [](const auto& c) { return c->finished(); }), conns_.end());
        // Konsole
        std::deque<std::string> lines;
        {
            std::lock_guard<std::mutex> lock(consoleMutex_);
            lines.swap(consoleLines_);
        }
        for (const auto& l : lines) dispatchCommand(console, l);
        bridge_->process();   // Aufträge der Java-Plugins (auch zwischen den Ticks)
        // Tick
        auto now = clock::now();
        if (now >= next) {
            try {
                doTick();
            } catch (const std::exception& e) {
                logx::error(std::string("Fehler im Server-Tick: ") + e.what());
            }
            double dt = std::chrono::duration<double>(now - last).count();
            tps_ = tps_ * 0.95 + std::min(double(TPS), 1.0 / std::max(dt, 1e-6)) * 0.05;
            last = now;
            next += interval;
            if (now - next > std::chrono::seconds(1)) next = now;   // stark im Rückstand -> nicht nachholen
        }
        std::unique_lock<std::mutex> lock(wakeMutex_);
        wakeCv_.wait_until(lock, next, [&] { return woken_ || stop_.load(); });
        woken_ = false;
    }

    logx::info("Server wird gestoppt...");
    for (Player* p : players()) p->kick(std::string(text::SECTION) + "cServer wird gestoppt");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    for (Player* p : players()) removePlayer(p);
    bridge_->stop();   // Java-Plugins: onDisable()
    CLOSESOCK(listen_);
    world_->save();
    logx::info("Welt gespeichert. Tschüss!");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}
