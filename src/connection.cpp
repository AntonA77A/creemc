#include "connection.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

#include "buffer.hpp"
#include "data.hpp"
#include "player.hpp"
#include "server.hpp"
#include "text.hpp"

#ifdef _WIN32
#include <ws2tcpip.h>
#define CLOSESOCK closesocket
#else
#include <sys/socket.h>
#include <unistd.h>
#define CLOSESOCK ::close
#endif

static constexpr int MAX_PACKET = 2 * 1024 * 1024;

Bytes encodeFrame(int pid, const Bytes& payload) {
    // Ohne Kompression: [Länge][Paket-ID][Daten]
    Bytes body = varintBytes(pid) + payload;
    return varintBytes(int32_t(body.size())) + body;
}

static Uuid offlineUuid(const std::string& name) {
    // Wie Vanilla im Offline-Modus: UUID v3 von "OfflinePlayer:<name>"
    Bytes h = auth::md5("OfflinePlayer:" + name);
    Uuid u;
    std::memcpy(u.b.data(), h.data(), 16);
    u.b[6] = (u.b[6] & 0x0F) | 0x30;
    u.b[8] = (u.b[8] & 0x3F) | 0x80;
    return u;
}

Connection::Connection(Server& server, socket_t sock, std::string addr, int p)
    : address(std::move(addr)), port(p), server_(server), sock_(sock) {}

Connection::~Connection() {
    if (sock_ != socket_t(~0)) CLOSESOCK(sock_);
}

void Connection::start() {
    auto self = shared_from_this();
    std::thread([self] { self->readerLoop(); }).detach();
    std::thread([self] { self->writerLoop(); }).detach();
}

// ---- Threads ----------------------------------------------------------------

void Connection::readerLoop() {
    Bytes buf;
    char chunk[16384];
    while (!closed_) {
        int n = ::recv(sock_, chunk, sizeof chunk, 0);
        if (n <= 0) break;
        {
            std::lock_guard<std::mutex> lock(decryptMutex_);
            if (decrypt_) decrypt_->apply(chunk, size_t(n));
        }
        buf.append(chunk, size_t(n));
        // Vollständige Pakete herausschneiden
        while (true) {
            if (state == State::Handshake && !buf.empty() && uint8_t(buf[0]) == 0xFE) {   // uralter Ping
                closed_ = true;
                break;
            }
            uint32_t len = 0;
            size_t i = 0;
            bool complete = false;
            for (; i < 5 && i < buf.size(); ++i) {
                len |= uint32_t(uint8_t(buf[i]) & 0x7F) << (7 * i);
                if (!(uint8_t(buf[i]) & 0x80)) {
                    complete = true;
                    ++i;
                    break;
                }
            }
            if (!complete) {
                if (buf.size() >= 5) closed_ = true;
                break;
            }
            if (len == 0 || len > uint32_t(MAX_PACKET)) {
                closed_ = true;
                break;
            }
            if (buf.size() < i + len) break;
            Bytes frame = buf.substr(i, len);
            buf.erase(0, i + len);
            try {
                Reader r(frame);
                int pid = r.varint();
                std::lock_guard<std::mutex> lock(inMutex_);
                inbox_.emplace_back(pid, r.rest());
            } catch (const std::exception&) {
                closed_ = true;
                break;
            }
            server_.wake();
        }
    }
    closed_ = true;
    readerDone_ = true;
    outCv_.notify_all();
    server_.wake();
}

void Connection::writerLoop() {
    while (true) {
        Bytes data;
        {
            std::unique_lock<std::mutex> lock(outMutex_);
            outCv_.wait(lock, [&] { return !outbox_.empty() || closed_; });
            if (outbox_.empty() && closed_) break;
            data.swap(outbox_);
        }
        size_t sent = 0;
        while (sent < data.size()) {
            int n = ::send(sock_, data.data() + sent, int(std::min<size_t>(data.size() - sent, 1 << 20)), 0);
            if (n <= 0) {
                closed_ = true;
                break;
            }
            sent += size_t(n);
        }
        if (closed_) {
            std::lock_guard<std::mutex> lock(outMutex_);
            if (outbox_.empty()) break;
        }
    }
#ifdef _WIN32
    ::shutdown(sock_, SD_BOTH);
#else
    ::shutdown(sock_, SHUT_RDWR);
#endif
    writerDone_ = true;
    server_.wake();
}

