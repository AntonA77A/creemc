#include "bridge.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <thread>

#include "api.hpp"
#include "blocks.hpp"
#include "buffer.hpp"
#include "connection.hpp"
#include "data.hpp"
#include "player.hpp"
#include "server.hpp"
#include "text.hpp"
#include "vanilla.hpp"
#include "version.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {

// ---- Plattform: Prozess mit drei Pipes --------------------------------------

#ifdef _WIN32
std::wstring widen(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0);
    std::wstring w(size_t(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), w.data(), n);
    return w;
}

std::wstring quoteArg(const std::string& a) { return L"\"" + widen(a) + L"\""; }
#endif

struct Proc {
    void* process = nullptr;
    void* in = nullptr;    // wir schreiben
    void* out = nullptr;   // wir lesen (stdout)
    void* err = nullptr;   // wir lesen (stderr)
};

bool spawn(const std::vector<std::string>& args, const std::string& cwd, Proc& p) {
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE inR, inW, outR, outW, errR, errW;
    if (!CreatePipe(&inR, &inW, &sa, 0) || !CreatePipe(&outR, &outW, &sa, 0) || !CreatePipe(&errR, &errW, &sa, 0)) return false;
    SetHandleInformation(inW, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(outR, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(errR, HANDLE_FLAG_INHERIT, 0);
    std::wstring cmd;
    for (const auto& a : args) cmd += (cmd.empty() ? L"" : L" ") + quoteArg(a);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = inR;
    si.hStdOutput = outW;
    si.hStdError = errW;
    PROCESS_INFORMATION pi{};
    std::wstring wcwd = widen(cwd);
    BOOL ok = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, wcwd.c_str(), &si, &pi);
    CloseHandle(inR);
    CloseHandle(outW);
    CloseHandle(errW);
    if (!ok) {
        CloseHandle(inW);
        CloseHandle(outR);
        CloseHandle(errR);
        return false;
    }
    CloseHandle(pi.hThread);
    p.process = pi.hProcess;
    p.in = inW;
    p.out = outR;
    p.err = errR;
    return true;
#else
    int inP[2], outP[2], errP[2];
    if (pipe(inP) || pipe(outP) || pipe(errP)) return false;
    pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0) {
        dup2(inP[0], 0);
        dup2(outP[1], 1);
        dup2(errP[1], 2);
        for (int fd : {inP[0], inP[1], outP[0], outP[1], errP[0], errP[1]}) close(fd);
        if (chdir(cwd.c_str()) != 0) _exit(126);
        std::vector<char*> argv;
        for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        _exit(127);
    }
    close(inP[0]);
    close(outP[1]);
    close(errP[1]);
    signal(SIGPIPE, SIG_IGN);
    p.process = reinterpret_cast<void*>(intptr_t(pid));
    p.in = reinterpret_cast<void*>(intptr_t(inP[1]));
    p.out = reinterpret_cast<void*>(intptr_t(outP[0]));
    p.err = reinterpret_cast<void*>(intptr_t(errP[0]));
    return true;
#endif
}

long readSome(void* h, char* buf, size_t n) {
#ifdef _WIN32
    DWORD got = 0;
    if (!ReadFile(h, buf, DWORD(n), &got, nullptr)) return -1;
    return long(got);
#else
    return long(::read(int(intptr_t(h)), buf, n));
#endif
}

bool writeAll(void* h, const std::string& s) {
#ifdef _WIN32
    size_t done = 0;
    while (done < s.size()) {
        DWORD n = 0;
        if (!WriteFile(h, s.data() + done, DWORD(s.size() - done), &n, nullptr)) return false;
        done += n;
    }
    return true;
#else
    size_t done = 0;
    while (done < s.size()) {
        ssize_t n = ::write(int(intptr_t(h)), s.data() + done, s.size() - done);
        if (n <= 0) return false;
        done += size_t(n);
    }
    return true;
#endif
}

void closeHandle(void*& h) {
    if (!h) return;
#ifdef _WIN32
    CloseHandle(h);
#else
    ::close(int(intptr_t(h)));
#endif
    h = nullptr;
}

bool processAlive(void* proc) {
#ifdef _WIN32
    return proc && WaitForSingleObject(proc, 0) == WAIT_TIMEOUT;
#else
    if (!proc) return false;
    int status;
    return waitpid(pid_t(intptr_t(proc)), &status, WNOHANG) == 0;
#endif
}

