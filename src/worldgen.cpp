#include "worldgen.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <numeric>
#include <random>
#include <unordered_map>

#include "data.hpp"
#include "util.hpp"

namespace worldgen {
namespace {

// ---- Rauschen (Perlin, mit Seed gemischt) -----------------------------------------

class Perlin {
public:
    explicit Perlin(uint64_t seed) {
        std::array<int, 256> p{};
        std::iota(p.begin(), p.end(), 0);
        std::mt19937_64 r(seed);
        std::shuffle(p.begin(), p.end(), r);
        for (int i = 0; i < 512; ++i) perm_[size_t(i)] = uint8_t(p[size_t(i & 255)]);
        std::uniform_real_distribution<double> off(0, 256);
        ox_ = off(r);
        oy_ = off(r);
        oz_ = off(r);
    }
    double noise2(double x, double y) const {
        x += ox_;
        y += oz_;
        int xi = int(std::floor(x)), yi = int(std::floor(y));
        double xf = x - xi, yf = y - yi;
        int X = xi & 255, Y = yi & 255;
        double u = fade(xf), v = fade(yf);
        int aa = perm_[size_t(perm_[size_t(X)] + Y)], ab = perm_[size_t(perm_[size_t(X)] + Y + 1)];
        int ba = perm_[size_t(perm_[size_t(X + 1)] + Y)], bb = perm_[size_t(perm_[size_t(X + 1)] + Y + 1)];
        double x1 = lerp(u, grad2(aa, xf, yf), grad2(ba, xf - 1, yf));
        double x2 = lerp(u, grad2(ab, xf, yf - 1), grad2(bb, xf - 1, yf - 1));
        return lerp(v, x1, x2);
    }
    double noise3(double x, double y, double z) const {
        x += ox_;
        y += oy_;
        z += oz_;
        int xi = int(std::floor(x)), yi = int(std::floor(y)), zi = int(std::floor(z));
        double xf = x - xi, yf = y - yi, zf = z - zi;
        int X = xi & 255, Y = yi & 255, Z = zi & 255;
        double u = fade(xf), v = fade(yf), w = fade(zf);
        auto P = [this](int i) { return int(perm_[size_t(i)]); };
        int A = P(X) + Y, AA = P(A) + Z, AB = P(A + 1) + Z, B = P(X + 1) + Y, BA = P(B) + Z, BB = P(B + 1) + Z;
        return lerp(w,
                    lerp(v, lerp(u, grad3(P(AA), xf, yf, zf), grad3(P(BA), xf - 1, yf, zf)),
                         lerp(u, grad3(P(AB), xf, yf - 1, zf), grad3(P(BB), xf - 1, yf - 1, zf))),
                    lerp(v, lerp(u, grad3(P(AA + 1), xf, yf, zf - 1), grad3(P(BA + 1), xf - 1, yf, zf - 1)),
                         lerp(u, grad3(P(AB + 1), xf, yf - 1, zf - 1), grad3(P(BB + 1), xf - 1, yf - 1, zf - 1))));
    }

private:
    std::array<uint8_t, 512> perm_{};
    double ox_, oy_, oz_;
    static double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
    static double lerp(double t, double a, double b) { return a + t * (b - a); }
    static double grad2(int h, double x, double y) {
        switch (h & 7) {
            case 0: return x + y;
            case 1: return -x + y;
            case 2: return x - y;
            case 3: return -x - y;
            case 4: return x;
            case 5: return -x;
            case 6: return y;
            default: return -y;
        }
    }
    static double grad3(int h, double x, double y, double z) {
        int hh = h & 15;
        double u = hh < 8 ? x : y, v = hh < 4 ? y : (hh == 12 || hh == 14 ? x : z);
        return ((hh & 1) ? -u : u) + ((hh & 2) ? -v : v);
    }
};

class Octaves {
public:
    Octaves(uint64_t seed, int n, double freq) : freq_(freq) {
        for (int i = 0; i < n; ++i) o_.emplace_back(seed * 6364136223846793005ULL + uint64_t(i) * 1442695040888963407ULL + 1);
    }
    // ungefähr -1 .. 1
    double at2(double x, double z) const {
        double sum = 0, amp = 1, total = 0, f = freq_;
        for (const auto& p : o_) {
            sum += p.noise2(x * f, z * f) * amp;
            total += amp;
            amp *= 0.5;
            f *= 2;
        }
        return std::max(-1.0, std::min(1.0, sum / total * 1.6));
    }
    double at3(double x, double y, double z, double yScale) const {
        double sum = 0, amp = 1, total = 0, f = freq_;
        for (const auto& p : o_) {
            sum += p.noise3(x * f, y * f * yScale, z * f) * amp;
            total += amp;
            amp *= 0.5;
            f *= 2;
        }
        return sum / total;
    }

private:
    std::vector<Perlin> o_;
    double freq_;
};

constexpr double SPAG = 0.011;   // Spaghetti-Höhlen: je größer, desto mehr Gänge

double smooth(double a, double b, double x) {
    double t = std::max(0.0, std::min(1.0, (x - a) / (b - a)));
    return t * t * (3 - 2 * t);
}
double lerpd(double a, double b, double t) { return a + (b - a) * t; }

// Kurve aus Stützpunkten (x aufsteigend)
double spline(const std::vector<std::pair<double, double>>& pts, double x) {
    if (x <= pts.front().first) return pts.front().second;
    for (size_t i = 1; i < pts.size(); ++i)
        if (x <= pts[i].first) {
            double t = (x - pts[i - 1].first) / (pts[i].first - pts[i - 1].first);
            t = t * t * (3 - 2 * t);
            return lerpd(pts[i - 1].second, pts[i].second, t);
        }
    return pts.back().second;
}

uint64_t mix(uint64_t a, uint64_t b) {
    uint64_t h = a * 0x9E3779B97F4A7C15ULL ^ (b + 0x632BE59BD9B4E019ULL + (a << 6) + (a >> 2));
    h ^= h >> 31;
    h *= 0xBF58476D1CE4E5B9ULL;
    return h ^ (h >> 29);
}

// ---- Blöcke -----------------------------------------------------------------

int S(const char* n) {
    int s = data::blockState(n);
    return s < 0 ? data::blockState("stone") : s;
}
int SP(const char* n, std::initializer_list<std::pair<const char*, const char*>> props) {
    int s = S(n);
    std::map<std::string, std::string> m;
    for (const auto& [k, v] : props) m[k] = v;
    return data::withProps(s, m);
}

struct Blocks {
    int air = 0, stone, deepslate, bedrock, water, lava, dirt, grass, snowyGrass, coarse, podzol, sand, redSand, sandstone,
        redSandstone, gravel, clay, snowBlock, snowLayer, ice, packedIce, mud, tuff, granite, diorite, andesite, calcite;
    int terracotta[7];
    int shortGrass, tallLow, tallUp, fern, largeFernLow, largeFernUp, deadBush, cactus, sugarCane, lilyPad, seagrass, kelp,
        kelpPlant, sweetBerry, pumpkin, melon, brownMushroom, redMushroom, sunflowerLow, sunflowerUp;
    std::vector<int> flowers, forestFlowers, swampFlowers;
    int coal, dCoal, iron, dIron, copper, dCopper, gold, dGold, redstone, dRedstone, diamond, dDiamond, lapis, dLapis, emerald, dEmerald;
    Blocks() {
        stone = S("stone"); deepslate = SP("deepslate", {{"axis", "y"}}); bedrock = S("bedrock"); water = S("water"); lava = S("lava");
        dirt = S("dirt"); grass = SP("grass_block", {{"snowy", "false"}}); snowyGrass = SP("grass_block", {{"snowy", "true"}});
        coarse = S("coarse_dirt"); podzol = SP("podzol", {{"snowy", "false"}}); sand = S("sand"); redSand = S("red_sand");
        sandstone = S("sandstone"); redSandstone = S("red_sandstone"); gravel = S("gravel"); clay = S("clay");
        snowBlock = S("snow_block"); snowLayer = SP("snow", {{"layers", "1"}}); ice = S("ice"); packedIce = S("packed_ice");
        mud = S("mud"); tuff = S("tuff"); granite = S("granite"); diorite = S("diorite"); andesite = S("andesite"); calcite = S("calcite");
        const char* terr[7] = {"terracotta", "orange_terracotta", "yellow_terracotta", "white_terracotta", "red_terracotta",
                               "brown_terracotta", "light_gray_terracotta"};
        for (int i = 0; i < 7; ++i) terracotta[i] = S(terr[i]);
        shortGrass = data::blockState("short_grass") >= 0 ? S("short_grass") : S("grass");
        tallLow = SP("tall_grass", {{"half", "lower"}}); tallUp = SP("tall_grass", {{"half", "upper"}});
        fern = S("fern"); largeFernLow = SP("large_fern", {{"half", "lower"}}); largeFernUp = SP("large_fern", {{"half", "upper"}});
        sunflowerLow = SP("sunflower", {{"half", "lower"}}); sunflowerUp = SP("sunflower", {{"half", "upper"}});
        deadBush = S("dead_bush"); cactus = S("cactus"); sugarCane = S("sugar_cane"); lilyPad = S("lily_pad");
        seagrass = S("seagrass"); kelp = S("kelp"); kelpPlant = S("kelp_plant"); sweetBerry = SP("sweet_berry_bush", {{"age", "3"}});
        pumpkin = S("pumpkin"); melon = S("melon"); brownMushroom = S("brown_mushroom"); redMushroom = S("red_mushroom");
        for (const char* f : {"dandelion", "poppy", "azure_bluet", "oxeye_daisy", "cornflower"}) flowers.push_back(S(f));
        for (const char* f : {"dandelion", "poppy", "allium", "azure_bluet", "red_tulip", "orange_tulip", "white_tulip", "pink_tulip",
                              "oxeye_daisy", "cornflower", "lily_of_the_valley"})
            forestFlowers.push_back(S(f));
        swampFlowers = {S("blue_orchid")};
        coal = S("coal_ore"); dCoal = S("deepslate_coal_ore"); iron = S("iron_ore"); dIron = S("deepslate_iron_ore");
        copper = S("copper_ore"); dCopper = S("deepslate_copper_ore"); gold = S("gold_ore"); dGold = S("deepslate_gold_ore");
        redstone = SP("redstone_ore", {{"lit", "false"}}); dRedstone = SP("deepslate_redstone_ore", {{"lit", "false"}});
        diamond = S("diamond_ore"); dDiamond = S("deepslate_diamond_ore"); lapis = S("lapis_ore"); dLapis = S("deepslate_lapis_ore");
        emerald = S("emerald_ore"); dEmerald = S("deepslate_emerald_ore");
    }
};

const Blocks& B() {
    static const Blocks b;
    return b;
}

// ---- Biome ------------------------------------------------------------------

const std::vector<std::string> BIOMES = {
    "plains", "sunflower_plains", "forest", "flower_forest", "birch_forest", "dark_forest", "taiga", "old_growth_spruce_taiga",
    "snowy_plains", "snowy_taiga", "ice_spikes", "savanna", "windswept_savanna", "desert", "badlands", "eroded_badlands", "jungle",
    "sparse_jungle", "swamp", "mangrove_swamp", "meadow", "cherry_grove", "grove", "snowy_slopes", "jagged_peaks", "frozen_peaks",
    "stony_peaks", "windswept_hills", "beach", "snowy_beach", "stony_shore", "river", "frozen_river", "ocean", "deep_ocean",
    "cold_ocean", "deep_cold_ocean", "frozen_ocean", "deep_frozen_ocean", "lukewarm_ocean", "deep_lukewarm_ocean", "warm_ocean"};

enum Biome {
    PLAINS, SUNFLOWER_PLAINS, FOREST, FLOWER_FOREST, BIRCH_FOREST, DARK_FOREST, TAIGA, OLD_TAIGA, SNOWY_PLAINS, SNOWY_TAIGA,
    ICE_SPIKES, SAVANNA, WINDSWEPT_SAVANNA, DESERT, BADLANDS, ERODED_BADLANDS, JUNGLE, SPARSE_JUNGLE, SWAMP, MANGROVE_SWAMP,
    MEADOW, CHERRY_GROVE, GROVE, SNOWY_SLOPES, JAGGED_PEAKS, FROZEN_PEAKS, STONY_PEAKS, WINDSWEPT_HILLS, BEACH, SNOWY_BEACH,
    STONY_SHORE, RIVER, FROZEN_RIVER, OCEAN, DEEP_OCEAN, COLD_OCEAN, DEEP_COLD_OCEAN, FROZEN_OCEAN, DEEP_FROZEN_OCEAN,
    LUKEWARM_OCEAN, DEEP_LUKEWARM_OCEAN, WARM_OCEAN
};

bool isOcean(int b) { return b >= OCEAN; }
bool isSnowy(int b) {
    return b == SNOWY_PLAINS || b == SNOWY_TAIGA || b == ICE_SPIKES || b == GROVE || b == SNOWY_SLOPES || b == JAGGED_PEAKS ||
           b == FROZEN_PEAKS || b == SNOWY_BEACH || b == FROZEN_RIVER || b == FROZEN_OCEAN || b == DEEP_FROZEN_OCEAN;
}
bool isFrozenWater(int b) { return b == FROZEN_RIVER || b == FROZEN_OCEAN || b == DEEP_FROZEN_OCEAN || b == SNOWY_PLAINS || b == ICE_SPIKES; }

// ---- Generator --------------------------------------------------------------

struct Column {
    double height;      // Geländehöhe (Oberkante)
    double river;       // 0..1 wie stark ein Fluss hier ist
    double temp, humid, cont, eros, weird;
};

class Realistic : public Generator {
public:
    explicit Realistic(int64_t seed)
        : seed_(uint64_t(seed)), cont_(mix(seed_, 1), 5, 1.0 / 1600), eros_(mix(seed_, 2), 4, 1.0 / 900), weird_(mix(seed_, 3), 4, 1.0 / 650),
          temp_(mix(seed_, 4), 4, 1.0 / 1250), humid_(mix(seed_, 5), 4, 1.0 / 1000), detail_(mix(seed_, 6), 4, 1.0 / 130),
          river_(mix(seed_, 7), 3, 1.0 / 900), jag_(mix(seed_, 8), 3, 1.0 / 28), cheese_(mix(seed_, 9), 2, 1.0 / 90),
          spagA_(mix(seed_, 10), 2, 1.0 / 60), spagB_(mix(seed_, 11), 2, 1.0 / 60), patch_(mix(seed_, 12), 2, 1.0 / 24) {}