// ---- Senden -----------------------------------------------------------------

void Connection::sendFrame(const Bytes& frame) {
    if (closed_) return;
    std::lock_guard<std::mutex> lock(outMutex_);
    size_t start = outbox_.size();
    outbox_ += frame;
    if (encrypt_) encrypt_->apply(outbox_.data() + start, frame.size());
    outCv_.notify_one();
}

void Connection::sendRaw(int pid, const Bytes& payload) { sendFrame(encodeFrame(pid, payload)); }

void Connection::send(const std::string& name, const Bytes& payload) {
    if (!version || !version->has(name)) return;
    sendRaw(version->id(name), payload);
}

void Connection::kick(const std::string& reason) {
    try {
        if (state == State::Login || state == State::LoginEncrypt || state == State::AuthWait || state == State::LoginAck) {
            Writer w;
            w.string(text::toJson(reason));
            sendRaw(0x00, w.out);
        } else if (state == State::Config) {
            send("config.disconnect", text::toNbt(reason));
        } else if (state == State::Play) {
            send("disconnect", text::toNbt(reason));
        }
    } catch (const std::exception&) {
    }
    if (closeAt == 0) closeAt = nowSeconds() + 0.2;
}

void Connection::close() {
    if (closed_.exchange(true)) return;
    outCv_.notify_all();
#ifdef _WIN32
    ::shutdown(sock_, SD_BOTH);
#else
    ::shutdown(sock_, SHUT_RDWR);
#endif
}

// ---- Verarbeitung (Hauptthread) ---------------------------------------------

void Connection::process() {
    if (closeAt && nowSeconds() >= closeAt) {
        close();
        return;
    }
    if (state == State::AuthWait) pollAuth();
    for (int budget = 0; budget < 256; ++budget) {
        std::pair<int, Bytes> pk;
        {
            std::lock_guard<std::mutex> lock(inMutex_);
            if (inbox_.empty()) break;
            pk = std::move(inbox_.front());
            inbox_.pop_front();
        }
        if (closeAt) continue;   // nach einem Kick nichts mehr verarbeiten
        try {
            handle(pk.first, pk.second);
        } catch (const ProtocolError& e) {
            if (state == State::Play) continue;   // fehlerhaftes Spielpaket ignorieren
            close();
            return;
        } catch (const std::exception& e) {
            logx::error("Fehler in Verbindung " + address + ": " + e.what());
            if (state != State::Play) {
                close();
                return;
            }
        }
        if (state == State::AuthWait) break;
    }
}

void Connection::handle(int pid, const Bytes& payload) {
    Reader r(payload);
    switch (state) {
        case State::Handshake: handleHandshake(pid, r); break;
        case State::Status: handleStatus(pid, r); break;
        case State::Login:
        case State::LoginEncrypt:
        case State::LoginAck: handleLogin(pid, r); break;
        case State::AuthWait: break;
        case State::Config: handleConfig(pid, r); break;
        case State::Play:
            if (player) player->handlePacket(pid, payload);
            break;
    }
}

void Connection::handleHandshake(int pid, Reader& r) {
    if (pid != 0x00) {
        close();
        return;
    }
    clientProtocol = r.varint();
    r.string(255);   // Adresse
    r.u16();         // Port
    int next = r.varint();
    version = Version::get(clientProtocol);
    if (next == 1) state = State::Status;
    else if (next == 2 || next == 3) state = State::Login;
    else close();
}

