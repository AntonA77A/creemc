#include "world.hpp"

#include <algorithm>
#include <map>
#include <cmath>
#include <set>
#include <tuple>

#include "buffer.hpp"
#include "data.hpp"
#include "nbt.hpp"
#include "version.hpp"
#include "worldgen.hpp"

std::vector<uint8_t> FLUID_STATE;

namespace {
int AIR = 0, STONE, DIRT, GRASS, BEDROCK, WATER, SAND, GRAVEL, OAK_LOG, OAK_LEAVES, SHORT_GRASS, DANDELION, POPPY;
std::vector<std::string> BE_KIND;   // Zustand -> Block-Entity-Typ ("" = keiner)
bool g_init = false;

std::string renderKind(const std::string& n) {
    // Blöcke, die der Client nur mit Block-Entity zeichnet (sonst unsichtbar)
    if (n == "chest" || n == "trapped_chest" || n == "ender_chest" || n == "bell" || n == "decorated_pot") return n;
    if (endsWith(n, "shulker_box")) return "shulker_box";
    if (endsWith(n, "_hanging_sign")) return "hanging_sign";
    if (endsWith(n, "_sign")) return "sign";
    if ((endsWith(n, "_skull") || endsWith(n, "_head")) && n.find("piston") == std::string::npos) return "skull";
    if (endsWith(n, "_banner")) return "banner";
    if (endsWith(n, "_bed")) return "bed";
    return "";
}

void initStatics() {
    if (g_init) return;
    g_init = true;
    auto S = [](const char* n) { return data::blockState(n); };
    STONE = S("stone"); DIRT = S("dirt"); GRASS = S("grass_block"); BEDROCK = S("bedrock");
    WATER = S("water"); SAND = S("sand"); GRAVEL = S("gravel");
    OAK_LOG = S("oak_log"); OAK_LEAVES = S("oak_leaves");
    SHORT_GRASS = S("short_grass"); DANDELION = S("dandelion"); POPPY = S("poppy");
    FLUID_STATE.assign(size_t(data::MAX_STATE) + 1, 0);
    for (const char* f : {"water", "lava"}) {
        const auto& b = data::BLOCKS.at(f);
        for (int s = b.lo; s <= b.hi; ++s) FLUID_STATE[size_t(s)] = 1;
    }
    BE_KIND.assign(size_t(data::MAX_STATE) + 1, "");
    for (const auto& [n, b] : data::BLOCKS) {
        std::string k = renderKind(n);
        if (!k.empty())
            for (int s = b.lo; s <= b.hi; ++s) BE_KIND[size_t(s)] = k;
    }
}

constexpr int LIGHT_SECTIONS = SECTIONS + 2;

const Bytes& lightTail() {
    static Bytes tail = [] {
        Writer w;
        int64_t full = (int64_t(1) << LIGHT_SECTIONS) - 1;
        w.varint(1).i64(full);   // Himmelslicht vorhanden
        w.varint(0);             // Blocklicht vorhanden
        w.varint(0);             // Himmelslicht leer
        w.varint(1).i64(full);   // Blocklicht leer
        w.varint(LIGHT_SECTIONS);
        for (int i = 0; i < LIGHT_SECTIONS; ++i) w.varint(2048).raw(Bytes(2048, '\xff'));
        w.varint(0);
        return w.out;
    }();
    return tail;
}

std::vector<int64_t> packHeightmap(const std::array<int, 256>& heights) {
    std::vector<int64_t> longs;
    for (int i = 0; i < 256; i += 7) {
        uint64_t v = 0;
        for (int j = 0; j < 7 && i + j < 256; ++j) v |= uint64_t((heights[size_t(i + j)] - MIN_Y + 1) & 0x1FF) << (9 * j);
        longs.push_back(int64_t(v));
    }
    return longs;
}

// Eine Block-Sektion im Format der Version v -> (Bytes, Anzahl Longs)
std::pair<Bytes, int> serializeSection(const std::vector<uint16_t>* sec, const Version& v) {
    int p = v.protocol;
    bool lengthPrefix = p < V1_21_5;
    Writer w;
    auto header = [&](int count, int fluids) {
        w.i16(int16_t(count));
        if (p >= V26_1) w.i16(int16_t(fluids));
    };
    if (!sec) {
        header(0, 0);
        w.u8(0).varint(v.remap[0]);
        if (lengthPrefix) w.varint(0);
        return {w.out, 0};
    }
    std::vector<int> palette;
    {
        std::set<int> uniq(sec->begin(), sec->end());
        palette.assign(uniq.begin(), uniq.end());
    }
    int count = 0, fluids = 0;
    for (uint16_t s : *sec) {
        if (s != AIR) ++count;
        if (FLUID_STATE[s]) ++fluids;
    }
    if (palette.size() == 1) {
        int s = palette[0];
        header(s == AIR ? 0 : 4096, FLUID_STATE[size_t(s)] ? 4096 : 0);
        w.u8(0).varint(v.remap[size_t(s)]);
        if (lengthPrefix) w.varint(0);
        return {w.out, 0};
    }
    if (palette.size() <= 256) {
        std::unordered_map<int, int> index;
        for (size_t i = 0; i < palette.size(); ++i) index[palette[i]] = int(i);
        int bpe = palette.size() <= 16 ? 4 : 8;
        int per = 64 / bpe;
        int n = 4096 / per;
        header(count, fluids);
        w.u8(uint8_t(bpe)).varint(int32_t(palette.size()));
        for (int s : palette) w.varint(v.remap[size_t(s)]);
        if (lengthPrefix) w.varint(n);
        for (int i = 0; i < 4096; i += per) {
            uint64_t x = 0;
            for (int j = 0; j < per; ++j) x |= uint64_t(index[(*sec)[size_t(i + j)]]) << (bpe * j);
            w.i64(int64_t(x));
        }
        return {w.out, n};
    }
    // Direktes Format (globale Zustands-IDs)
    int bpe = v.stateBits;
    int per = 64 / bpe;
    int n = (4096 + per - 1) / per;
    header(count, fluids);
    w.u8(uint8_t(bpe));
    if (lengthPrefix) w.varint(n);
    for (int i = 0; i < 4096; i += per) {
        uint64_t x = 0;
        for (int j = 0; j < per && i + j < 4096; ++j) x |= uint64_t(v.remap[(*sec)[size_t(i + j)]]) << (bpe * j);
        w.i64(int64_t(x));
    }
    return {w.out, n};
}

// ---- Generatoren ----

class FlatGenerator : public Generator {
public:
    void generate(Chunk& c) override {
        int layers[4] = {BEDROCK, DIRT, DIRT, GRASS};
        auto sec = std::make_unique<std::vector<uint16_t>>(4096, uint16_t(AIR));
        for (int i = 0; i < 4; ++i)
            std::fill(sec->begin() + i * 256, sec->begin() + (i + 1) * 256, uint16_t(layers[i]));
        c.sections[0] = std::move(sec);
        c.heights.fill(MIN_Y + 3);
    }
};

class NormalGenerator : public Generator {
public:
    explicit NormalGenerator(int64_t seed) : seed_(uint32_t(seed & 0xFFFFFFFF)) {}

