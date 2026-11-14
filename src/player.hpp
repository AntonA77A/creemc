// Ein verbundener Spieler: Paketverarbeitung im Play-Zustand.
#pragma once
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "commands.hpp"
#include "inventory.hpp"
#include "json.hpp"
#include "util.hpp"
#include "world.hpp"

class Connection;
class Server;
class Version;
class Reader;

using Stack = std::pair<std::string, int>;   // (Item, Anzahl); Item "" = leer

int xpForLevel(int level);
void levelForXp(int total, int& level, float& progress);

class Player : public CommandSender {
public:
    Player(Server& server, std::shared_ptr<Connection> conn, std::string name, Uuid uuid);
    ~Player() override;

    // CommandSender
    void sendMessage(const std::string& msg) override;
    std::string senderName() const override { return name; }
    bool senderIsOp() const override { return isOp(); }
    Player* asPlayer() override { return this; }

    Server& server;
    std::shared_ptr<Connection> conn;
    const Version* version;
    std::string name;
    Uuid uuid;
    int entityId;
    int gamemode;
    double x = 0, y = 0, z = 0;
    float yaw = 0, pitch = 0;
    bool onGround = false, flying = false, sneaking = false, sprinting = false;
    int heldSlot = 0;
    std::map<int, Stack> inventory;     // Slot -> Stack (1-45 wie Vanilla-Fenster 0)
    Stack cursor;
    float health = 20, maxHealth = 20, saturation = 5, exhaustion = 0;
    int food = 20, air = 300, fireTicks = 0, xpTotal = 0, ping = 0;
    double fallDistance = 0;
    bool dead = false;
    int invulnerableTicks = 0, lastAttacker = 0, regenTimer = 0, breach = 0;
    float lastDamage = 0;
    struct UsingItem {
        int hand = 0;
        std::string item;
        int64_t start = 0;
    };
    std::optional<UsingItem> usingItem;   // gerade am Essen/Trinken
    bool allowFlight = false;             // Fliegen im Überleben erlaubt (Plugins)
    float flySpeed = 0.05f, walkSpeed = 0.1f;
    std::set<Uuid> hiddenPlayers;         // diese Spieler sieht dieser Spieler nicht (hidePlayer)
    bool canSee(const Player* other) const { return !hiddenPlayers.count(other->uuid); }
    void refreshAbilities() { sendAbilities(); }
    std::set<ChunkPos> loadedChunks;
    std::optional<ChunkPos> center;
    std::string tabName;
    json::Value pluginData;
    std::unique_ptr<Window> window;     // gerade offenes Fenster (Fenster 0 = Inventar)

    bool isOp() const;
    int viewDistance() const;
    std::string heldItem() const;
    std::string address() const;

    // API
    void send(const std::string& packet, const Bytes& payload = Bytes());
    void sendActionBar(const std::string& msg);
    void sendTitle(const std::string& title, const std::string& subtitle = "", int fadeIn = 10, int stay = 70, int fadeOut = 20);
    void teleport(double x, double y, double z, std::optional<float> yaw = {}, std::optional<float> pitch = {});
    bool setGamemode(int mode);
    int addItem(const std::string& item, int count, bool sync = true);   // -> Rest
    int removeItem(const std::string& item, int count);
    void setSlot(int slot, const std::string& item, int count);
    void syncInventory();
    void closeWindow(bool notifyClient = false);
    int newWindowId() { nextWindowId_ = nextWindowId_ % 100 + 1; return nextWindowId_; }
    void openWindow(std::unique_ptr<Window> w);
    void sendWindowProperty(int prop, int value);
    void kick(const std::string& reason);
    void join();
    void tick();
    void tickKeepAlive();
    void sendPermissionLevel();
    void sendRuleFlags();
    void sendExperience();
    void sendHealth();
    void push(double vx, double vy, double vz);

    // Überleben
    bool isSurvival() const { return gamemode == 0 || gamemode == 2; }
    void setHealth(float h);
    void setFood(int f, float sat = -1);   // sat < 0 = unverändert
    void heal(float amount);
    void addExhaustion(float amount);
    void setAir(int a);
    int level() const;
    void giveXp(int points);
    void setLevel(int level);
    void takeLevels(int levels);
    void playSound(const std::string& name, float volume = 1, float pitch = 1, int category = 7);
    // attacker: Entity-ID (Spieler/Mob), 0 = keiner. -> true, wenn Schaden genommen
    bool damage(float amount, const std::string& cause = "generic", int attacker = 0);
    void kill(const std::string& message = "");
    void respawn();
    void finishUsingItem();
    void setHeldItem(const std::string& item, int count = 1);
    void sendMetadata();
    void setTabName(const std::string& name);   // "" = normaler Name
    void chatAs(const std::string& msg) { chat(msg); }
    void saveData();
    void requestChunkUpdate() { chunkDirty_ = true; }
    void sendChunks();   // ein paar Chunks pro Tick
    bool hasChunk(int cx, int cz) const { return loadedChunks.count({cx, cz}) > 0; }
    void resyncBlock(int x, int y, int z);
    void dropStack(const std::string& item, int count);

    // Sichtbarkeit
    std::vector<std::pair<std::string, Bytes>> spawnPackets(const Version& v) const;
    Bytes infoEntry() const;
    Bytes metadata(const Version& v) const;
    Bytes equipment(const Version& v) const;
    void broadcastPosition();
    void broadcastEquipment();

    void handlePacket(int pid, const Bytes& payload);

private:
    void loadData();
    std::string dataFile() const;
    void sendAbilities();
    void ack(int seq);
    void store(int slot, const Stack& st);
    void move(double nx, double ny, double nz, float nyaw, float npitch, int flags);
    void setSneaking(bool s);
    void setSprinting(bool s);
    // Fragt Java-Plugins (PlayerInteractEvent); true = abgebrochen
    bool interactEvent(int hand, const std::string& item, int x, int y, int z, int face, bool hasBlock);
    void onDig(Reader& r);
    void onPlace(Reader& r);
    void onCommand(const std::string& line);
    void chat(const std::string& msg);
    void die(const std::string& message);
    void trackFall(double dy);
    void interactEntity(int target, bool attack, int hand);

    int teleportId_ = 0;
    std::optional<int> awaitingTeleport_;
    int64_t keepAliveId_ = 0;
    double keepAliveSent_ = 0, lastKeepAlive_;
    bool chunkDirty_ = false;
    std::vector<ChunkPos> chunkQueue_;
    int stateId_ = 0;
    int nextWindowId_ = 0;
};