void Connection::handleStatus(int pid, Reader& r) {
    if (pid == 0x00) {
        const Config& cfg = server_.config();
        std::string motd = cfg.motd;
        int maxPlayers = cfg.maxPlayers;
        if (server_.bridge().listens("server_list_ping")) {
            json::Object d;
            d["motd"] = motd;
            d["max_players"] = maxPlayers;
            d["online"] = server_.playerCount();
            d["address"] = address;
            json::Value r = server_.bridge().event("server_list_ping", std::move(d));
            if (r["motd"].isString()) motd = r["motd"].asString();
            if (r["max_players"].isNumber()) maxPlayers = int(r["max_players"].asInt());
        }
        json::Object status, ver, players;
        ver["name"] = "CreeMC " + data::rangeText();
        ver["protocol"] = version ? clientProtocol : data::LATEST;   // eigene Nummer -> kein "veraltet"
        players["max"] = maxPlayers;
        players["online"] = server_.playerCount();
        json::Array sample;
        int n = 0;
        for (Player* p : server_.players()) {
            if (n++ >= 12) break;
            json::Object e;
            e["name"] = p->name;
            e["id"] = p->uuid.str();
            sample.emplace_back(std::move(e));
        }
        players["sample"] = json::Value(std::move(sample));
        status["version"] = json::Value(std::move(ver));
        status["players"] = json::Value(std::move(players));
        status["description"] = text::component(motd);
        status["enforcesSecureChat"] = false;
        if (!server_.favicon.empty()) status["favicon"] = server_.favicon;
        Writer w;
        w.string(json::write(json::Value(std::move(status))));
        sendRaw(0x00, w.out);
    } else if (pid == 0x01) {
        sendRaw(0x01, r.read(8));
        if (closeAt == 0) closeAt = nowSeconds() + 0.5;
    }
}

void Connection::handleLogin(int pid, Reader& r) {
    if (state == State::Login) {
        if (pid != 0x00) {
            close();
            return;
        }
        std::string name = r.string(16);
        if (!version) {
            kick(std::string(text::SECTION) + "cDiese Minecraft-Version wird nicht unterstützt.\n" + text::SECTION +
                 "7Erlaubt: " + text::SECTION + "e" + data::rangeText());
            return;
        }
        bool valid = name.size() >= 3 && name.size() <= 16;
        for (char c : name)
            if (!std::isalnum((unsigned char)c) && c != '_') valid = false;
        if (!valid) {
            kick(std::string(text::SECTION) + "cUngültiger Name");
            return;
        }
        loginName_ = name;
        if (server_.config().onlineMode && server_.keys()) {
            verifyToken_ = auth::randomBytes(4);
            Writer w;
            w.string("").prefixed(server_.keys()->publicDer).prefixed(verifyToken_).boolean(true);
            send("login.encryption_request", w.out);
            state = State::LoginEncrypt;
        } else {
            finishLogin(name, offlineUuid(name));
        }
        return;
    }
    if (state == State::LoginEncrypt) {
        if (pid != version->id("login.c_encryption_response")) {
            close();
            return;
        }
        Bytes encSecret = r.read(size_t(r.varint()));
        Bytes encToken = r.read(size_t(r.varint()));
        Bytes secret, token;
        if (!server_.keys()->decrypt(encSecret, secret) || !server_.keys()->decrypt(encToken, token) ||
            token != verifyToken_ || secret.size() != 16) {
            close();
            return;
        }
        {
            std::lock_guard<std::mutex> lock(decryptMutex_);
            decrypt_ = std::make_unique<auth::Cipher>(secret, false);
        }
        {
            std::lock_guard<std::mutex> lock(outMutex_);
            encrypt_ = std::make_unique<auth::Cipher>(secret, true);
        }
        std::string hash = auth::serverHash("", secret, server_.keys()->publicDer);
        std::string name = loginName_;
        authFuture_ = std::async(std::launch::async, [name, hash] { return auth::hasJoined(name, hash); });
        state = State::AuthWait;
        return;
    }
    if (state == State::LoginAck && pid == version->id("login.ack")) startConfiguration();
}

