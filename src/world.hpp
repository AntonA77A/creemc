// Welt, Chunks, Generatoren, Chunk-Pakete und Speichern (gleiches Format wie die Python-Version).
#pragma once
#include <array>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "json.hpp"
#include "util.hpp"

class Version;

constexpr int MIN_Y = -64;
constexpr int HEIGHT = 384;
constexpr int SECTIONS = HEIGHT / 16;
constexpr int SEA_LEVEL = 62;

struct ChunkPos {
    int x = 0, z = 0;
    bool operator==(const ChunkPos& o) const { return x == o.x && z == o.z; }
    bool operator<(const ChunkPos& o) const { return x != o.x ? x < o.x : z < o.z; }
};
struct ChunkPosHash {
    size_t operator()(const ChunkPos& p) const { return size_t(uint32_t(p.x)) * 73856093u ^ size_t(uint32_t(p.z)) * 19349663u; }
};

struct BlockPos {
    int x = 0, y = 0, z = 0;
    bool operator==(const BlockPos& o) const { return x == o.x && y == o.y && z == o.z; }
    bool operator<(const BlockPos& o) const { return x != o.x ? x < o.x : y != o.y ? y < o.y : z < o.z; }
};
struct BlockPosHash {
    size_t operator()(const BlockPos& p) const {
        return size_t(uint32_t(p.x)) * 73856093u ^ size_t(uint32_t(p.y)) * 19349663u ^ size_t(uint32_t(p.z)) * 83492791u;
    }
};

class Chunk {
public:
    int x, z;
    std::array<std::unique_ptr<std::vector<uint16_t>>, SECTIONS> sections;
    std::array<int, 256> heights;                     // höchster nicht-Luft-Block je Spalte
    std::array<uint8_t, 16> biomes{};                 // Biom je 4x4-Feld (Index in worldgen::biomeNames(), 0 = Ebene)
    std::map<std::tuple<int, int, int>, std::string> blockEntities;   // lokale Position -> Typ
    std::unordered_map<int, Bytes> frames;            // Protokoll -> fertiges Chunk-Paket

    Chunk(int x, int z);
    int get(int lx, int y, int lz) const;
    void set(int lx, int y, int lz, int state);
    Bytes payload(const Version& v) const;
};

class Generator {
public:
    virtual ~Generator() = default;
    virtual void generate(Chunk& c) = 0;
};

class World {
public:
    std::string name, folder, generatorName;
    int64_t seed = 0, age = 0, time = 1000;
    std::unordered_map<BlockPos, json::Value, BlockPosHash> blockData;   // Inhalt von Kisten, Öfen ...

    World(const std::string& name, const std::string& folder, const std::string& generator, int64_t seed);
    Chunk& chunk(int cx, int cz);
    int getBlock(int x, int y, int z);
    void setBlock(int x, int y, int z, int state);
    int highestBlock(int x, int z);
    void spawnPoint(double& x, double& y, double& z);
    void unloadUnused(const std::vector<ChunkPos>& keep);
    void save();

private:
    std::unique_ptr<Generator> gen_;
    std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash> chunks_;
    std::unordered_map<ChunkPos, std::map<std::tuple<int, int, int>, int>, ChunkPosHash> changes_;
    bool dirty_ = false;
    bool spawnFound_ = false;
    int spawnX_ = 0, spawnZ_ = 0;
    void load();
};

extern std::vector<uint8_t> FLUID_STATE;   // Zustand -> Wasser/Lava