    Column column(double x, double z) const {
        Column c;
        c.cont = cont_.at2(x, z);
        c.eros = eros_.at2(x, z);
        c.weird = weird_.at2(x, z);
        c.temp = temp_.at2(x, z);
        c.humid = humid_.at2(x, z);
        double pv = 1 - std::abs(3 * std::abs(c.weird) - 2);   // Gipfel (+1) und Täler (-1)
        // Grundhöhe aus der Kontinentalität: Tiefsee, Ozean, Küste, Inland
        double base = spline({{-1.0, -48}, {-0.55, -38}, {-0.3, -18}, {-0.16, -6}, {-0.08, 0}, {0.0, 3}, {0.15, 8}, {0.4, 16}, {1.0, 26}}, c.cont);
        // Gebirge: weit im Inland und wenig Erosion
        double inland = smooth(-0.05, 0.35, c.cont);
        double rugged = 1 - smooth(-0.55, 0.35, c.eros);
        double m = inland * rugged;
        double peak = std::max(0.0, pv);
        double mountain = m * (18 + 150 * peak * peak) + m * peak * jag_.at2(x, z) * 10;
        double hills = detail_.at2(x, z) * (4 + 16 * (1 - smooth(-0.3, 0.6, c.eros))) * smooth(-0.15, 0.05, c.cont);
        double valley = std::max(0.0, -pv) * m * 14;
        double h = SEA_LEVEL + base + mountain + hills - valley;
        // Flüsse: schmale Bänder, die das Gelände bis unter den Meeresspiegel absenken
        double r = std::abs(river_.at2(x, z));
        double rf = (1 - smooth(0.0, 0.045, r)) * smooth(-0.2, -0.08, c.cont) * (1 - smooth(SEA_LEVEL + 40, SEA_LEVEL + 90, h));
        c.river = rf;
        if (rf > 0) h = lerpd(h, std::min(h, double(SEA_LEVEL - 3)), rf);
        c.height = std::max(double(MIN_Y + 8), std::min(double(MIN_Y + HEIGHT - 20), h));
        return c;
    }

