// Paper-/Bukkit-Plugins (.jar): startet creemc-bridge.jar (Java) und verbindet es mit dem Server.
// Protokoll: eine JSON-Nachricht pro Zeile über stdin/stdout (wie in der Python-Version, siehe creemc/paper.py).
//   C++ -> Java: init, tick, event (Antwort: result), command/complete (Antwort: result), reply, stop
//   Java -> C++: ready, result, call (Auftrag), req (Frage, Antwort: reply), stopped
#pragma once
#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "json.hpp"

class Server;
class Player;

// Chat-Format umrechnen: CreeMC "{name}: {message}" <-> Bukkit "%1$s: %2$s"
std::string bridgeToBukkitFormat(const std::string& f);
std::string bridgeFromBukkitFormat(std::string f);

class JavaBridge {
public:
    explicit JavaBridge(Server& server);
    ~JavaBridge();

    // Startet Java, wenn .jar-Dateien in plugins/ liegen. false = keine Jars oder Fehler.
    bool start();
    void stop();
    bool running() const { return running_; }

    // Hauptthread
    void process();                                   // wartende Aufträge/Anfragen aus Java abarbeiten
    void tick();
    bool listens(const std::string& event) const { return running_ && events_.count(event) > 0; }
    // Schickt ein Ereignis und wartet auf das Ergebnis (null = keine Antwort / hört nicht zu)
    json::Value event(const std::string& name, json::Object data);
    json::Object snap(Player* p) const;

    std::vector<std::string> plugins;                 // "Name vX"

private:
    void send(const json::Value& msg);
    json::Value ask(json::Object msg, double timeout);
    void handle(const json::Value& msg);
    void handleCall(const json::Value& m);
    json::Value handleRequest(const json::Value& m);
    void registerCommand(const json::Value& c);
    void setListening(const json::Value& names);
    void readStdout();
    void readStderr();

    Server& server_;
    std::atomic<bool> running_{false};
    void* process_ = nullptr;   // HANDLE
    void* stdin_ = nullptr;
    void* stdout_ = nullptr;
    void* stderr_ = nullptr;
    std::mutex inMutex_;
    std::condition_variable inCv_;
    std::deque<json::Value> inbox_;
    std::map<int64_t, json::Value> results_;
    int64_t ids_ = 0;
    std::set<std::string> events_;
    std::atomic<bool> stopped_{false};
    std::atomic<bool> exited_{false};
    int64_t tick_ = 0;
    int depth_ = 0;
};
