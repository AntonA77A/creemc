// Kleinkram: Logging, UUIDs, Text-Helfer, Zufall.
#pragma once
#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

using Bytes = std::string;   // Binärdaten (std::string ist binärsicher)

// ---- Logging ----------------------------------------------------------------
namespace logx {
void info(const std::string& msg);
void warn(const std::string& msg);
void error(const std::string& msg);
}  // namespace logx

// ---- UUID -------------------------------------------------------------------
struct Uuid {
    std::array<uint8_t, 16> b{};
    bool operator==(const Uuid& o) const { return b == o.b; }
    bool operator!=(const Uuid& o) const { return b != o.b; }
    bool operator<(const Uuid& o) const { return b < o.b; }
    std::string str() const;                 // mit Bindestrichen
    std::string hex() const;                 // ohne Bindestriche
    static Uuid parse(const std::string& s);
    static Uuid random();
};

struct UuidHash {
    size_t operator()(const Uuid& u) const {
        size_t h = 1469598103934665603ull;
        for (uint8_t c : u.b) h = (h ^ c) * 1099511628211ull;
        return h;
    }
};

// ---- Text -------------------------------------------------------------------
std::string toLower(std::string s);
bool startsWith(const std::string& s, const std::string& p);
bool endsWith(const std::string& s, const std::string& p);
std::string removePrefix(const std::string& s, const std::string& p);
std::vector<std::string> split(const std::string& s, char sep, bool skipEmpty = false);
std::string trim(const std::string& s);
std::string ns(const std::string& name);   // "stone" -> "minecraft:stone"
std::string fmtDouble(double v, int digits = 2);

// ---- Zufall -----------------------------------------------------------------
std::mt19937_64& rng();
double randDouble(double lo = 0.0, double hi = 1.0);
int randInt(int lo, int hi);   // inklusiv

// ---- Dateien ----------------------------------------------------------------
bool readFile(const std::string& path, std::string& out);
bool writeFileAtomic(const std::string& path, const std::string& data);
void makeDirs(const std::string& path);
bool fileExists(const std::string& path);
std::string joinPath(const std::string& a, const std::string& b);

// ---- Zeit -------------------------------------------------------------------
double nowSeconds();   // monoton