    int biomeFor(const Column& c) const {
        double h = c.height, t = c.temp, hu = c.humid;
        if (c.river > 0.55 && c.cont > -0.2 && h < SEA_LEVEL + 1) return t < -0.45 ? FROZEN_RIVER : RIVER;
        if (h < SEA_LEVEL - 1) {
            bool deep = h < SEA_LEVEL - 18;
            if (t < -0.45) return deep ? DEEP_FROZEN_OCEAN : FROZEN_OCEAN;
            if (t < -0.15) return deep ? DEEP_COLD_OCEAN : COLD_OCEAN;
            if (t < 0.2) return deep ? DEEP_OCEAN : OCEAN;
            if (t < 0.5) return deep ? DEEP_LUKEWARM_OCEAN : LUKEWARM_OCEAN;
            return WARM_OCEAN;
        }
        if (h <= SEA_LEVEL + 2 && c.cont < 0.02) {
            if (c.eros < -0.45) return STONY_SHORE;
            return t < -0.45 ? SNOWY_BEACH : (t > 0.55 && hu < 0.0 ? DESERT : BEACH);
        }
        if (h > SEA_LEVEL + 110) {   // Gipfel: oberhalb der Schneegrenze verschneit, darunter grauer Fels
            if (t < -0.15) return FROZEN_PEAKS;
            if (h > SEA_LEVEL + 150 && t < 0.3) return JAGGED_PEAKS;
            return STONY_PEAKS;
        }
        if (h > SEA_LEVEL + 80) {
            if (t < -0.2) return hu > 0.2 ? GROVE : SNOWY_SLOPES;
            if (t < 0.25) return hu > 0.0 && t > 0.05 ? CHERRY_GROVE : MEADOW;
            return c.eros < -0.3 ? WINDSWEPT_SAVANNA : MEADOW;
        }
        if (c.eros > 0.45 && h < SEA_LEVEL + 12 && hu > 0.25 && t > -0.25) return t > 0.4 ? MANGROVE_SWAMP : SWAMP;
        if (t < -0.45) {
            if (hu < -0.35 && c.weird > 0.6) return ICE_SPIKES;
            return hu > 0.2 ? SNOWY_TAIGA : SNOWY_PLAINS;
        }
        if (t < -0.15) {
            if (c.eros < -0.35 && h > SEA_LEVEL + 30) return WINDSWEPT_HILLS;
            if (hu < -0.35) return PLAINS;
            return hu > 0.45 ? OLD_TAIGA : TAIGA;
        }
        if (t < 0.2) {
            if (hu < -0.35) return c.weird > 0.5 ? SUNFLOWER_PLAINS : PLAINS;
            if (hu < -0.1) return PLAINS;
            if (hu < 0.1) return c.weird > 0.55 ? FLOWER_FOREST : FOREST;
            if (hu < 0.3) return BIRCH_FOREST;
            return DARK_FOREST;
        }
        if (t < 0.55) {
            if (hu < -0.1) return c.eros < -0.4 ? WINDSWEPT_SAVANNA : SAVANNA;
            if (hu < 0.3) return hu < 0.1 ? PLAINS : FOREST;
            return hu < 0.45 ? SPARSE_JUNGLE : JUNGLE;
        }
        if (hu < 0.2) {
            if (c.eros < -0.2 && c.cont > 0.15) return c.eros < -0.5 ? ERODED_BADLANDS : BADLANDS;
            return DESERT;
        }
        return JUNGLE;
    }