    void generate(Chunk& c) override {
        int bx = c.x * 16, bz = c.z * 16;
        double grid[5][5];
        for (int gz = 0; gz < 5; ++gz)
            for (int gx = 0; gx < 5; ++gx) grid[gz][gx] = heightF(bx + gx * 4, bz + gz * 4);
        int heights[256];
        for (int z = 0; z < 16; ++z) {
            int gz = z >> 2;
            double tz = (z & 3) / 4.0;
            for (int x = 0; x < 16; ++x) {
                int gx = x >> 2;
                double tx = (x & 3) / 4.0;
                double a = grid[gz][gx] + (grid[gz][gx + 1] - grid[gz][gx]) * tx;
                double b = grid[gz + 1][gx] + (grid[gz + 1][gx + 1] - grid[gz + 1][gx]) * tx;
                heights[z * 16 + x] = int(a + (b - a) * tz);
            }
        }
        int maxH = SEA_LEVEL;
        for (int h : heights) maxH = std::max(maxH, h);
        int topSi = (maxH - MIN_Y) >> 4;
        for (int si = 0; si <= topSi && si < SECTIONS; ++si) {
            auto s = std::make_unique<std::vector<uint16_t>>(4096, uint16_t(AIR));
            for (int col = 0; col < 256; ++col) {
                int h = heights[col];
                int surface, under;
                if (h < SEA_LEVEL - 3) { surface = GRAVEL; under = DIRT; }
                else if (h <= SEA_LEVEL + 1) { surface = SAND; under = SAND; }
                else { surface = GRASS; under = DIRT; }
                for (int ly = 0; ly < 16; ++ly) {
                    int y = MIN_Y + si * 16 + ly;
                    int st;
                    if (y == MIN_Y) st = BEDROCK;
                    else if (y <= h - 4) st = STONE;
                    else if (y < h) st = under;
                    else if (y == h) st = surface;
                    else if (y <= SEA_LEVEL) st = WATER;
                    else st = AIR;
                    (*s)[size_t(ly * 256 + col)] = uint16_t(st);
                }
            }
            c.sections[size_t(si)] = std::move(s);
        }
        for (int col = 0; col < 256; ++col) c.heights[size_t(col)] = std::max(heights[col], SEA_LEVEL);

        // Deko: Gras, Blumen, Bäume (gleiches Ergebnis pro Seed und Chunk)
        std::mt19937 r(uint32_t((int64_t(seed_) * 31 + c.x) * 31 + c.z));
        std::uniform_real_distribution<double> u(0.0, 1.0);
        for (int i = 0; i < 256; ++i) {
            int x = i & 15, z = i >> 4, h = heights[i];
            if (c.get(x, h, z) != GRASS) continue;
            double v = u(r);
            if (v < 0.12) c.set(x, h + 1, z, SHORT_GRASS);
            else if (v < 0.13) c.set(x, h + 1, z, u(r) < 0.5 ? DANDELION : POPPY);
        }
        int trees = int(r() % 3);
        for (int t = 0; t < trees; ++t) {
            int x = 2 + int(r() % 12), z = 2 + int(r() % 12);
            int h = heights[(z << 4) | x];
            if (c.get(x, h, z) != GRASS) continue;
            int top = h + 4 + int(r() % 3);
            for (int dy = -2; dy < 2; ++dy) {
                int radius = dy < 0 ? 2 : 1;
                for (int dx = -radius; dx <= radius; ++dx)
                    for (int dz = -radius; dz <= radius; ++dz) {
                        if (std::abs(dx) == radius && std::abs(dz) == radius && (dy == 1 || u(r) < 0.5)) continue;
                        if (c.get(x + dx, top + dy, z + dz) == AIR) c.set(x + dx, top + dy, z + dz, OAK_LEAVES);
                    }
            }
            for (int y = h + 1; y <= top; ++y) c.set(x, y, z, OAK_LOG);
            c.set(x, h, z, DIRT);
        }
    }

private:
    uint32_t seed_;

