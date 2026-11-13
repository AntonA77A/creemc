#include "util.hpp"

#include <cctype>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

// UTF-8-Text <-> Pfad (C++20: u8path ist veraltet)
static fs::path P(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }
static std::string S8(const fs::path& p) {
    auto u = p.u8string();
    return std::string(u.begin(), u.end());
}

// ---- Logging ----------------------------------------------------------------
namespace {
std::mutex g_logMutex;
std::ofstream g_logFile;

void logLine(const char* level, const std::string& msg) {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char ts[16];
    std::strftime(ts, sizeof ts, "%H:%M:%S", &tm);
    std::string line = std::string("[") + ts + " " + level + "] " + msg;
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::cout << line << std::endl;
    if (!g_logFile.is_open()) {
        fs::create_directories("logs");
        g_logFile.open("logs/latest.log", std::ios::app);
    }
    if (g_logFile) g_logFile << line << "\n" << std::flush;
}
}  // namespace

namespace logx {
void info(const std::string& msg) { logLine("INFO", msg); }
void warn(const std::string& msg) { logLine("WARNING", msg); }
void error(const std::string& msg) { logLine("ERROR", msg); }
}  // namespace logx

// ---- UUID -------------------------------------------------------------------
std::string Uuid::hex() const {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (uint8_t c : b) {
        s += d[c >> 4];
        s += d[c & 15];
    }
    return s;
}

std::string Uuid::str() const {
    std::string h = hex();
    return h.substr(0, 8) + "-" + h.substr(8, 4) + "-" + h.substr(12, 4) + "-" + h.substr(16, 4) + "-" + h.substr(20);
}

Uuid Uuid::parse(const std::string& s) {
    std::string h;
    for (char c : s)
        if (c != '-') h += c;
    Uuid u;
    for (size_t i = 0; i < 16 && i * 2 + 1 < h.size(); ++i)
        u.b[i] = uint8_t(std::stoul(h.substr(i * 2, 2), nullptr, 16));
    return u;
}

Uuid Uuid::random() {
    Uuid u;
    for (auto& c : u.b) c = uint8_t(rng()());
    u.b[6] = (u.b[6] & 0x0F) | 0x40;
    u.b[8] = (u.b[8] & 0x3F) | 0x80;
    return u;
}

// ---- Text -------------------------------------------------------------------
std::string toLower(std::string s) {
    for (auto& c : s) c = char(std::tolower((unsigned char)c));
    return s;
}
bool startsWith(const std::string& s, const std::string& p) { return s.size() >= p.size() && s.compare(0, p.size(), p) == 0; }
bool endsWith(const std::string& s, const std::string& p) {
    return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0;
}
std::string removePrefix(const std::string& s, const std::string& p) { return startsWith(s, p) ? s.substr(p.size()) : s; }

std::vector<std::string> split(const std::string& s, char sep, bool skipEmpty) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) {
            if (!skipEmpty || !cur.empty()) out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    if (!skipEmpty || !cur.empty()) out.push_back(cur);
    return out;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string ns(const std::string& name) { return name.find(':') != std::string::npos ? name : "minecraft:" + name; }

std::string fmtDouble(double v, int digits) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*f", digits, v);
    return buf;
}

// ---- Zufall -----------------------------------------------------------------
std::mt19937_64& rng() {
    static std::mt19937_64 g{std::random_device{}()};
    return g;
}
double randDouble(double lo, double hi) { return std::uniform_real_distribution<double>(lo, hi)(rng()); }
int randInt(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng()); }

// ---- Dateien ----------------------------------------------------------------
bool readFile(const std::string& path, std::string& out) {
    std::ifstream f(P(path), std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

bool writeFileAtomic(const std::string& path, const std::string& data) {
    fs::path p = P(path);
    if (p.has_parent_path()) fs::create_directories(p.parent_path());
    fs::path tmp = p;
    tmp += ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f.write(data.data(), std::streamsize(data.size()));
        if (!f) return false;
    }
    std::error_code ec;
    fs::rename(tmp, p, ec);
    if (ec) {   // Windows: Ziel existiert evtl. noch
        fs::remove(p, ec);
        fs::rename(tmp, p, ec);
    }
    return !ec;
}

void makeDirs(const std::string& path) {
    std::error_code ec;
    fs::create_directories(P(path), ec);
}

bool fileExists(const std::string& path) { return fs::exists(P(path)); }

std::string joinPath(const std::string& a, const std::string& b) { return S8(P(a) / P(b)); }

double nowSeconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}