    void generate(Chunk& ch) override {
        const Blocks& b = B();
        int bx = ch.x * 16, bz = ch.z * 16;
        // Säulen mit Rand (für Steilheit)
        static thread_local std::array<Column, 18 * 18> cols;
        for (int z = -1; z <= 16; ++z)
            for (int x = -1; x <= 16; ++x) cols[size_t((z + 1) * 18 + x + 1)] = column(bx + x, bz + z);
        auto col = [&](int x, int z) -> const Column& { return cols[size_t((z + 1) * 18 + x + 1)]; };
        // Biome je 4x4-Feld (wie Vanilla)
        int biome[16];
        for (int cz = 0; cz < 4; ++cz)
            for (int cx = 0; cx < 4; ++cx) {
                int b2 = biomeFor(col(cx * 4 + 2, cz * 4 + 2));
                biome[cz * 4 + cx] = b2;
                ch.biomes[size_t(cz * 4 + cx)] = uint8_t(b2);
            }
        std::mt19937_64 rng(mix(seed_, mix(uint64_t(int64_t(ch.x)), uint64_t(int64_t(ch.z)))));
        auto rnd = [&]() { return std::uniform_real_distribution<double>(0, 1)(rng); };

        int heights[256];
        int maxH = SEA_LEVEL;
        for (int z = 0; z < 16; ++z)
            for (int x = 0; x < 16; ++x) {
                heights[z * 16 + x] = int(std::floor(col(x, z).height));
                maxH = std::max(maxH, heights[z * 16 + x]);
            }
        // Grundgelände
        for (int si = 0; si <= std::min(SECTIONS - 1, (maxH + 2 - MIN_Y) >> 4); ++si)
            ch.sections[size_t(si)] = std::make_unique<std::vector<uint16_t>>(4096, uint16_t(b.air));
        for (int z = 0; z < 16; ++z)
            for (int x = 0; x < 16; ++x) {
                int h = heights[z * 16 + x];
                int bi = biome[(z >> 2) * 4 + (x >> 2)];
                double steep = std::max({std::abs(col(x + 1, z).height - col(x - 1, z).height), std::abs(col(x, z + 1).height - col(x, z - 1).height)});
                int top, under, underDepth = 3;
                surfaceFor(bi, h, steep, bx + x, bz + z, top, under, underDepth);
                for (int y = MIN_Y; y <= std::max(h, SEA_LEVEL); ++y) {
                    int st;
                    if (y <= h) {
                        if (y == h) st = top;
                        else if (y >= h - underDepth) st = under;
                        else if (bi == BADLANDS || bi == ERODED_BADLANDS) st = y > SEA_LEVEL - 10 ? b.terracotta[size_t(((y % 7) + 7) % 7)] : b.stone;
                        else st = b.stone;
                    } else {
                        bool frozen = y == SEA_LEVEL && isFrozenWater(bi) &&
                                      ((bi != FROZEN_OCEAN && bi != DEEP_FROZEN_OCEAN) || patch_.at2((bx + x) * 0.5, (bz + z) * 0.5) > -0.35);   // Eisschollen
                        st = frozen ? b.ice : b.water;
                    }
                    if (st == b.stone && y < 8) {   // Tiefenschiefer unten, mit Übergang
                        if (y < 0 || rnd() < (8 - y) / 8.0) st = b.deepslate;
                    }
                    if (y == MIN_Y || (y < MIN_Y + 5 && rnd() < (MIN_Y + 5 - y) / 5.0)) st = b.bedrock;
                    (*ch.sections[size_t((y - MIN_Y) >> 4)])[size_t((((y - MIN_Y) & 15) << 8) | (z << 4) | x)] = uint16_t(st);
                }
                // Schnee auf kalten Böden
                if (isSnowy(bi) && h >= SEA_LEVEL && top != b.ice && !isOcean(bi) && bi != FROZEN_RIVER) ch.set(x, h + 1, z, b.snowLayer);
                // Eiszacken
                if (bi == ICE_SPIKES && h > SEA_LEVEL && rnd() < 0.002) {
                    int len = 6 + int(rnd() * 20);
                    for (int y = 1; y <= len; ++y) ch.set(x, h + y, z, b.packedIce);
                }
            }
        carveCaves(ch, heights, biome);
        placeOres(ch, rng, biome);
        decorate(ch, heights, biome, rng);
        for (int i = 0; i < 256; ++i) {   // Höhenkarte neu berechnen
            int x = i & 15, z = i >> 4, y = MIN_Y + HEIGHT - 1;
            while (y > MIN_Y && ch.get(x, y, z) == b.air) --y;
            ch.heights[size_t(i)] = y;
        }
    }

    bool land(int x, int z, int& h) const {
        Column c = column(x, z);
        h = int(std::floor(c.height));
        return c.height > SEA_LEVEL + 1 && c.river < 0.3;
    }

private:
    uint64_t seed_;
    Octaves cont_, eros_, weird_, temp_, humid_, detail_, river_, jag_, cheese_, spagA_, spagB_, patch_;

    void surfaceFor(int bi, int h, double steep, int x, int z, int& top, int& under, int& depth) const {
        const Blocks& b = B();
        bool wet = h < SEA_LEVEL;
        double pn = patch_.at2(x, z);
        depth = 3;
        top = b.grass;
        under = b.dirt;
        if (wet) {   // Meeres- und Flussboden
            if (bi == RIVER || bi == FROZEN_RIVER) top = pn > 0.2 ? b.gravel : b.sand;
            else if (bi == SWAMP || bi == MANGROVE_SWAMP) top = b.mud;
            else top = pn > 0.35 ? b.gravel : pn < -0.45 ? b.clay : b.sand;
            under = top == b.mud ? b.dirt : top;
            return;
        }
        switch (bi) {
            case DESERT:
                top = b.sand;
                under = b.sand;
                depth = 4;
                break;
            case BEACH:
            case SNOWY_BEACH:
                top = under = b.sand;
                break;
            case STONY_SHORE:
                top = under = b.stone;
                break;
            case STONY_PEAKS:   // grauer Fels mit Kalzit-Bändern
                top = under = std::abs(std::sin(h * 0.9 + pn * 3)) > 0.93 ? b.calcite : b.stone;
                break;
            case BADLANDS:
            case ERODED_BADLANDS:
                top = b.redSand;
                under = b.terracotta[0];
                depth = 1;
                break;
            case JAGGED_PEAKS:
            case FROZEN_PEAKS:
            case SNOWY_SLOPES:
                top = steep > 4 ? b.stone : b.snowBlock;
                under = steep > 4 ? b.stone : b.snowBlock;
                if (bi == FROZEN_PEAKS && pn > 0.4) top = b.packedIce;
                depth = 2;
                break;
            case SNOWY_PLAINS:
            case SNOWY_TAIGA:
            case ICE_SPIKES:
            case GROVE:
                top = b.snowyGrass;
                break;
            case OLD_TAIGA:
                top = pn > 0.25 ? b.podzol : pn < -0.4 ? b.coarse : b.grass;
                break;
            case SAVANNA:
            case WINDSWEPT_SAVANNA:
                top = pn > 0.45 ? b.coarse : b.grass;
                break;
            case WINDSWEPT_HILLS:
                top = steep > 3 || pn > 0.5 ? b.stone : b.grass;
                break;
            default:
                break;
        }
        if (steep > 6 && top != b.sand && top != b.redSand && top != b.snowBlock) top = under = b.stone;   // Felswände
        if (top == b.sand && bi == DESERT) under = b.sand;
    }

