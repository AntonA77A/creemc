#include "data.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>

#include "util.hpp"

#ifdef _WIN32
#include <windows.h>
#else
// Von CMake erzeugt (embed.S + embedded.cpp): eingebettete Dateien
struct EmbeddedFile {
    const char* name;
    const char* start;
    const char* end;
};
extern const EmbeddedFile EMBEDDED_FILES[];
extern const int EMBEDDED_COUNT;
#endif

namespace data {

std::vector<int> SUPPORTED;
int LATEST = 0;
std::unordered_map<std::string, BlockRange> BLOCKS;
std::vector<uint8_t> SOLID;
int MAX_STATE = 0;
std::unordered_map<std::string, int> ITEMS;

namespace {
std::map<int, json::Value> g_versions;
json::Value g_game;
std::vector<std::pair<int, std::string>> g_ranges;   // (erster Zustand, Name), sortiert
std::vector<BlockRange> g_rangeInfo;                 // parallel zu g_ranges
std::unordered_map<std::string, std::vector<PropDef>> g_props;   // Name -> Eigenschaften
std::unordered_map<std::string, int> g_stack;
}  // namespace

std::string resource(const std::string& name) {
    // 1) Datei neben dem Programm (zum Testen / Überschreiben)
    std::string out;
    if (readFile(joinPath("data", name), out)) return out;
#ifdef _WIN32
    // 2) Eingebettete Ressource (Name: Pfad mit '/' -> '_', groß geschrieben)
    std::string id = name;
    for (auto& c : id) {
        if (c == '/' || c == '.') c = '_';
        c = char(std::toupper((unsigned char)c));
    }
    HRSRC res = FindResourceA(nullptr, id.c_str(), MAKEINTRESOURCEA(10));   // RT_RCDATA
    if (res) {
        HGLOBAL h = LoadResource(nullptr, res);
        DWORD size = SizeofResource(nullptr, res);
        if (h && size) return std::string(static_cast<const char*>(LockResource(h)), size);
    }
#else
    for (int i = 0; i < EMBEDDED_COUNT; ++i)
        if (name == EMBEDDED_FILES[i].name)
            return std::string(EMBEDDED_FILES[i].start, size_t(EMBEDDED_FILES[i].end - EMBEDDED_FILES[i].start));
#endif
    throw std::runtime_error("Daten fehlen: " + name);
}

void init() {
    // Unterstützte Versionen: 767 (1.21) bis 777 (26.3)
    for (int p = 767; p <= 799; ++p) {
        try {
            g_versions[p] = json::parse(resource("versions/" + std::to_string(p) + ".json"));
            SUPPORTED.push_back(p);
        } catch (const std::exception&) {
            if (p > 777) break;
            throw;
        }
    }
    LATEST = SUPPORTED.back();
    g_game = json::parse(resource("game.json"));

    const json::Value& canon = g_versions[LATEST];
    for (const auto& [name, b] : canon["blocks"].asObject()) {
        BlockRange r{int(b[0].asInt()), int(b[1].asInt()), int(b[2].asInt()), int(b[3].asInt())};
        BLOCKS[name] = r;
        g_ranges.emplace_back(r.lo, name);
        MAX_STATE = std::max(MAX_STATE, r.hi);
    }
    std::sort(g_ranges.begin(), g_ranges.end());
    for (const auto& [lo, name] : g_ranges) g_rangeInfo.push_back(BLOCKS[name]);
    for (const auto& [name, id] : canon["items"].asObject()) ITEMS[name] = int(id.asInt());

    SOLID.assign(size_t(MAX_STATE) + 1, 0);
    const json::Value& info = g_game["blocks"];
    for (const auto& [name, r] : BLOCKS) {
        const json::Value& bi = info["minecraft:" + name];
        if (bi["solid"].asBool())
            std::fill(SOLID.begin() + r.lo, SOLID.begin() + r.hi + 1, uint8_t(1));
        const json::Value& states = bi["states"];
        if (states.isArray() && states.size()) {
            std::vector<PropDef> defs;
            for (const auto& pd : states.asArray()) {
                PropDef d;
                d.name = pd[0].asString();
                for (const auto& v : pd[1].asArray()) d.values.push_back(v.asString());
                defs.push_back(std::move(d));
            }
            g_props["minecraft:" + name] = std::move(defs);
        }
    }
    for (const auto& [item, n] : g_game["stack_sizes"].asObject()) g_stack[item] = int(n.asInt());
}

const json::Value& version(int protocol) { return g_versions.at(protocol); }
const json::Value& game() { return g_game; }

int blockState(const std::string& name) {
    auto it = BLOCKS.find(removePrefix(name, "minecraft:"));
    return it == BLOCKS.end() ? -1 : it->second.def;
}

static int rangeIndex(int state) {
    auto it = std::upper_bound(g_ranges.begin(), g_ranges.end(), state,
                               [](int s, const std::pair<int, std::string>& r) { return s < r.first; });
    if (it == g_ranges.begin()) return -1;
    int i = int(it - g_ranges.begin()) - 1;
    return state <= g_rangeInfo[size_t(i)].hi ? i : -1;
}

std::string blockName(int state) {
    int i = rangeIndex(state);
    return i < 0 ? "minecraft:air" : "minecraft:" + g_ranges[size_t(i)].second;
}

const std::vector<PropDef>* propDefs(int state) {
    auto it = g_props.find(blockName(state));
    return it == g_props.end() ? nullptr : &it->second;
}

std::map<std::string, std::string> props(int state) {
    std::map<std::string, std::string> out;
    int i = rangeIndex(state);
    const auto* defs = propDefs(state);
    if (i < 0 || !defs) return out;
    int idx = state - g_rangeInfo[size_t(i)].lo;
    for (auto it = defs->rbegin(); it != defs->rend(); ++it) {   // letzte Eigenschaft ändert sich am schnellsten
        int n = int(it->values.size());
        out[it->name] = it->values[size_t(idx % n)];
        idx /= n;
    }
    return out;
}

std::string prop(int state, const std::string& key) {
    auto p = props(state);
    auto it = p.find(key);
    return it == p.end() ? "" : it->second;
}

bool hasProp(int state, const std::string& key) {
    const auto* defs = propDefs(state);
    if (!defs) return false;
    for (const auto& d : *defs)
        if (d.name == key) return true;
    return false;
}

int withProps(int state, const std::map<std::string, std::string>& changes) {
    int i = rangeIndex(state);
    const auto* defs = propDefs(state);
    if (i < 0 || !defs) return state;
    auto current = props(state);
    int idx = 0;
    for (const auto& d : *defs) {
        auto c = changes.find(d.name);
        std::string want = c != changes.end() ? toLower(c->second) : current[d.name];
        auto pos = std::find(d.values.begin(), d.values.end(), want);
        if (pos == d.values.end()) pos = std::find(d.values.begin(), d.values.end(), current[d.name]);
        idx = idx * int(d.values.size()) + int(pos - d.values.begin());
    }
    return g_rangeInfo[size_t(i)].lo + idx;
}

std::string baseItem(const std::string& item) {
    size_t at = item.find('@');
    return at == std::string::npos ? item : item.substr(0, at);
}

bool isItem(const std::string& name) { return ITEMS.count(removePrefix(baseItem(name), "minecraft:")) > 0; }

int stackSize(const std::string& item) {
    auto it = g_stack.find(baseItem(item));
    return it == g_stack.end() ? 64 : it->second;
}

std::vector<std::string> versionNames() {
    std::vector<std::string> out;
    for (int p : SUPPORTED)
        for (const auto& n : g_versions[p]["names"].asArray()) out.push_back(n.asString());
    return out;
}

std::string rangeText() {
    auto n = versionNames();
    return n.front() + " - " + n.back();
}

}  // namespace data