void Connection::pollAuth() {
    if (!authFuture_ || authFuture_->wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    std::optional<auth::Profile> profile;
    try {
        profile = authFuture_->get();
    } catch (const std::exception& e) {
        authFuture_.reset();
        logx::warn(loginName_ + " (" + address + ") abgewiesen: Mojang-Server nicht erreichbar (" + e.what() + ")");
        kick(std::string(text::SECTION) + "cDie Mojang-Server sind gerade nicht erreichbar. Versuche es später erneut.");
        return;
    }
    authFuture_.reset();
    if (!profile) {
        logx::info(loginName_ + " (" + address + ") abgewiesen: nicht bei Mojang angemeldet (Offline-/Crack-Client?)");
        kick(std::string(text::SECTION) + "cAnmeldung fehlgeschlagen: Bitte Minecraft neu starten (Konto nicht verifiziert).");
        return;
    }
    logx::info(profile->name + " bei Mojang bestätigt (UUID " + profile->uuid.str() + ")");
    properties = profile->properties;
    finishLogin(profile->name, profile->uuid);
}

void Connection::finishLogin(const std::string& name, const Uuid& uuid) {
    if (server_.playerCount() >= server_.config().maxPlayers) {
        kick(std::string(text::SECTION) + "cDer Server ist voll!");
        return;
    }
    if (server_.bridge().listens("player_pre_login")) {   // z.B. Bann- oder Whitelist-Plugins
        json::Object d;
        d["name"] = name;
        d["uuid"] = uuid.str();
        d["address"] = address;
        json::Value r = server_.bridge().event("player_pre_login", std::move(d));
        if (r["cancelled"].asBool()) {
            std::string msg = r["kick_message"].asString();
            kick(msg.empty() ? std::string(text::SECTION) + "cDu darfst diesen Server nicht betreten." : msg);
            return;
        }
    }
    if (Player* old = server_.playerByUuid(uuid)) {
        old->kick(std::string(text::SECTION) + "eDu hast dich von einem anderen Ort eingeloggt.");
        server_.removePlayer(old);
    }
    loginName_ = name;
    send("login.success", version->loginSuccess(uuid, name, properties));
    state = State::LoginAck;
    // UUID für die Konfiguration merken
    verifyToken_.assign(reinterpret_cast<const char*>(uuid.b.data()), 16);
}

void Connection::startConfiguration() {
    state = State::Config;
    Writer brand;
    brand.string("minecraft:brand").string("CreeMC");
    send("config.custom_payload", brand.out);
    Writer flags;
    flags.varint(1).string("minecraft:vanilla");
    send("config.feature_flags", flags.out);
    send("config.known_packs", version->knownPacks());
}

void Connection::readSettings(Reader& r) {
    try {
        settings.locale = r.string(16);
        settings.viewDistance = r.i8();
        r.varint();    // Chat-Modus
        r.boolean();   // Chat-Farben
        settings.skinParts = r.u8();
        settings.mainHand = r.varint();
    } catch (const ProtocolError&) {
    }
}

void Connection::handleConfig(int pid, Reader& r) {
    if (pid == version->id("config.c_settings")) {
        readSettings(r);
    } else if (pid == version->id("config.c_known_packs")) {
        if (r.varint() == 0) {   // Client kennt die Vanilla-Daten unserer Version nicht (Mods/Snapshot)
            kick(std::string(text::SECTION) + "cDein Client passt nicht zu Minecraft " + text::SECTION + "e" + version->name +
                 text::SECTION + "c.");
            return;
        }
        for (const auto& payload : version->registryPackets()) send("config.registry_data", payload);
        send("config.tags", version->tagsPayload());
        send("config.finish");
    } else if (pid == version->id("config.c_finish")) {
        state = State::Play;
        Uuid uuid;
        std::memcpy(uuid.b.data(), verifyToken_.data(), 16);
        auto p = std::make_unique<Player>(server_, shared_from_this(), loginName_, uuid);
        player = p.get();
        server_.addPlayer(std::move(p));
    }
}
