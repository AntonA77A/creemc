// Unterstützte Minecraft-Versionen (1.21 bis 26.3) und ihre Protokollunterschiede.
// Jeder Client bekommt ein Version-Objekt: Paket-IDs, Block-/Item-Umrechnung, versionsabhängige Pakete.
#pragma once
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "buffer.hpp"
#include "json.hpp"

constexpr int V1_21 = 767;
constexpr int V1_21_2 = 768;
constexpr int V1_21_4 = 769;
constexpr int V1_21_5 = 770;
constexpr int V1_21_6 = 771;
constexpr int V1_21_9 = 773;
constexpr int V26_1 = 775;
constexpr int V26_2 = 776;
constexpr int V26_3 = 777;

struct ProfileProperty {
    std::string name, value, signature;
};

Bytes profileProperties(const std::vector<ProfileProperty>& props);
Bytes legacyVelocity(double vx, double vy, double vz);
Bytes lpVec3(double vx, double vy, double vz);
Bytes soundPacket(const std::string& name, double x, double y, double z, float volume, float pitch, int category);

class Version {
public:
    static const Version* get(int protocol);   // nullptr = nicht unterstützt

    int protocol = 0;
    std::vector<std::string> names;
    std::string name;
    std::unordered_map<std::string, int> ids;
    std::unordered_map<int, std::string> idNames;          // Play-Paket-ID -> logischer Name (nur c_*)
    int playerEntity = 0;
    std::unordered_map<std::string, int> entities, metaTypes, blockEntities, components, items, particles;
    std::unordered_map<int, std::string> itemNames;
    std::unordered_map<std::string, int> blockIds;          // "minecraft:stone" -> Block-Registry-ID
    std::vector<int> remap;                                 // kanonischer Zustand -> Zustand dieser Version
    int stateBits = 15;
    int plains = 0, overworld = 0, overworldClock = 0;

    int id(const std::string& packet) const;
    bool has(const std::string& packet) const { return ids.count(packet) > 0; }
    int registryIndex(const std::string& registry, const std::string& entry) const;
    int registryFind(const std::string& registry, const std::string& entry) const;   // -1 = gibt es nicht
    int registryCount(const std::string& registry) const;
    std::optional<int> itemId(const std::string& name) const;
    std::string itemName(int id) const;
    std::optional<int> entityType(const std::string& name) const;

    // Konfiguration
    Bytes knownPacks() const;
    std::vector<Bytes> registryPackets() const;
    const Bytes& tagsPayload() const;

    // Versionsabhängige Pakete
    Bytes loginSuccess(const Uuid& uuid, const std::string& name, const std::vector<ProfileProperty>& props) const;
    Bytes login(int entityId, int gamemode, int maxPlayers, int viewDistance, bool isFlat, bool onlineMode) const;
    Bytes respawn(int gamemode, bool isFlat) const;
    Bytes syncPlayerPosition(int teleportId, double x, double y, double z, float yaw, float pitch) const;
    std::pair<std::string, Bytes> entityPosition(int eid, double x, double y, double z, float yaw, float pitch, bool onGround) const;
    Bytes spawnEntity(int eid, const Uuid& uuid, int type, double x, double y, double z, float yaw, float pitch,
                      double vx = 0, double vy = 0, double vz = 0, int extra = 0) const;
    Bytes spawnPlayer(int eid, const Uuid& uuid, double x, double y, double z, float yaw, float pitch) const;
    Bytes velocity(int eid, double vx, double vy, double vz) const;
    Bytes itemMetadata(int eid, const std::string& item, int count) const;
    Bytes flagsMetadata(int eid, int flags) const;
    Bytes airMetadata(int eid, int air) const;
    Bytes playerMetadata(int eid, bool sneaking, int skinParts, bool burning) const;
    Bytes windowItems(int windowId, int stateId, const std::vector<std::pair<std::string, int>>& slots,
                      const std::pair<std::string, int>& cursor) const;
    Bytes slot(const std::string& item, int count) const;
    int enchantmentId(const std::string& name) const;
    std::pair<std::string, Bytes> swing(int eid, bool offHand) const;
    Bytes spawnPosition(int x, int y, int z) const;
    Bytes updateTime(int64_t age, int64_t time, bool ticking) const;

    struct Interact {
        int target = 0;
        bool attack = false;
        int hand = 0;
    };
    Interact readInteract(Reader& r) const;

private:
    explicit Version(int protocol);
    std::map<std::string, std::vector<std::string>> registries_;
    std::vector<std::string> registryOrder_;
    json::Value tags_;
    mutable Bytes tagsPayload_;
    std::vector<Bytes> buildComponents(const json::Value& meta) const;
};