void killProcess(void*& proc) {
    if (!proc) return;
#ifdef _WIN32
    if (WaitForSingleObject(proc, 0) == WAIT_TIMEOUT) TerminateProcess(proc, 1);
    WaitForSingleObject(proc, 3000);
    CloseHandle(proc);
#else
    pid_t pid = pid_t(intptr_t(proc));
    if (processAlive(proc)) kill(pid, SIGKILL);
    int status;
    waitpid(pid, &status, 0);
#endif
    proc = nullptr;
}

// java -version ausführen -> Hauptversion (0 = unbekannt)
int javaVersion(const std::string& java, const std::string& cwd) {
    Proc p;
    if (!spawn({java, "-version"}, cwd, p)) return 0;
    closeHandle(p.in);
    std::string text;
    char buf[4096];
    long n;
    while ((n = readSome(p.err, buf, sizeof buf)) > 0) text.append(buf, size_t(n));
    while ((n = readSome(p.out, buf, sizeof buf)) > 0) text.append(buf, size_t(n));
    closeHandle(p.out);
    closeHandle(p.err);
    killProcess(p.process);
    size_t q = text.find("version \"");
    if (q == std::string::npos) return 0;
    int major = std::atoi(text.c_str() + q + 9);
    if (major == 1) {   // "1.8.0_..."
        size_t dot = text.find('.', q + 9);
        return dot == std::string::npos ? 1 : std::atoi(text.c_str() + dot + 1);
    }
    return major;
}

std::string findJava() {
    const char* home = std::getenv("JAVA_HOME");
#ifdef _WIN32
    const char* exe = "java.exe";
#else
    const char* exe = "java";
#endif
    if (home) {
        fs::path p = fs::path(home) / "bin" / exe;
        if (fs::exists(p)) return p.string();
    }
    const char* path = std::getenv("PATH");
    if (!path) return "";
#ifdef _WIN32
    char sep = ';';
#else
    char sep = ':';
#endif
    for (const auto& dir : split(path, sep, true)) {
        fs::path p = fs::path(dir) / exe;
        std::error_code ec;
        if (fs::exists(p, ec)) return p.string();
    }
    return "";
}

std::string stateString(int state) {
    std::string n = data::blockName(state);
    auto p = data::props(state);
    if (p.empty()) return n;
    std::string s = n + "[";
    bool first = true;
    for (const auto& [k, v] : p) {
        if (!first) s += ",";
        first = false;
        s += k + "=" + v;
    }
    return s + "]";
}

int parseState(const std::string& text) {
    size_t b = text.find('[');
    int state = data::blockState(text.substr(0, b));
    if (state < 0 || b == std::string::npos) return state;
    std::map<std::string, std::string> kv;
    std::string inner = text.substr(b + 1);
    if (!inner.empty() && inner.back() == ']') inner.pop_back();
    for (const auto& part : split(inner, ',', true)) {
        size_t eq = part.find('=');
        if (eq != std::string::npos) kv[part.substr(0, eq)] = part.substr(eq + 1);
    }
    return data::withProps(state, kv);
}

std::string S(const char* code) { return std::string(text::SECTION) + code; }

}  // namespace

std::string bridgeToBukkitFormat(const std::string& f) {
    std::string out;
    for (char c : f) {
        if (c == '%') out += "%%";
        else out += c;
    }
    for (auto [a, b] : {std::pair<const char*, const char*>{"{name}", "%1$s"}, {"{message}", "%2$s"}}) {
        size_t p;
        while ((p = out.find(a)) != std::string::npos) out.replace(p, std::strlen(a), b);
    }
    return out;
}

std::string bridgeFromBukkitFormat(std::string f) {
    auto rep = [&](const std::string& a, const std::string& b, bool once) {
        size_t p;
        while ((p = f.find(a)) != std::string::npos) {
            f.replace(p, a.size(), b);
            if (once) break;
        }
    };
    rep("%1$s", "{name}", false);
    rep("%2$s", "{message}", false);
    rep("%s", "{name}", true);
    rep("%s", "{message}", true);
    rep("%%", "%", false);
    return f;
}

// ---- JavaBridge -------------------------------------------------------------

JavaBridge::JavaBridge(Server& server) : server_(server) {}

