// Der CreeMC-Server: Spielerverwaltung, Tick-Schleife, Konsole, Befehle.
#pragma once
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "auth.hpp"
#include "bridge.hpp"
#include "commands.hpp"
#include "config.hpp"
#include "connection.hpp"
#include "entity.hpp"
#include "recipes.hpp"
#include "world.hpp"

class Player;

class Server {
public:
    explicit Server(std::string root);
    ~Server();
    int run();

    // Zugriff
    Config& config() { return config_; }
    World& world() { return *world_; }
    Commands& commands() { return commands_; }
    RecipeBook& recipes() { return recipes_; }
    JavaBridge& bridge() { return *bridge_; }
    const auth::KeyPair* keys() const { return keys_.get(); }
    const std::string& root() const { return root_; }
    int64_t tick() const { return tick_; }
    double tps() const { return tps_; }
    std::string favicon;

    // Spieler
    std::vector<Player*> players();
    Player* getPlayer(const std::string& name);
    Player* playerByUuid(const Uuid& uuid);
    int playerCount() const { return int(players_.size()); }
    void addPlayer(std::unique_ptr<Player> p);
    void removePlayer(Player* p);
    int nextEntityId() { return ++entityId_; }
    Player* playerById(int entityId);

    // Entities (Items, Mobs)
    Entity* spawnEntity(std::unique_ptr<Entity> e);
    Entity* getEntity(int entityId);
    std::vector<Entity*> entities();
    bool entityPosition(int entityId, double& x, double& y, double& z);   // Spieler oder Entity
    ItemEntity* dropItem(double x, double y, double z, const std::string& item, int count, int pickupDelay = 10,
                         bool scatter = false);
    ItemEntity* dropItemVelocity(double x, double y, double z, const std::string& item, int count, int pickupDelay,
                                 double vx, double vy, double vz);
    Mob* spawnMob(const std::string& type, double x, double y, double z, float health);
    bool isOp(const std::string& name) const;
    void setOp(const std::string& name, bool value);

    // Senden
    void broadcast(const std::string& message, bool logConsole = true);
    // build(version) -> (Paketname, Daten); einmal pro Client-Version kodiert
    void broadcastVersioned(const std::function<std::pair<std::string, Bytes>(const Version&)>& build,
                            Player* exclude = nullptr, const std::function<bool(Player*)>& only = nullptr);
    void broadcastPacket(const std::string& name, const Bytes& payload, Player* exclude = nullptr,
                         const std::function<bool(Player*)>& only = nullptr);
    void broadcastPacketV(const std::string& name, const std::function<Bytes(const Version&)>& payload,
                          Player* exclude = nullptr, const std::function<bool(Player*)>& only = nullptr);
    void playSound(const std::string& name, double x, double y, double z, float volume = 1, float pitch = 1, int category = 0);

    // Blöcke
    std::string getBlock(int x, int y, int z);
    void setBlock(int x, int y, int z, const std::string& block);
    void setBlockState(int x, int y, int z, int state, bool update = true);
    void addBlockListener(std::function<void(int, int, int)> f) { blockListeners_.push_back(std::move(f)); }
    std::map<std::tuple<int, int, int>, int> prevStates;   // Zustand vor der Änderung (nur während der Updates)
    bool playerAtBlock(int x, int y, int z);
    const Bytes& chunkFrame(int cx, int cz, const Version& v);

    // Zeit & Befehle
    void sendTime(Player* p = nullptr);
    void sendCommands(Player* p);
    void refreshCommands();
    void dispatchCommand(CommandSender& sender, const std::string& line);
    void requestStop() { stop_ = true; wake(); }
    void runLater(int ticks, std::function<void()> f) { later_.emplace(tick_ + std::max(1, ticks), std::move(f)); }
    void save();
    void wake();

    ConsoleSender console;

private:
    void acceptLoop();
    void consoleLoop();
    void doTick();
    void notifyNeighbors(int x, int y, int z);
    void loadOps();
    void saveOps();

    std::string root_;
    Config config_;
    std::unique_ptr<World> world_;
    std::unique_ptr<auth::KeyPair> keys_;
    Commands commands_;
    RecipeBook recipes_;
    std::unique_ptr<JavaBridge> bridge_;
    std::vector<std::shared_ptr<Connection>> conns_;
    std::mutex newConnMutex_;
    std::vector<std::shared_ptr<Connection>> newConns_;
    std::map<Uuid, std::unique_ptr<Player>> players_;
    std::map<int, std::unique_ptr<Entity>> entities_;
    std::multimap<int64_t, std::function<void()>> later_;
    std::set<std::string> ops_;
    int entityId_ = 0;
    int64_t tick_ = 0;
    double tps_ = 20;
    std::atomic<bool> stop_{false};
    socket_t listen_ = socket_t(~0);
    std::mutex wakeMutex_;
    std::condition_variable wakeCv_;
    bool woken_ = false;
    std::mutex consoleMutex_;
    std::deque<std::string> consoleLines_;
    std::vector<std::function<void(int, int, int)>> blockListeners_;
    std::deque<std::tuple<int, int, int>> updateQueue_;
    bool updating_ = false;
};