    double rand(int ix, int iz, int salt) const {
        uint32_t h = uint32_t(ix) * 374761393u + uint32_t(iz) * 668265263u + (seed_ + uint32_t(salt)) * 2246822519u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return double((h ^ (h >> 16)) & 0xFFFF) / 65535.0;
    }
    double noise(double x, double z, double scale, int salt) const {
        double fx = x / scale, fz = z / scale;
        int ix = int(std::floor(fx)), iz = int(std::floor(fz));
        double tx = fx - ix, tz = fz - iz;
        tx = tx * tx * (3 - 2 * tx);
        tz = tz * tz * (3 - 2 * tz);
        double a = rand(ix, iz, salt), b = rand(ix + 1, iz, salt), c = rand(ix, iz + 1, salt), d = rand(ix + 1, iz + 1, salt);
        double top = a + (b - a) * tx;
        return top + ((c + (d - c) * tx) - top) * tz;
    }
    double heightF(int x, int z) const {
        return 40 + (noise(x, z, 160, 1) * 34 + noise(x, z, 60, 2) * 12 + noise(x, z, 20, 3) * 4);
    }
};

std::string encodeBlock(int state) {
    std::string n = data::blockName(state);
    const auto& b = data::BLOCKS.at(removePrefix(n, "minecraft:"));
    return state == b.def ? n : n + "#" + std::to_string(state - b.lo);
}

int decodeBlock(const json::Value& v) {
    if (v.isNumber()) return AIR;   // sehr altes Format (Zustands-IDs aus 1.21.4) wird nicht mehr gelesen
    std::string s = v.asString();
    std::string name = s, offset;
    size_t h = s.find('#');
    if (h != std::string::npos) {
        name = s.substr(0, h);
        offset = s.substr(h + 1);
    }
    auto it = data::BLOCKS.find(removePrefix(name, "minecraft:"));
    if (it == data::BLOCKS.end()) return AIR;
    if (!offset.empty()) return std::min(it->second.hi, it->second.lo + std::stoi(offset));
    return it->second.def;
}

}  // namespace