JavaBridge::~JavaBridge() { stop(); }

bool JavaBridge::start() {
    std::string pluginDir = joinPath(server_.root(), "plugins");
    std::vector<std::string> jars;
    std::error_code ec;
    if (fs::is_directory(fs::path(pluginDir), ec))
        for (const auto& e : fs::directory_iterator(fs::path(pluginDir), ec))
            if (e.is_regular_file() && toLower(e.path().extension().string()) == ".jar") jars.push_back(e.path().filename().string());
    if (jars.empty()) return false;
    std::string list;
    for (const auto& j : jars) list += (list.empty() ? "" : ", ") + j;

    std::string java = findJava();
    if (java.empty()) {
        logx::error("Paper-Plugins gefunden (" + list + "), aber kein Java installiert. Installiere Java 21 oder neuer "
                    "(z.B. https://adoptium.net) und starte den Server neu.");
        return false;
    }
    int ver = javaVersion(java, server_.root());
    if (ver && ver < 21) {
        logx::error("Paper-Plugins brauchen Java 21 oder neuer - gefunden: Java " + std::to_string(ver) + " (" + java + ")");
        return false;
    }
    // Bridge-Jar aus der .exe entpacken (nur wenn geändert)
    std::string jar = joinPath(joinPath(server_.root(), ".creemc"), "creemc-bridge.jar");
    try {
        std::string blob = data::resource("bridge.jar");
        std::string old;
        if (!readFile(jar, old) || old != blob) writeFileAtomic(jar, blob);
    } catch (const std::exception&) {
        logx::error("creemc-bridge.jar fehlt in dieser Version - Paper-Plugins können nicht geladen werden");
        return false;
    }
    logx::info("Starte Paper-Bridge (Java " + (ver ? std::to_string(ver) : std::string("?")) + ") für " +
               std::to_string(jars.size()) + " Jar(s)...");
    Proc p;
    if (!spawn({java, "-Dfile.encoding=UTF-8", "-Dstdout.encoding=UTF-8", "-Dstderr.encoding=UTF-8", "-jar", jar, pluginDir},
               server_.root(), p)) {
        logx::error("Java konnte nicht gestartet werden: " + java);
        return false;
    }
    process_ = p.process;
    stdin_ = p.in;
    stdout_ = p.out;
    stderr_ = p.err;
    running_ = true;
    std::thread([this] { readStdout(); }).detach();
    std::thread([this] { readStderr(); }).detach();

    // Serverinfos und Spieler schicken
    const Config& cfg = server_.config();
    double sx, sy, sz;
    server_.world().spawnPoint(sx, sy, sz);
    json::Object info;
    info["version"] = "CreeMC 0.2.0";
    info["minecraft"] = Version::get(data::LATEST)->name;
    info["max_players"] = cfg.maxPlayers;
    info["port"] = cfg.port;
    info["host"] = cfg.host;
    info["motd"] = cfg.motd;
    info["online_mode"] = cfg.onlineMode;
    info["view_distance"] = cfg.viewDistance;
    info["gamemode"] = cfg.gamemode;
    info["world"] = cfg.worldName;
    info["spawn"] = json::Array{sx, sy, sz};
    info["min_y"] = MIN_Y;
    info["max_y"] = MIN_Y + HEIGHT - 64;
    json::Array players;
    for (Player* pl : server_.players()) players.emplace_back(snap(pl));
    json::Object init;
    init["t"] = "init";
    init["server"] = json::Value(std::move(info));
    init["players"] = json::Value(std::move(players));
    send(json::Value(std::move(init)));

    // Auf "ready" warten (Plugins laden dauert); dabei Aufträge der Plugins schon ausführen
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(120);
    json::Value ready;
    while (running_ && std::chrono::steady_clock::now() < deadline) {
        json::Value msg;
        {
            std::unique_lock<std::mutex> lock(inMutex_);
            inCv_.wait_for(lock, std::chrono::milliseconds(200), [&] { return !inbox_.empty() || exited_.load(); });
            if (inbox_.empty()) {
                if (exited_) break;
                continue;
            }
            msg = inbox_.front();
            inbox_.pop_front();
        }
        if (msg["t"].asString() == "ready") {
            ready = msg;
            break;
        }
        handle(msg);
    }
    if (ready.isNull()) {
        logx::error("Paper-Bridge: Java hat nicht rechtzeitig geantwortet");
        stop();
        return false;
    }
    for (const auto& pl : ready["plugins"].asArray()) plugins.push_back(pl.asString());
    for (const auto& c : ready["commands"].asArray()) registerCommand(c);
    setListening(ready["events"]);
    std::string names;
    for (const auto& pl : plugins) names += (names.empty() ? "" : ", ") + pl;
    logx::info("Paper-Bridge: " + std::to_string(plugins.size()) + " Java-Plugin(s): " + (names.empty() ? "-" : names));
    return true;
}