    void carveCaves(Chunk& ch, const int* heights, const int* biome) {
        const Blocks& b = B();
        int bx = ch.x * 16, bz = ch.z * 16;
        // Rauschen auf einem groben Gitter (alle 4 Blöcke waagerecht, 8 senkrecht), dazwischen interpoliert
        constexpr int NY = HEIGHT / 8 + 1;
        static thread_local std::array<float, 5 * 5 * NY> cheese, spag;
        for (int gz = 0; gz < 5; ++gz)
            for (int gx = 0; gx < 5; ++gx)
                for (int gy = 0; gy < NY; ++gy) {
                    double x = bx + gx * 4, z = bz + gz * 4, y = MIN_Y + gy * 8;
                    size_t i = size_t((gz * 5 + gx) * NY + gy);
                    cheese[i] = float(cheese_.at3(x, y, z, 1.8));
                    double a = spagA_.at3(x, y, z, 0.7), c = spagB_.at3(x, y, z, 0.7);   // flach: lange, eher waagerechte Gänge
                    spag[i] = float(a * a + c * c);
                }
        auto sample = [&](const std::array<float, 5 * 5 * NY>& g, int x, int y, int z) {
            double fx = x / 4.0, fz = z / 4.0, fy = (y - MIN_Y) / 8.0;
            int x0 = std::min(3, int(fx)), z0 = std::min(3, int(fz)), y0 = std::min(NY - 2, int(fy));
            double tx = fx - x0, tz = fz - z0, ty = fy - y0;
            auto G = [&](int a, int bb, int c) { return double(g[size_t(((z0 + c) * 5 + x0 + a) * NY + y0 + bb)]); };
            double c00 = lerpd(G(0, 0, 0), G(1, 0, 0), tx), c10 = lerpd(G(0, 0, 1), G(1, 0, 1), tx);
            double c01 = lerpd(G(0, 1, 0), G(1, 1, 0), tx), c11 = lerpd(G(0, 1, 1), G(1, 1, 1), tx);
            return lerpd(lerpd(c00, c10, tz), lerpd(c01, c11, tz), ty);
        };
        for (int z = 0; z < 16; ++z)
            for (int x = 0; x < 16; ++x) {
                int h = heights[z * 16 + x];
                int bi = biome[(z >> 2) * 4 + (x >> 2)];
                bool underwater = h < SEA_LEVEL || isOcean(bi) || bi == RIVER || bi == FROZEN_RIVER;
                int maxY = underwater ? h - 6 : h;
                for (int y = MIN_Y + 4; y <= maxY; ++y) {
                    // Käsehöhlen: große Räume tief unten, weiter oben seltener
                    double depthBoost = smooth(SEA_LEVEL, double(MIN_Y + 20), y) * 0.12;
                    bool open = sample(cheese, x, y, z) > 0.38 - depthBoost;
                    // Spaghetti-Höhlen: lange Gänge, wo beide Rauschwerte nahe 0 sind
                    if (!open) open = sample(spag, x, y, z) < SPAG;
                    if (!open) continue;
                    if (y > h - 3 && !(sample(spag, x, y, z) < SPAG && h > SEA_LEVEL + 3)) continue;   // Oberfläche nur an Höhleneingängen öffnen
                    int cur = ch.get(x, y, z);
                    if (cur == b.water || cur == b.bedrock || cur == b.ice) continue;
                    ch.set(x, y, z, y < -54 ? b.lava : b.air);   // tief unten: Lavaseen wie in Vanilla
                }
            }
    }

    struct Ore {
        int normal, deep;
        int attempts, size;
        int minY, maxY;
        bool triangle;
    };

    void placeOres(Chunk& ch, std::mt19937_64& rng, const int* biome) {
        const Blocks& b = B();
        bool mountains = false;
        for (int i = 0; i < 16; ++i)
            if (biome[i] == JAGGED_PEAKS || biome[i] == FROZEN_PEAKS || biome[i] == STONY_PEAKS || biome[i] == SNOWY_SLOPES ||
                biome[i] == WINDSWEPT_HILLS || biome[i] == MEADOW || biome[i] == GROVE)
                mountains = true;
        // Verteilung wie Vanilla (vereinfacht): Versuche pro Chunk, Größe, Höhenbereich, Dreieck = Häufung in der Mitte
        std::vector<Ore> ores = {
            {b.coal, b.dCoal, 20, 17, 0, 192, true},          {b.coal, b.dCoal, 10, 17, 136, 320, false},
            {b.iron, b.dIron, 10, 9, -24, 56, true},          {b.iron, b.dIron, 10, 4, -64, 72, false},
            {b.copper, b.dCopper, 16, 10, -16, 112, true},    {b.gold, b.dGold, 4, 9, -64, 32, true},
            {b.redstone, b.dRedstone, 4, 8, -64, 15, false},  {b.redstone, b.dRedstone, 8, 8, -96, -32, true},
            {b.diamond, b.dDiamond, 7, 4, -144, 16, true},    {b.diamond, b.dDiamond, 1, 12, -144, 16, true},
            {b.lapis, b.dLapis, 2, 7, -32, 32, true},         {b.lapis, b.dLapis, 4, 7, -64, 64, false},
            {b.granite, b.granite, 2, 48, 0, 60, false},      {b.diorite, b.diorite, 2, 48, 0, 60, false},
            {b.andesite, b.andesite, 2, 48, 0, 60, false},    {b.tuff, b.tuff, 2, 48, -64, 0, false},
            {b.dirt, b.dirt, 7, 33, 0, 160, false},           {b.gravel, b.gravel, 10, 33, -64, 320, false},
        };
        if (mountains) {
            ores.push_back({b.iron, b.dIron, 40, 9, 80, 384, true});
            ores.push_back({b.emerald, b.dEmerald, 20, 1, -16, 320, true});
        }
        std::uniform_int_distribution<int> lx(0, 15);
        std::uniform_real_distribution<double> u(0, 1);
        for (const Ore& o : ores)
            for (int a = 0; a < o.attempts; ++a) {
                int y = o.triangle ? int((o.minY + (o.maxY - o.minY) * u(rng) + o.minY + (o.maxY - o.minY) * u(rng)) / 2)
                                   : o.minY + int((o.maxY - o.minY) * u(rng));
                if (y < MIN_Y + 1 || y > MIN_Y + HEIGHT - 2) continue;
                int cx = lx(rng), cz = lx(rng);
                double r = std::cbrt(double(o.size)) * 0.7;
                int placed = 0;
                for (int t = 0; t < o.size * 3 && placed < o.size; ++t) {
                    int x = cx + int(std::round((u(rng) * 2 - 1) * r)), yy = y + int(std::round((u(rng) * 2 - 1) * r)),
                        z = cz + int(std::round((u(rng) * 2 - 1) * r));
                    if (x < 0 || x > 15 || z < 0 || z > 15) continue;
                    int cur = ch.get(x, yy, z);
                    if (cur == b.stone || cur == b.granite || cur == b.diorite || cur == b.andesite) ch.set(x, yy, z, o.normal);
                    else if (cur == b.deepslate || cur == b.tuff) ch.set(x, yy, z, o.deep);
                    else continue;
                    ++placed;
                }
            }
    }