// ---- Chunk ------------------------------------------------------------------

Chunk::Chunk(int x_, int z_) : x(x_), z(z_) {
    initStatics();
    heights.fill(MIN_Y - 1);
}

int Chunk::get(int lx, int y, int lz) const {
    if (y < MIN_Y || y >= MIN_Y + HEIGHT || lx < 0 || lx > 15 || lz < 0 || lz > 15) return AIR;
    const auto& s = sections[size_t((y - MIN_Y) >> 4)];
    if (!s) return AIR;
    return (*s)[size_t((((y - MIN_Y) & 15) << 8) | (lz << 4) | lx)];
}

void Chunk::set(int lx, int y, int lz, int state) {
    if (y < MIN_Y || y >= MIN_Y + HEIGHT || lx < 0 || lx > 15 || lz < 0 || lz > 15) return;
    auto& s = sections[size_t((y - MIN_Y) >> 4)];
    if (!s) {
        if (state == AIR) return;
        s = std::make_unique<std::vector<uint16_t>>(4096, uint16_t(AIR));
    }
    (*s)[size_t((((y - MIN_Y) & 15) << 8) | (lz << 4) | lx)] = uint16_t(state);
    frames.clear();
    const std::string& kind = BE_KIND[size_t(state)];
    auto key = std::make_tuple(lx, y, lz);
    if (!kind.empty()) blockEntities[key] = kind;
    else blockEntities.erase(key);
    int col = (lz << 4) | lx;
    if (state != AIR && y > heights[size_t(col)]) {
        heights[size_t(col)] = y;
    } else if (state == AIR && y == heights[size_t(col)]) {
        int yy = y - 1;
        while (yy >= MIN_Y && get(lx, yy, lz) == AIR) --yy;
        heights[size_t(col)] = yy;
    }
}

