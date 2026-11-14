#include "worldmap.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "data.hpp"
#include "util.hpp"
#include "world.hpp"

namespace worldmap {
namespace {

uint32_t crcTable(uint32_t n) {
    uint32_t c = n;
    for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
    return c;
}

uint32_t crc32(const std::string& s) {
    static std::array<uint32_t, 256> t = [] {
        std::array<uint32_t, 256> a{};
        for (uint32_t i = 0; i < 256; ++i) a[i] = crcTable(i);
        return a;
    }();
    uint32_t c = 0xFFFFFFFFu;
    for (unsigned char ch : s) c = t[(c ^ ch) & 255] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

void be32(std::string& o, uint32_t v) {
    for (int i = 3; i >= 0; --i) o += char((v >> (i * 8)) & 255);
}

void chunk(std::string& png, const char* type, const std::string& data) {
    be32(png, uint32_t(data.size()));
    std::string td = std::string(type) + data;
    png += td;
    be32(png, crc32(td));
}

// Unkomprimiertes PNG (zlib mit "stored"-Blöcken)
std::string encodePng(int w, int h, const std::vector<uint8_t>& rgb) {
    std::string raw;
    raw.reserve(size_t(h) * (size_t(w) * 3 + 1));
    for (int y = 0; y < h; ++y) {
        raw += '\0';
        raw.append(reinterpret_cast<const char*>(&rgb[size_t(y) * size_t(w) * 3]), size_t(w) * 3);
    }
    std::string z = "\x78\x01";
    size_t pos = 0;
    while (pos < raw.size() || raw.empty()) {
        size_t n = std::min<size_t>(65535, raw.size() - pos);
        bool last = pos + n >= raw.size();
        z += char(last ? 1 : 0);
        z += char(n & 255);
        z += char(n >> 8);
        z += char(~n & 255);
        z += char((~n >> 8) & 255);
        z.append(raw, pos, n);
        pos += n;
        if (raw.empty()) break;
    }
    uint32_t a = 1, b = 0;
    for (unsigned char c : raw) {
        a = (a + c) % 65521;
        b = (b + a) % 65521;
    }
    be32(z, (b << 16) | a);
    std::string png = "\x89PNG\r\n\x1a\n";
    std::string ihdr;
    be32(ihdr, uint32_t(w));
    be32(ihdr, uint32_t(h));
    ihdr += "\x08\x02\x00\x00\x00";
    chunk(png, "IHDR", ihdr);
    chunk(png, "IDAT", z);
    chunk(png, "IEND", "");
    return png;
}

uint32_t colorOf(const std::string& n) {
    static const std::unordered_map<std::string, uint32_t> C = {
        {"grass_block", 0x6A9A3A}, {"dirt", 0x8B6440}, {"coarse_dirt", 0x7A5A3A}, {"podzol", 0x6B4A25}, {"sand", 0xDBCF8E},
        {"red_sand", 0xBE6621}, {"sandstone", 0xD8CB8D}, {"gravel", 0x837E7C}, {"stone", 0x7D7D7D}, {"snow", 0xF5FAFA},
        {"snow_block", 0xF0F6F6}, {"ice", 0x9CC0F5}, {"packed_ice", 0x8DB4EB}, {"water", 0x3355CC}, {"lava", 0xD45A12},
        {"oak_leaves", 0x3E7A22}, {"birch_leaves", 0x5E8E3A}, {"spruce_leaves", 0x3B5E3B}, {"jungle_leaves", 0x2E8A18},
        {"acacia_leaves", 0x4E8A20}, {"dark_oak_leaves", 0x2E5E18}, {"cherry_leaves", 0xF0AAC8}, {"mud", 0x3C3A3D},
        {"clay", 0xA0A6B3}, {"terracotta", 0x985E43}, {"orange_terracotta", 0xA15325}, {"yellow_terracotta", 0xBA8523},
        {"white_terracotta", 0xD1B2A1}, {"red_terracotta", 0x8F3D2E}, {"brown_terracotta", 0x4D3323}, {"light_gray_terracotta", 0x876B62},
        {"cactus", 0x0F7A2A}, {"short_grass", 0x6A9A3A}, {"tall_grass", 0x6A9A3A}, {"fern", 0x5A8A3A}, {"lily_pad", 0x208030},
        {"seagrass", 0x3355CC}, {"kelp", 0x3355CC}, {"kelp_plant", 0x3355CC}, {"deepslate", 0x505050}, {"bedrock", 0x333333},
        {"oak_log", 0x6B5333}, {"spruce_log", 0x3A2A1A}, {"birch_log", 0xD7D3C8}, {"jungle_log", 0x56441F}, {"acacia_log", 0x6A6460},
        {"dark_oak_log", 0x3B2B18}, {"calcite", 0xDFE0DC}, {"sugar_cane", 0x8AB863}, {"pumpkin", 0xE08A1D}, {"melon", 0x7EB02E},
    };
    auto it = C.find(removePrefix(n, "minecraft:"));
    if (it != C.end()) return it->second;
    if (endsWith(n, "_flower") || n.find("tulip") != std::string::npos || endsWith(n, "dandelion") || endsWith(n, "poppy")) return 0x6A9A3A;
    if (endsWith(n, "_leaves")) return 0x3E7A22;
    if (endsWith(n, "_log")) return 0x6B5333;
    if (endsWith(n, "_ore")) return 0x7D7D7D;
    return 0x9A9A9A;
}

}  // namespace

bool render(World& w, int cx, int cz, int size, const std::string& file, int sliceY) {
    size = std::max(16, std::min(4096, size));
    std::vector<uint8_t> rgb(size_t(size) * size_t(size) * 3);
    std::vector<int> hs(size_t(size) * size_t(size));
    int x0 = cx - size / 2, z0 = cz - size / 2;
    std::vector<uint32_t> cols(size_t(size) * size_t(size));
    for (int z = 0; z < size; ++z)
        for (int x = 0; x < size; ++x) {
            int wx = x0 + x, wz = z0 + z;
            if (sliceY != 99999) {   // Schnitt: Luft schwarz, Erze bunt
                int st = w.getBlock(wx, sliceY, wz);
                std::string n = data::blockName(st);
                uint32_t c = st == 0 ? 0x101010 : n == "minecraft:lava" ? 0xE05010 : n == "minecraft:water" ? 0x3355CC
                    : n.find("diamond") != std::string::npos ? 0x40E0E0 : n.find("gold") != std::string::npos ? 0xF0D020
                    : n.find("iron") != std::string::npos ? 0xD8A880 : n.find("coal") != std::string::npos ? 0x202020
                    : n.find("redstone") != std::string::npos ? 0xE01010 : n.find("lapis") != std::string::npos ? 0x2040C0
                    : n.find("copper") != std::string::npos ? 0xE07040 : n.find("emerald") != std::string::npos ? 0x20D040
                    : colorOf(n);
                cols[size_t(z) * size_t(size) + size_t(x)] = c;
                hs[size_t(z) * size_t(size) + size_t(x)] = sliceY;
                continue;
            }
            int y = w.highestBlock(wx, wz);
            int st = w.getBlock(wx, y, wz);
            std::string n = data::blockName(st);
            // dünne Pflanzen/Schnee durchsehen
            while ((n == "minecraft:short_grass" || n == "minecraft:snow" || n == "minecraft:tall_grass") && y > MIN_Y) {
                if (n == "minecraft:snow") break;
                st = w.getBlock(wx, --y, wz);
                n = data::blockName(st);
            }
            uint32_t c = colorOf(n);
            if (n == "minecraft:water") {   // tieferes Wasser dunkler
                int d = 0;
                while (d < 30 && data::blockName(w.getBlock(wx, y - d - 1, wz)) == "minecraft:water") ++d;
                double f = std::max(0.35, 1.0 - d / 40.0);
                c = (uint32_t(((c >> 16) & 255) * f) << 16) | (uint32_t(((c >> 8) & 255) * f) << 8) | uint32_t((c & 255) * f);
            }
            cols[size_t(z) * size_t(size) + size_t(x)] = c;
            hs[size_t(z) * size_t(size) + size_t(x)] = y;
        }
    for (int z = 0; z < size; ++z)
        for (int x = 0; x < size; ++x) {
            size_t i = size_t(z) * size_t(size) + size_t(x);
            uint32_t c = cols[i];
            int north = z > 0 ? hs[i - size_t(size)] : hs[i];
            double shade = hs[i] > north ? 1.12 : hs[i] < north ? 0.85 : 1.0;   // Licht von Norden
            for (int k = 0; k < 3; ++k) {
                double v = double((c >> (16 - 8 * k)) & 255) * shade;
                rgb[i * 3 + size_t(k)] = uint8_t(std::min(255.0, v));
            }
        }
    return writeFileAtomic(file, encodePng(size, size, rgb));
}

}  // namespace worldmap