    // ---- Bäume und Pflanzen ----

    struct TreeSpot {
        int x, z;   // Weltkoordinaten
        int kind;   // 0 Eiche, 1 Birke, 2 Fichte, 3 Dschungel, 4 Akazie, 5 Schwarzeiche, 6 Kirsche, 7 große Eiche
        uint64_t r;
    };

    int treeDensity(int bi) const {   // Bäume pro Chunk (ungefähr)
        switch (bi) {
            case FOREST: case BIRCH_FOREST: case FLOWER_FOREST: return 9;
            case DARK_FOREST: return 14;
            case TAIGA: case SNOWY_TAIGA: case OLD_TAIGA: case GROVE: return 9;
            case JUNGLE: return 18;
            case SPARSE_JUNGLE: return 4;
            case SWAMP: case MANGROVE_SWAMP: return 3;
            case SAVANNA: case WINDSWEPT_SAVANNA: return 2;
            case CHERRY_GROVE: return 6;
            case PLAINS: case SUNFLOWER_PLAINS: case MEADOW: return 0;
            case WINDSWEPT_HILLS: return 2;
            case SNOWY_PLAINS: return 0;
            default: return 0;
        }
    }

    int treeKind(int bi, double v) const {
        switch (bi) {
            case BIRCH_FOREST: return 1;
            case FOREST: case FLOWER_FOREST: return v < 0.2 ? 1 : v < 0.3 ? 7 : 0;
            case DARK_FOREST: return v < 0.75 ? 5 : 0;
            case TAIGA: case SNOWY_TAIGA: case OLD_TAIGA: case GROVE: case WINDSWEPT_HILLS: return 2;
            case JUNGLE: case SPARSE_JUNGLE: return v < 0.6 ? 3 : 0;
            case SAVANNA: case WINDSWEPT_SAVANNA: return 4;
            case CHERRY_GROVE: return 6;
            default: return 0;
        }
    }

    std::vector<TreeSpot> treesOf(int cx, int cz) const {
        std::vector<TreeSpot> out;
        std::mt19937_64 r(mix(seed_ ^ 0x7EE5ULL, mix(uint64_t(int64_t(cx)), uint64_t(int64_t(cz)))));
        std::uniform_real_distribution<double> u(0, 1);
        Column c = column(cx * 16 + 8, cz * 16 + 8);
        int bi = biomeFor(c);
        int n = treeDensity(bi);
        if (bi == PLAINS || bi == SUNFLOWER_PLAINS || bi == MEADOW) n = u(r) < 0.08 ? 1 : 0;   // vereinzelte Bäume
        if (bi == SNOWY_PLAINS) n = u(r) < 0.05 ? 1 : 0;
        double nd = n * (0.7 + u(r) * 0.6);
        for (int i = 0; i < int(nd + 0.5); ++i) {
            TreeSpot t{cx * 16 + int(u(r) * 16), cz * 16 + int(u(r) * 16), 0, r()};
            Column tc = column(t.x, t.z);
            int tb = biomeFor(tc);
            t.kind = treeKind(tb, u(r));
            if (tc.height <= SEA_LEVEL && tb != SWAMP && tb != MANGROVE_SWAMP) continue;
            if (tb == DESERT || tb == BEACH || isOcean(tb) || tb == RIVER || (tb >= JAGGED_PEAKS && tb <= STONY_PEAKS)) continue;
            out.push_back(t);
        }
        return out;
    }

    void setIn(Chunk& ch, int wx, int y, int wz, int state, bool onlyAir = true) {
        int lx = wx - ch.x * 16, lz = wz - ch.z * 16;
        if (lx < 0 || lx > 15 || lz < 0 || lz > 15 || y <= MIN_Y || y >= MIN_Y + HEIGHT) return;
        if (onlyAir) {
            int cur = ch.get(lx, y, lz);
            const Blocks& b = B();
            if (cur != b.air && cur != b.snowLayer && cur != b.shortGrass && cur != b.fern) return;
        }
        ch.set(lx, y, lz, state);
    }

    void blob(Chunk& ch, int x, int y, int z, int rx, int ry, int rz, int leaves, std::mt19937_64& r, double holes = 0.25) {
        std::uniform_real_distribution<double> u(0, 1);
        for (int dy = -ry; dy <= ry; ++dy)
            for (int dx = -rx; dx <= rx; ++dx)
                for (int dz = -rz; dz <= rz; ++dz) {
                    double d = double(dx * dx) / (rx * rx + 0.5) + double(dy * dy) / (ry * ry + 0.5) + double(dz * dz) / (rz * rz + 0.5);
                    if (d > 1.0 || (d > 0.7 && u(r) < holes)) continue;
                    setIn(ch, x + dx, y + dy, z + dz, leaves);
                }
    }