void JavaBridge::stop() {
    if (!process_) return;
    if (running_ && processAlive(process_)) {
        json::Object m;
        m["t"] = "stop";
        send(json::Value(std::move(m)));
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (!stopped_ && !exited_ && std::chrono::steady_clock::now() < deadline) {
            process();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    running_ = false;
    closeHandle(stdin_);
    killProcess(process_);
}

void JavaBridge::send(const json::Value& msg) {
    if (!stdin_ || !running_) return;
    if (!writeAll(stdin_, json::write(msg) + "\n")) running_ = false;
}

void JavaBridge::readStdout() {
    std::string buf;
    char chunk[65536];
    long n;
    while ((n = readSome(stdout_, chunk, sizeof chunk)) > 0) {
        buf.append(chunk, size_t(n));
        size_t nl;
        while ((nl = buf.find('\n')) != std::string::npos) {
            std::string line = buf.substr(0, nl);
            buf.erase(0, nl + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            try {
                json::Value v = json::parse(line);
                {
                    std::lock_guard<std::mutex> lock(inMutex_);
                    inbox_.push_back(std::move(v));
                }
                inCv_.notify_all();
                server_.wake();
            } catch (const std::exception&) {
                logx::info("[Java] " + line);
            }
        }
    }
    exited_ = true;
    running_ = false;
    inCv_.notify_all();
    server_.wake();
}

void JavaBridge::readStderr() {
    std::string buf;
    char chunk[16384];
    long n;
    while ((n = readSome(stderr_, chunk, sizeof chunk)) > 0) {
        buf.append(chunk, size_t(n));
        size_t nl;
        while ((nl = buf.find('\n')) != std::string::npos) {
            std::string line = buf.substr(0, nl);
            buf.erase(0, nl + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            auto parts = split(line, '\t');
            if (parts.size() >= 3 && (parts[0] == "INFO" || parts[0] == "WARNING" || parts[0] == "ERROR" || parts[0] == "DEBUG")) {
                std::string who = parts[1];
                std::string msg = line.substr(parts[0].size() + parts[1].size() + 2);
                std::string prefix = (who.empty() || who == "STDOUT" || who == "Server") ? "" : "[" + who + "] ";
                if (parts[0] == "ERROR") logx::error(prefix + msg);
                else if (parts[0] == "WARNING") logx::warn(prefix + msg);
                else if (parts[0] == "INFO") logx::info(prefix + msg);
            } else if (!line.empty()) {
                logx::warn("[Java] " + line);
            }
        }
    }
}

// Wartet auf die Antwort zu einer Nachricht; Aufträge/Anfragen aus Java werden dabei sofort ausgeführt
json::Value JavaBridge::ask(json::Object msg, double timeout) {
    if (!running_) return json::Value();
    int64_t id = ++ids_;
    msg["id"] = id;
    send(json::Value(std::move(msg)));
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(int64_t(timeout * 1000));
    ++depth_;
    while (true) {
        auto it = results_.find(id);
        if (it != results_.end()) {
            json::Value r = it->second;
            results_.erase(it);
            --depth_;
            return r;
        }
        if (!running_ || std::chrono::steady_clock::now() >= deadline) break;
        json::Value m;
        {
            std::unique_lock<std::mutex> lock(inMutex_);
            inCv_.wait_until(lock, deadline, [&] { return !inbox_.empty() || exited_.load(); });
            if (inbox_.empty()) continue;
            m = inbox_.front();
            inbox_.pop_front();
        }
        handle(m);
    }
    --depth_;
    if (running_) logx::warn("Paper-Bridge: keine Antwort von Java (Zeitüberschreitung)");
    return json::Value();
}

void JavaBridge::process() {
    for (int i = 0; i < 1000; ++i) {
        json::Value m;
        {
            std::lock_guard<std::mutex> lock(inMutex_);
            if (inbox_.empty()) return;
            m = inbox_.front();
            inbox_.pop_front();
        }
        handle(m);
    }
}

void JavaBridge::handle(const json::Value& msg) {
    const std::string& t = msg["t"].asString();
    try {
        if (t == "result") {
            results_[msg["id"].asInt()] = msg;
        } else if (t == "call") {
            handleCall(msg);
        } else if (t == "req") {
            json::Object r;
            r["t"] = "reply";
            r["id"] = msg["id"];
            r["value"] = handleRequest(msg);
            send(json::Value(std::move(r)));
        } else if (t == "stopped") {
            stopped_ = true;
        }
    } catch (const std::exception& e) {
        logx::error("Paper-Bridge: Fehler bei Nachricht '" + t + "': " + e.what());
    }
}

void JavaBridge::tick() {
    if (!running_) {
        if (process_ && exited_) {
            logx::warn("Paper-Bridge: Java-Prozess wurde beendet");
            killProcess(process_);
            closeHandle(stdin_);
        }
        return;
    }
    process();
    json::Object m;
    m["t"] = "tick";
    m["tick"] = ++tick_;
    m["time"] = server_.world().time;
    json::Array players;
    for (Player* p : server_.players()) players.emplace_back(snap(p));
    m["players"] = json::Value(std::move(players));
    send(json::Value(std::move(m)));
}

json::Value JavaBridge::event(const std::string& name, json::Object data) {
    if (!listens(name) || depth_ > 8) return json::Value();
    json::Object m;
    m["t"] = "event";
    m["name"] = name;
    m["data"] = json::Value(std::move(data));
    return ask(std::move(m), 5.0);
}

json::Object JavaBridge::snap(Player* p) const {
    int level;
    float progress;
    levelForXp(p->xpTotal, level, progress);
    auto held = p->inventory.find(36 + p->heldSlot);
    json::Object s;
    s["uuid"] = p->uuid.str();
    s["name"] = p->name;
    s["id"] = p->entityId;
    s["x"] = p->x;
    s["y"] = p->y;
    s["z"] = p->z;
    s["yaw"] = double(p->yaw);
    s["pitch"] = double(p->pitch);
    s["ground"] = p->onGround;
    s["health"] = double(p->health);
    s["max_health"] = double(p->maxHealth);
    s["food"] = p->food;
    s["sat"] = double(p->saturation);
    s["gm"] = p->gamemode;
    s["op"] = p->isOp();
    s["level"] = level;
    s["exp"] = double(progress);
    s["xpt"] = p->xpTotal;
    s["sneak"] = p->sneaking;
    s["sprint"] = p->sprinting;
    s["fly"] = p->flying;
    s["slot"] = p->heldSlot;
    s["hand"] = held != p->inventory.end() ? json::Value(held->second.first) : json::Value();
    s["hand_n"] = held != p->inventory.end() ? held->second.second : 0;
    s["ip"] = p->conn->address;
    s["port"] = p->conn->port;
    s["locale"] = p->conn->settings.locale;
    s["ping"] = p->ping;
    s["dead"] = p->dead;
    s["fire"] = p->fireTicks;
    s["air"] = p->air;
    s["display"] = p->tabName.empty() ? json::Value() : json::Value(p->tabName);
    return s;
}

void JavaBridge::registerCommand(const json::Value& c) {
    std::string name = toLower(c["name"].asString());
    if (name.empty()) return;
    std::string label = name;
    if (server_.commands().get(name)) {
        logx::warn("Paper-Bridge: /" + name + " gibt es schon - Java-Befehl ist als /java:" + name + " erreichbar");
        name = "java:" + name;
    }
    Command cmd;
    cmd.name = name;
    cmd.description = c["description"].asString();
    cmd.usage = c["usage"].asString();
    for (const auto& a : c["aliases"].asArray())
        if (!server_.commands().get(a.asString())) cmd.aliases.push_back(a.asString());
    cmd.func = [this, label](CommandSender& sender, const std::vector<std::string>& args) {
        json::Object m;
        m["t"] = "command";
        Player* p = sender.asPlayer();
        m["sender"] = p ? json::Value(p->uuid.str()) : json::Value();
        m["label"] = label;
        json::Array a;
        for (const auto& x : args) a.push_back(x);
        m["args"] = json::Value(std::move(a));
        ask(std::move(m), 30.0);
    };
    cmd.completer = [this, label](CommandSender& sender, const std::vector<std::string>& args) {
        json::Object m;
        m["t"] = "complete";
        Player* p = sender.asPlayer();
        m["sender"] = p ? json::Value(p->uuid.str()) : json::Value();
        m["label"] = label;
        json::Array a;
        for (const auto& x : args) a.push_back(x);
        m["args"] = json::Value(std::move(a));
        json::Value r = ask(std::move(m), 2.0);
        std::vector<std::string> out;
        for (const auto& s : r["list"].asArray()) out.push_back(s.asString());
        return out;
    };
    server_.commands().add(std::move(cmd));
    server_.refreshCommands();
}

void JavaBridge::setListening(const json::Value& names) {
    events_.clear();
    for (const auto& n : names.asArray()) events_.insert(n.asString());
}

// ---- Aufträge aus Java ------------------------------------------------------

void JavaBridge::handleCall(const json::Value& m) {
    if (api::handleCall(server_, m)) return;
    const std::string& op = m["op"].asString();
    Server& s = server_;
    Player* p = m["to"].isString() ? s.playerByUuid(Uuid::parse(m["to"].asString())) : nullptr;
    if (op == "msg") {
        if (m["to"].isNull()) s.console.sendMessage(m["text"].asString());
        else if (p) p->sendMessage(m["text"].asString());
    } else if (op == "broadcast") {
        s.broadcast(m["text"].asString());
    } else if (op == "commands") {
        for (const auto& c : m["list"].asArray())
            if (!s.commands().get(c["name"].asString())) registerCommand(c);
    } else if (op == "listen") {
        setListening(m["events"]);
    } else if (op == "command") {
        Player* sender = m["sender"].isString() ? s.playerByUuid(Uuid::parse(m["sender"].asString())) : nullptr;
        std::string line = removePrefix(m["line"].asString(), "/");
        if (sender) s.dispatchCommand(*sender, line);
        else s.dispatchCommand(s.console, line);
    } else if (op == "time") {
        s.world().time = m["value"].asInt();
        s.sendTime();
    } else if (op == "set_block") {
        int state = parseState(m["block"].asString());
        if (state >= 0) s.setBlockState(int(m["x"].asInt()), int(m["y"].asInt()), int(m["z"].asInt()), state);
    } else if (op == "break_block") {
        int x = int(m["x"].asInt()), y = int(m["y"].asInt()), z = int(m["z"].asInt());
        int state = s.world().getBlock(x, y, z);
        if (m["drops"].asBool() && state != 0) {
            blocks::dropContents(s, x, y, z);
            for (auto& [item, c] : vanilla::blockDrops(data::blockName(state), "", true)) s.dropItem(x + .5, y + .3, z + .5, item, c);
        }
        s.setBlockState(x, y, z, 0);
    } else if (op == "explode") {   // ---- eigene CreeMC-API ----
        vanilla::explode(s, m["x"].asDouble(), m["y"].asDouble(), m["z"].asDouble(), std::max(0.1, m["power"].asDouble(4)),
                         m["break"].asBool(true));
    } else if (op == "tnt") {
        vanilla::spawnTnt(s, m["x"].asDouble(), m["y"].asDouble(), m["z"].asDouble(), int(m["fuse"].asInt(80)));
    } else if (op == "drop_item") {
        std::string item = m["item"].asString();
        if (data::isItem(item)) s.dropItem(m["x"].asDouble(), m["y"].asDouble(), m["z"].asDouble(), ns(item), int(m["count"].asInt(1)));
    } else if (op == "sound") {
        if (p) {
            Bytes pk = soundPacket(m["name"].asString(), p->x, p->y, p->z, float(m["volume"].asDouble(1)), float(m["pitch"].asDouble(1)), 7);
            p->send("sound_effect", pk);
        } else {
            s.playSound(m["name"].asString(), m["x"].asDouble(), m["y"].asDouble(), m["z"].asDouble(),
                        float(m["volume"].asDouble(1)), float(m["pitch"].asDouble(1)));
        }
    } else if (op == "op") {
        s.setOp(m["name"].asString(), m["value"].asBool());
    } else if (op == "save") {
        s.save();
    } else if (op == "shutdown") {
        s.requestStop();
    } else if (!p) {
        return;   // alles Weitere betrifft einen Spieler
    } else if (op == "actionbar") {
        p->sendActionBar(m["text"].asString());
    } else if (op == "title") {
        p->sendTitle(m["title"].asString(), m["sub"].asString(), int(m["fade_in"].asInt(10)), int(m["stay"].asInt(70)),
                     int(m["fade_out"].asInt(20)));
    } else if (op == "tablist") {
        Bytes pk = text::toNbt(m["header"].asString()) + text::toNbt(m["footer"].asString());
        p->send("tab_list", pk);
    } else if (op == "list_name" || op == "display_name") {
        p->setTabName(m["name"].str());
    } else if (op == "teleport") {
        p->teleport(m["x"].asDouble(), m["y"].asDouble(), m["z"].asDouble(), float(m["yaw"].asDouble(p->yaw)),
                    float(m["pitch"].asDouble(p->pitch)));
    } else if (op == "gamemode") {
        p->setGamemode(int(m["mode"].asInt()));
    } else if (op == "health") {
        p->health = float(std::max(0.0, std::min(double(p->maxHealth), m["value"].asDouble())));
        p->sendHealth();
    } else if (op == "food") {
        p->food = std::max(0, std::min(20, int(m["value"].asInt())));
        p->saturation = float(std::max(0.0, std::min(double(p->food), m["sat"].asDouble(p->saturation))));
        p->sendHealth();
    } else if (op == "air") {
        p->air = int(m["value"].asInt());
    } else if (op == "level") {
        p->xpTotal = xpForLevel(std::max(0, int(m["value"].asInt())));
        p->sendExperience();
    } else if (op == "exp") {
        int level;
        float progress;
        levelForXp(p->xpTotal, level, progress);
        double f = std::max(0.0, std::min(1.0, m["value"].asDouble()));
        p->xpTotal = xpForLevel(level) + int(f * (xpForLevel(level + 1) - xpForLevel(level)));
        p->sendExperience();
    } else if (op == "total_xp") {
        p->xpTotal = std::max(0, int(m["value"].asInt()));
        p->sendExperience();
    } else if (op == "give_xp") {
        p->xpTotal = std::max(0, p->xpTotal + int(m["value"].asInt()));
        p->sendExperience();
    } else if (op == "kick") {
        std::string reason = m["reason"].asString();
        p->kick(reason.empty() ? S("cDu wurdest gekickt.") : reason);
    } else if (op == "chat") {
        std::string t = m["text"].asString();
        if (startsWith(t, "/")) s.dispatchCommand(*p, t.substr(1));
        else p->chatAs(t);
    } else if (op == "set_slot") {
        std::string item = m["item"].isString() ? data::baseItem(m["item"].asString()) == m["item"].asString()
                                                      ? m["item"].asString()
                                                      : m["item"].asString()
                                                : "";
        p->setSlot(int(m["slot"].asInt()), item, int(m["count"].asInt()));
    } else if (op == "clear") {
        p->inventory.clear();
        p->cursor = {};
        p->syncInventory();
    } else if (op == "held_slot") {
        p->heldSlot = std::max(0, std::min(8, int(m["slot"].asInt())));
        Writer w;
        w.u8(uint8_t(p->heldSlot));   // Byte (bis 1.21.1) und VarInt sind für 0-8 gleich
        p->send("held_item", w.out);
    } else if (op == "sync_inventory") {
        p->syncInventory();
    } else if (op == "close_window") {
        p->closeWindow(true);
    } else if (op == "velocity") {
        p->push(m["x"].asDouble(), m["y"].asDouble(), m["z"].asDouble());
    } else if (op == "damage") {
        if (p->gamemode == 0 || p->gamemode == 2) {
            p->health = float(std::max(0.0, double(p->health) - m["amount"].asDouble()));
            p->sendHealth();
        }
    } else if (op == "fire") {
        p->fireTicks = int(m["ticks"].asInt());
        s.broadcastPacketV("entity_metadata", [p](const Version& v) { return p->metadata(v); });
    } else if (op == "heal") {
        p->setHealth(p->maxHealth);
        p->setFood(20, 5.0f);
        p->fireTicks = 0;
        p->sendMetadata();
    } else if (op == "kill") {
        p->kill(m["message"].str());
    } else if (op == "damage_cause") {
        p->damage(float(m["amount"].asDouble()), m["cause"].str("generic"));
    } else if (op == "give_xp_sound") {
        p->giveXp(int(m["value"].asInt()));
    }
}

json::Value JavaBridge::handleRequest(const json::Value& m) {
    json::Value apiOut;
    if (api::handleRequest(server_, m, apiOut)) return apiOut;
    const std::string& op = m["op"].asString();
    Server& s = server_;
    if (op == "block") return stateString(s.world().getBlock(int(m["x"].asInt()), int(m["y"].asInt()), int(m["z"].asInt())));
    if (op == "highest") {
        int x = int(m["x"].asInt()), z = int(m["z"].asInt());
        for (int y = MIN_Y + HEIGHT - 1; y >= MIN_Y; --y)
            if (s.world().getBlock(x, y, z) != 0) return y;
        return MIN_Y;
    }
    if (op == "inventory" || op == "give" || op == "take") {
        Player* p = m["uuid"].isString() ? s.playerByUuid(Uuid::parse(m["uuid"].asString())) : nullptr;
        if (!p) return op == "inventory" ? json::Value(json::Object()) : json::Value(m["count"].asInt());
        if (op == "inventory") {
            json::Object inv;
            for (const auto& [slot, st] : p->inventory) inv[std::to_string(slot)] = json::Array{st.first, st.second};
            return json::Value(std::move(inv));
        }
        std::string item = m["item"].asString();
        int count = int(m["count"].asInt());
        if (op == "give") return p->addItem(item, count);
        int missing = count;
        for (auto it = p->inventory.begin(); it != p->inventory.end() && missing > 0;) {
            if (it->second.first == item) {
                int take = std::min(missing, it->second.second);
                missing -= take;
                it->second.second -= take;
                if (it->second.second <= 0) {
                    it = p->inventory.erase(it);
                    continue;
                }
            }
            ++it;
        }
        p->syncInventory();
        return missing;
    }
    // ---- eigene CreeMC-API ----
    if (op == "tick") return json::Value(s.tick());
    if (op == "gamerule") {
        std::string v = s.gameRules().get(m["rule"].str());
        return v.empty() ? json::Value() : json::Value(v);
    }
    if (op == "set_gamerule") {
        if (!s.gameRules().set(m["rule"].str(), m["value"].str())) return false;
        for (Player* p : s.players()) p->sendRuleFlags();
        s.sendTime();
        return true;
    }
    if (op == "gamerules") {
        json::Object o;
        for (const auto& [n, v] : s.gameRules().all()) o[n] = v;
        return json::Value(std::move(o));
    }
    if (op == "spawn_mob") {
        std::string type = toLower(removePrefix(m["type"].str(), "minecraft:"));
        if (!vanilla::isMobType(type)) return -1;
        size_t before = s.entities().size();
        vanilla::spawnMobs(s, type, m["x"].asDouble(), m["y"].asDouble(), m["z"].asDouble(), 1);
        auto all = s.entities();
        return all.size() > before ? json::Value(all.back()->entityId) : json::Value(-1);
    }
    if (op == "mob_types") {
        json::Array a;
        for (const auto& t : vanilla::mobTypes()) a.push_back(t);
        return json::Value(std::move(a));
    }
    if (op == "player_info") {
        Player* p = s.playerByUuid(Uuid::parse(m["uuid"].str()));
        if (!p) return json::Value();
        json::Object o;
        o["name"] = p->name;
        o["x"] = p->x;
        o["y"] = p->y;
        o["z"] = p->z;
        o["health"] = double(p->health);
        o["food"] = p->food;
        o["saturation"] = double(p->saturation);
        o["level"] = p->level();
        o["xp"] = p->xpTotal;
        o["gamemode"] = p->gamemode;
        o["dead"] = p->dead;
        o["ping"] = p->ping;
        o["version"] = p->version->name;
        return json::Value(std::move(o));
    }
    if (op == "is_op") return s.isOp(m["name"].asString());
    if (op == "played_before") return fileExists(joinPath(joinPath(s.root(), "playerdata"), m["uuid"].asString() + ".json"));
    if (op == "uuid_of") {
        Player* p = s.getPlayer(m["name"].asString());
        return p ? json::Value(p->uuid.str()) : json::Value();
    }
    return json::Value();
}