Bytes Chunk::payload(const Version& v) const {
    int p = v.protocol;
    // Biome: gleich für alle Sektionen (je 4x4-Feld einer Spalte), als Palette im Format der Version
    static std::map<int, std::vector<int>> biomeIdCache;
    auto& ids = biomeIdCache[p];
    if (ids.empty())
        for (const auto& bn : worldgen::biomeNames()) {
            int id = v.registryFind("minecraft:worldgen/biome", "minecraft:" + bn);
            ids.push_back(id >= 0 ? id : v.plains);
        }
    std::vector<int> palette;
    int cellIdx[16];
    for (int i = 0; i < 16; ++i) {
        int id = ids[biomes[size_t(i)] < ids.size() ? biomes[size_t(i)] : 0];
        auto pos = std::find(palette.begin(), palette.end(), id);
        if (pos == palette.end()) {
            palette.push_back(id);
            pos = palette.end() - 1;
        }
        cellIdx[i] = int(pos - palette.begin());
    }
    Writer biome;
    int biomeLongs = 0;
    if (palette.size() == 1) {
        biome.u8(0).varint(palette[0]);
        if (p < V1_21_5) biome.varint(0);
    } else {
        int total = v.registryCount("minecraft:worldgen/biome");
        bool direct = palette.size() > 8;
        int bits = direct ? std::max(1, int(std::ceil(std::log2(double(std::max(2, total)))))) : std::max(1, int(std::ceil(std::log2(double(palette.size())))));
        int per = 64 / bits;
        biomeLongs = (64 + per - 1) / per;
        biome.u8(uint8_t(bits));
        if (!direct) {
            biome.varint(int32_t(palette.size()));
            for (int id : palette) biome.varint(id);
        }
        if (p < V1_21_5) biome.varint(biomeLongs);
        for (int l = 0; l < biomeLongs; ++l) {
            uint64_t word = 0;
            for (int j = 0; j < per; ++j) {
                int i = l * per + j;   // Index: (y << 4) | (z << 2) | x, alle Höhen gleich
                if (i >= 64) break;
                int cell = i & 15;
                uint64_t val = uint64_t(direct ? palette[size_t(cellIdx[cell])] : cellIdx[cell]);
                word |= val << (bits * j);
            }
            biome.i64(int64_t(word));
        }
    }
    Bytes body;
    size_t padding = 0;
    for (const auto& s : sections) {
        auto [blocks, nLongs] = serializeSection(s.get(), v);
        body += blocks;
        body += biome.out;
        if (p == V1_21_5) padding += varintBytes(nLongs).size() + varintBytes(biomeLongs).size();   // 1.21.5 erwartet noch Platz dafür
    }
    body.append(padding, '\0');

    Writer w;
    w.i32(x).i32(z);
    auto hm = packHeightmap(heights);
    if (p >= V1_21_5) {
        Writer raw;
        raw.varint(int32_t(hm.size()));
        for (int64_t l : hm) raw.i64(l);
        w.varint(2).varint(1).raw(raw.out).varint(4).raw(raw.out);   // WORLD_SURFACE, MOTION_BLOCKING
    } else {
        nbt::Tag t = nbt::Tag::compound();
        t.put("MOTION_BLOCKING", nbt::Tag::longArray(hm)).put("WORLD_SURFACE", nbt::Tag::longArray(hm));
        w.raw(t.encode());
    }
    w.prefixed(body);
    std::vector<Bytes> entries;
    for (const auto& [pos, kind] : blockEntities) {
        auto it = v.blockEntities.find(kind);
        if (it == v.blockEntities.end()) continue;   // z.B. Betten sind ab 26.2 keine Block-Entities mehr
        auto [lx, y, lz] = pos;
        Writer e;
        e.u8(uint8_t((lx << 4) | lz)).i16(int16_t(y)).varint(it->second).u8(0x0A).u8(0x00);
        entries.push_back(e.out);
    }
    w.varint(int32_t(entries.size()));
    for (const auto& e : entries) w.raw(e);
    w.raw(lightTail());
    return w.out;
}

// ---- World ------------------------------------------------------------------

World::World(const std::string& n, const std::string& f, const std::string& generator, int64_t s)
    : name(n), folder(f), generatorName(generator), seed(s) {
    initStatics();
    // Eine bestehende Welt behält ihren Generator und Seed (sonst würde sich ihr Gelände ändern)
    std::string txt;
    if (readFile(joinPath(folder, "world.json"), txt)) {
        try {
            auto m = json::parse(txt);
            if (m["generator"].isString()) generatorName = m["generator"].asString();
            else if (readFile(joinPath(folder, "blocks.json"), txt) && generatorName == "realistic") generatorName = "normal";   // alte Welt
            if (m["seed"].isNumber()) seed = m["seed"].asInt(seed);
        } catch (...) {
        }
    }
    if (generatorName != generator) logx::info("Welt '" + name + "' benutzt weiter den Generator '" + generatorName + "'");
    if (generatorName == "flat") gen_ = std::make_unique<FlatGenerator>();
    else if (generatorName == "normal") gen_ = std::make_unique<NormalGenerator>(seed);
    else if (generatorName == "realistic") gen_ = worldgen::createRealistic(seed);
    else throw std::runtime_error("Unbekannter Generator '" + generatorName + "' (verfügbar: realistic, normal, flat)");
    load();
}

void World::load() {
    makeDirs(folder);
    std::string txt;
    if (readFile(joinPath(folder, "world.json"), txt)) {
        auto m = json::parse(txt);
        age = m["age"].asInt(0);
        time = m["time"].asInt(1000);
    }
    if (readFile(joinPath(folder, "blocks.json"), txt)) {
        auto raw = json::parse(txt);
        for (const auto& [key, blocks] : raw.asObject()) {
            auto xz = split(key, ',');
            ChunkPos cp{std::stoi(xz[0]), std::stoi(xz[1])};
            auto& ch = changes_[cp];
            for (const auto& b : blocks.asArray())
                ch[std::make_tuple(int(b[0].asInt()), int(b[1].asInt()), int(b[2].asInt()))] = decodeBlock(b[3]);
        }
        logx::info("Welt '" + name + "' geladen (" + std::to_string(changes_.size()) + " geänderte Chunks)");
    }
    if (readFile(joinPath(folder, "block_data.json"), txt)) {
        auto raw = json::parse(txt);
        for (const auto& [key, v] : raw.asObject()) {
            auto p = split(key, ',');
            if (p.size() == 3) blockData[{std::stoi(p[0]), std::stoi(p[1]), std::stoi(p[2])}] = v;
        }
    }
}