    void placeTree(Chunk& ch, const TreeSpot& t) {
        std::mt19937_64 r(t.r);
        std::uniform_real_distribution<double> u(0, 1);
        int base = int(std::floor(column(t.x, t.z).height)) + 1;
        auto log = [&](const char* n) { return SP(n, {{"axis", "y"}}); };
        auto leaf = [&](const char* n) { return SP(n, {{"persistent", "false"}, {"distance", "1"}}); };
        auto trunk = [&](int x, int z, int from, int to, int st) {
            for (int y = from; y <= to; ++y) setIn(ch, x, y, z, st, false);
        };
        switch (t.kind) {
            case 1: {   // Birke
                int hgt = 5 + int(u(r) * 3);
                blob(ch, t.x, base + hgt - 1, t.z, 2, 2, 2, leaf("birch_leaves"), r);
                trunk(t.x, t.z, base, base + hgt - 1, log("birch_log"));
                break;
            }
            case 2: {   // Fichte: Kegel aus Ringen
                int hgt = 7 + int(u(r) * 5);
                int lv = leaf("spruce_leaves");
                int top = base + hgt;
                setIn(ch, t.x, top, t.z, lv);
                for (int y = top - 1, ring = 0; y > base + 2; --y, ++ring) {
                    int rad = (ring % 2 == 0) ? std::min(3, 1 + ring / 3) : std::max(0, std::min(3, ring / 3));
                    for (int dx = -rad; dx <= rad; ++dx)
                        for (int dz = -rad; dz <= rad; ++dz)
                            if (std::abs(dx) + std::abs(dz) <= rad + 1 && !(std::abs(dx) == rad && std::abs(dz) == rad && rad > 1))
                                setIn(ch, t.x + dx, y, t.z + dz, lv);
                }
                trunk(t.x, t.z, base, top - 1, log("spruce_log"));
                break;
            }
            case 3: {   // Dschungelbaum, manchmal riesig (2x2)
                bool mega = u(r) < 0.15;
                int hgt = mega ? 18 + int(u(r) * 12) : 6 + int(u(r) * 6);
                int lv = leaf("jungle_leaves"), lg = log("jungle_log");
                blob(ch, t.x, base + hgt, t.z, mega ? 5 : 3, 2, mega ? 5 : 3, lv, r);
                trunk(t.x, t.z, base, base + hgt, lg);
                if (mega) {
                    trunk(t.x + 1, t.z, base, base + hgt, lg);
                    trunk(t.x, t.z + 1, base, base + hgt, lg);
                    trunk(t.x + 1, t.z + 1, base, base + hgt, lg);
                    for (int k = 0; k < 3; ++k)   // Nebenkronen
                        blob(ch, t.x + int(u(r) * 8) - 4, base + hgt / 2 + int(u(r) * hgt / 3), t.z + int(u(r) * 8) - 4, 2, 1, 2, lv, r);
                }
                break;
            }
            case 4: {   // Akazie: schräger Stamm, flache Krone
                int hgt = 5 + int(u(r) * 2);
                int lg = log("acacia_log"), lv = leaf("acacia_leaves");
                int dx = u(r) < 0.5 ? 1 : -1, dz = u(r) < 0.5 ? 1 : -1;
                int x = t.x, z = t.z;
                for (int y = base; y < base + hgt; ++y) {
                    if (y - base > 2 && u(r) < 0.5) {
                        x += dx;
                        z += dz;
                    }
                    setIn(ch, x, y, z, lg, false);
                }
                int top = base + hgt;
                for (int ddx = -3; ddx <= 3; ++ddx)
                    for (int ddz = -3; ddz <= 3; ++ddz)
                        if (std::abs(ddx) + std::abs(ddz) <= 4) setIn(ch, x + ddx, top, z + ddz, lv);
                for (int ddx = -1; ddx <= 1; ++ddx)
                    for (int ddz = -1; ddz <= 1; ++ddz) setIn(ch, x + ddx, top + 1, z + ddz, lv);
                break;
            }
            case 5: {   // Schwarzeiche 2x2
                int hgt = 6 + int(u(r) * 3);
                int lg = log("dark_oak_log"), lv = leaf("dark_oak_leaves");
                blob(ch, t.x, base + hgt, t.z, 4, 2, 4, lv, r, 0.15);
                for (int a = 0; a < 2; ++a)
                    for (int c2 = 0; c2 < 2; ++c2) trunk(t.x + a, t.z + c2, base, base + hgt, lg);
                break;
            }
            case 6: {   // Kirschbaum
                int hgt = 5 + int(u(r) * 3);
                int lg = data::blockState("cherry_log") >= 0 ? log("cherry_log") : log("oak_log");
                int lv = data::blockState("cherry_leaves") >= 0 ? leaf("cherry_leaves") : leaf("oak_leaves");
                blob(ch, t.x, base + hgt, t.z, 4, 2, 4, lv, r, 0.3);
                trunk(t.x, t.z, base, base + hgt, lg);
                break;
            }
            case 7: {   // große Eiche
                int hgt = 7 + int(u(r) * 5);
                int lg = log("oak_log"), lv = leaf("oak_leaves");
                blob(ch, t.x, base + hgt - 1, t.z, 3, 3, 3, lv, r);
                for (int k = 0; k < 3; ++k) {
                    int ox = int(u(r) * 6) - 3, oz = int(u(r) * 6) - 3, oy = base + hgt / 2 + int(u(r) * hgt / 2);
                    blob(ch, t.x + ox, oy, t.z + oz, 2, 1, 2, lv, r);
                    setIn(ch, t.x + ox / 2, oy - 1, t.z + oz / 2, lg, false);
                }
                trunk(t.x, t.z, base, base + hgt - 1, lg);
                break;
            }
            default: {   // Eiche
                int hgt = 4 + int(u(r) * 3);
                int lv = leaf("oak_leaves");
                int top = base + hgt;
                for (int dy = -2; dy < 2; ++dy) {
                    int rad = dy < 0 ? 2 : 1;
                    for (int dx = -rad; dx <= rad; ++dx)
                        for (int dz = -rad; dz <= rad; ++dz) {
                            if (std::abs(dx) == rad && std::abs(dz) == rad && (dy == 1 || u(r) < 0.5)) continue;
                            setIn(ch, t.x + dx, top + dy, t.z + dz, lv);
                        }
                }
                trunk(t.x, t.z, base, top - 1, log("oak_log"));
                break;
            }
        }
        // unter dem Stamm Erde statt Gras
        int lx = t.x - ch.x * 16, lz = t.z - ch.z * 16;
        if (lx >= 0 && lx <= 15 && lz >= 0 && lz <= 15 && ch.get(lx, base - 1, lz) == B().grass) ch.set(lx, base - 1, lz, B().dirt);
    }

