// Eine Client-Verbindung: Lesen/Schreiben in eigenen Threads, Verarbeitung im Hauptthread.
#pragma once
#include <atomic>
#include <condition_variable>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "auth.hpp"
#include "util.hpp"
#include "version.hpp"

#ifdef _WIN32
#include <winsock2.h>
using socket_t = SOCKET;
#else
using socket_t = int;
#endif

class Player;
class Server;

struct ClientSettings {
    std::string locale = "en_us";
    int viewDistance = 8;
    int skinParts = 0x7F;
    int mainHand = 1;
};

Bytes encodeFrame(int pid, const Bytes& payload);

class Connection : public std::enable_shared_from_this<Connection> {
public:
    enum class State { Handshake, Status, Login, LoginEncrypt, AuthWait, LoginAck, Config, Play };

    Connection(Server& server, socket_t sock, std::string address, int port);
    ~Connection();
    void start();

    // Hauptthread
    void process();                                  // alle wartenden Pakete abarbeiten
    void send(const std::string& name, const Bytes& payload = Bytes());
    void sendRaw(int pid, const Bytes& payload = Bytes());
    void sendFrame(const Bytes& frame);
    void kick(const std::string& reason);
    void close();
    bool closed() const { return closed_; }
    bool finished() const { return closed_ && readerDone_ && writerDone_; }

    State state = State::Handshake;
    const Version* version = nullptr;
    int clientProtocol = 0;
    ClientSettings settings;
    std::vector<ProfileProperty> properties;
    std::string address;
    int port = 0;
    Player* player = nullptr;
    double closeAt = 0;   // geplantes Schließen nach einem Kick (Zeit in Sekunden)

    void readSettings(class Reader& r);

private:
    void readerLoop();
    void writerLoop();
    void handle(int pid, const Bytes& payload);
    void handleHandshake(int pid, Reader& r);
    void handleStatus(int pid, Reader& r);
    void handleLogin(int pid, Reader& r);
    void finishLogin(const std::string& name, const Uuid& uuid);
    void startConfiguration();
    void handleConfig(int pid, Reader& r);
    void pollAuth();

    Server& server_;
    socket_t sock_;
    std::atomic<bool> closed_{false};
    std::atomic<bool> readerDone_{false}, writerDone_{false};
    std::mutex inMutex_;
    std::deque<std::pair<int, Bytes>> inbox_;
    std::mutex outMutex_;
    std::condition_variable outCv_;
    Bytes outbox_;
    std::unique_ptr<auth::Cipher> encrypt_;
    std::unique_ptr<auth::Cipher> decrypt_;
    std::mutex decryptMutex_;
    std::string loginName_;
    Bytes verifyToken_;
    std::optional<std::future<std::optional<auth::Profile>>> authFuture_;
};