void World::save() {
    makeDirs(folder);
    json::Object meta;
    meta["seed"] = seed;
    meta["generator"] = generatorName;
    meta["age"] = age;
    meta["time"] = time;
    meta["format"] = 2;
    writeFileAtomic(joinPath(folder, "world.json"), json::write(json::Value(std::move(meta))));
    json::Object bd;
    for (const auto& [p, v] : blockData)
        bd[std::to_string(p.x) + "," + std::to_string(p.y) + "," + std::to_string(p.z)] = v;
    writeFileAtomic(joinPath(folder, "block_data.json"), json::write(json::Value(std::move(bd))));
    if (!dirty_) return;
    json::Object raw;
    for (const auto& [cp, blocks] : changes_) {
        json::Array list;
        for (const auto& [pos, state] : blocks) {
            auto [x, y, z] = pos;
            list.push_back(json::Array{x, y, z, encodeBlock(state)});
        }
        raw[std::to_string(cp.x) + "," + std::to_string(cp.z)] = json::Value(std::move(list));
    }
    writeFileAtomic(joinPath(folder, "blocks.json"), json::write(json::Value(std::move(raw))));
    dirty_ = false;
}

Chunk& World::chunk(int cx, int cz) {
    ChunkPos cp{cx, cz};
    auto it = chunks_.find(cp);
    if (it != chunks_.end()) return *it->second;
    auto c = std::make_unique<Chunk>(cx, cz);
    gen_->generate(*c);
    auto ch = changes_.find(cp);
    if (ch != changes_.end())
        for (const auto& [pos, state] : ch->second) {
            auto [x, y, z] = pos;
            c->set(x, y, z, state);
        }
    Chunk& ref = *c;
    chunks_[cp] = std::move(c);
    return ref;
}

int World::getBlock(int x, int y, int z) { return chunk(x >> 4, z >> 4).get(x & 15, y, z & 15); }

void World::setBlock(int x, int y, int z, int state) {
    int cx = x >> 4, cz = z >> 4;
    Chunk& c = chunk(cx, cz);
    auto bd = blockData.find({x, y, z});
    if (bd != blockData.end() && data::blockName(c.get(x & 15, y, z & 15)) != data::blockName(state))
        blockData.erase(bd);   // anderer Block -> Inhalt (Kiste, Ofen) gehört nicht mehr dazu
    c.set(x & 15, y, z & 15, state);
    changes_[{cx, cz}][std::make_tuple(x & 15, y, z & 15)] = state;
    dirty_ = true;
}

int World::highestBlock(int x, int z) { return chunk(x >> 4, z >> 4).heights[size_t(((z & 15) << 4) | (x & 15))]; }

void World::spawnPoint(double& x, double& y, double& z) {
    if (!spawnFound_) {   // realistische Welt: Land in der Nähe von 0,0 suchen (Spirale)
        spawnFound_ = true;
        spawnX_ = 0;
        spawnZ_ = 0;
        int h;
        bool found = false;
        for (int r = 0; r <= 2048 && !found; r += 16)
            for (int a = 0; a < (r == 0 ? 1 : 16) && !found; ++a) {
                int sx = int(r * std::cos(a * 0.3927)), sz = int(r * std::sin(a * 0.3927));
                if (worldgen::isLand(*gen_, sx, sz, h)) {
                    spawnX_ = sx;
                    spawnZ_ = sz;
                    found = true;
                }
            }
    }
    x = spawnX_ + 0.5;
    y = highestBlock(spawnX_, spawnZ_) + 1.0;
    z = spawnZ_ + 0.5;
}

void World::unloadUnused(const std::vector<ChunkPos>& keep) {
    std::set<ChunkPos> k(keep.begin(), keep.end());
    for (auto it = chunks_.begin(); it != chunks_.end();) {
        if (!k.count(it->first)) it = chunks_.erase(it);
        else ++it;
    }
}