    void decorate(Chunk& ch, const int* heights, const int* biome, std::mt19937_64& rng) {
        const Blocks& b = B();
        std::uniform_real_distribution<double> u(0, 1);
        // Bäume aus diesem und den Nachbar-Chunks (Kronen ragen über die Grenze)
        for (int dz = -1; dz <= 1; ++dz)
            for (int dx = -1; dx <= 1; ++dx)
                for (const TreeSpot& t : treesOf(ch.x + dx, ch.z + dz)) {
                    int lx = t.x - ch.x * 16, lz = t.z - ch.z * 16;
                    if (dx == 0 && dz == 0) {   // Stamm nur auf passendem Boden
                        int top = ch.get(lx, heights[lz * 16 + lx], lz);
                        if (top != b.grass && top != b.snowyGrass && top != b.podzol && top != b.coarse && top != b.dirt && top != b.mud) continue;
                    } else {
                        int h;
                        if (!land(t.x, t.z, h) && biomeFor(column(t.x, t.z)) != SWAMP) continue;
                    }
                    placeTree(ch, t);
                }
        // Pflanzen
        for (int z = 0; z < 16; ++z)
            for (int x = 0; x < 16; ++x) {
                int h = heights[z * 16 + x];
                int bi = biome[(z >> 2) * 4 + (x >> 2)];
                int top = ch.get(x, h, z);
                int above = ch.get(x, h + 1, z);
                double v = u(rng);
                if (h < SEA_LEVEL) {   // unter Wasser: Seegras und Seetang
                    if (ch.get(x, h + 1, z) != b.water || top == b.ice) continue;
                    bool kelpy = bi == OCEAN || bi == DEEP_OCEAN || bi == COLD_OCEAN || bi == DEEP_COLD_OCEAN || bi == LUKEWARM_OCEAN || bi == DEEP_LUKEWARM_OCEAN;
                    if (kelpy && v < 0.06 && SEA_LEVEL - h > 4) {
                        int len = std::min(SEA_LEVEL - h - 2, 4 + int(u(rng) * 12));
                        for (int y = 1; y < len; ++y) ch.set(x, h + y, z, b.kelpPlant);
                        ch.set(x, h + len, z, b.kelp);
                    } else if (v < (bi == WARM_OCEAN || bi == RIVER ? 0.25 : 0.15)) {
                        ch.set(x, h + 1, z, b.seagrass);
                    }
                    continue;
                }
                if (bi == SWAMP || bi == MANGROVE_SWAMP) {   // Seerosen auf flachem Wasser
                    if (top == b.water && ch.get(x, h + 1, z) == b.air && v < 0.04) ch.set(x, h + 1, z, b.lilyPad);
                }
                if (above != b.air) continue;
                bool grassy = top == b.grass || top == b.podzol || top == b.coarse;
                // Zuckerrohr am Wasser
                if ((grassy || top == b.sand) && h == SEA_LEVEL && v < 0.03) {
                    bool nearWater = false;
                    for (auto [ox, oz] : {std::pair{1, 0}, {-1, 0}, {0, 1}, {0, -1}})
                        if (x + ox >= 0 && x + ox < 16 && z + oz >= 0 && z + oz < 16 && ch.get(x + ox, h, z + oz) == b.water) nearWater = true;
                    if (nearWater) {
                        int len = 1 + int(u(rng) * 3);
                        for (int y = 1; y <= len; ++y) ch.set(x, h + y, z, b.sugarCane);
                        continue;
                    }
                }
                if (top == b.sand || top == b.redSand) {
                    if ((bi == DESERT || bi == BADLANDS || bi == ERODED_BADLANDS) && v < 0.008) ch.set(x, h + 1, z, b.deadBush);
                    else if (bi == DESERT && v < 0.012 && x > 0 && x < 15 && z > 0 && z < 15) {
                        int len = 1 + int(u(rng) * 3);
                        for (int y = 1; y <= len; ++y) ch.set(x, h + y, z, b.cactus);
                    }
                    continue;
                }
                if (!grassy) continue;
                auto tall = [&](int low, int up) {
                    ch.set(x, h + 1, z, low);
                    ch.set(x, h + 2, z, up);
                };
                switch (bi) {
                    case PLAINS:
                    case SUNFLOWER_PLAINS:
                    case MEADOW:
                        if (v < 0.25) ch.set(x, h + 1, z, b.shortGrass);
                        else if (v < 0.28) tall(b.tallLow, b.tallUp);
                        else if (bi == SUNFLOWER_PLAINS && v < 0.31) tall(b.sunflowerLow, b.sunflowerUp);
                        else if (v < (bi == MEADOW ? 0.36 : 0.30)) ch.set(x, h + 1, z, b.flowers[size_t(u(rng) * b.flowers.size())]);
                        else if (v < 0.3005) ch.set(x, h, z, b.dirt), ch.set(x, h + 1, z, b.pumpkin);
                        break;
                    case FLOWER_FOREST:
                        if (v < 0.25) ch.set(x, h + 1, z, b.forestFlowers[size_t(u(rng) * b.forestFlowers.size())]);
                        else if (v < 0.35) ch.set(x, h + 1, z, b.shortGrass);
                        break;
                    case FOREST:
                    case BIRCH_FOREST:
                    case CHERRY_GROVE:
                        if (v < 0.12) ch.set(x, h + 1, z, b.shortGrass);
                        else if (v < 0.135) ch.set(x, h + 1, z, b.forestFlowers[size_t(u(rng) * b.forestFlowers.size())]);
                        break;
                    case DARK_FOREST:
                        if (v < 0.08) ch.set(x, h + 1, z, b.shortGrass);
                        else if (v < 0.11) ch.set(x, h + 1, z, u(rng) < 0.5 ? b.brownMushroom : b.redMushroom);
                        break;
                    case TAIGA:
                    case OLD_TAIGA:
                    case SNOWY_TAIGA:
                    case GROVE:
                        if (bi == SNOWY_TAIGA || bi == GROVE) break;
                        if (v < 0.15) ch.set(x, h + 1, z, b.fern);
                        else if (v < 0.17) tall(b.largeFernLow, b.largeFernUp);
                        else if (v < 0.175) ch.set(x, h + 1, z, b.sweetBerry);
                        else if (v < 0.25) ch.set(x, h + 1, z, b.shortGrass);
                        break;
                    case SAVANNA:
                    case WINDSWEPT_SAVANNA:
                        if (v < 0.35) ch.set(x, h + 1, z, b.shortGrass);
                        else if (v < 0.4) tall(b.tallLow, b.tallUp);
                        break;
                    case JUNGLE:
                    case SPARSE_JUNGLE:
                        if (v < 0.2) ch.set(x, h + 1, z, b.shortGrass);
                        else if (v < 0.3) ch.set(x, h + 1, z, b.fern);
                        else if (v < 0.305) ch.set(x, h + 1, z, b.melon);
                        break;
                    case SWAMP:
                    case MANGROVE_SWAMP:
                        if (v < 0.15) ch.set(x, h + 1, z, b.shortGrass);
                        else if (v < 0.16) ch.set(x, h + 1, z, b.swampFlowers[0]);
                        break;
                    case WINDSWEPT_HILLS:
                        if (v < 0.1) ch.set(x, h + 1, z, b.shortGrass);
                        break;
                    default:
                        break;
                }
            }
    }
};

}  // namespace

const std::vector<std::string>& biomeNames() { return BIOMES; }

int biomeIndex(const std::string& name) {
    auto it = std::find(BIOMES.begin(), BIOMES.end(), removePrefix(name, "minecraft:"));
    return it == BIOMES.end() ? -1 : int(it - BIOMES.begin());
}

std::unique_ptr<Generator> createRealistic(int64_t seed) { return std::make_unique<Realistic>(seed); }

bool isLand(Generator& g, int x, int z, int& height) {
    auto* r = dynamic_cast<Realistic*>(&g);
    if (!r) return false;
    return r->land(x, z, height);
}

}  // namespace worldgen
