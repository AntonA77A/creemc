#include "enchanting.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <random>

#include "blocks.hpp"
#include "data.hpp"
#include "player.hpp"
#include "server.hpp"
#include "vanilla.hpp"
#include "version.hpp"

namespace enchanting {
namespace {

const std::string BOOK = "minecraft:book", ENCHANTED_BOOK = "minecraft:enchanted_book", LAPIS = "minecraft:lapis_lazuli";
enum { MENU_ANVIL = 8, MENU_ENCHANTMENT = 13 };

const json::Value& ench() { return data::game()["enchantments"]; }
int enchantability(const std::string& base) { return int(data::game()["enchantable"][base].asInt(0)); }

bool contains(const json::Value& arr, const std::string& s) {
    if (!arr.isArray()) return false;
    for (const auto& v : arr.asArray())
        if (v.str() == s) return true;
    return false;
}
int cost(const json::Value& pair, int level) { return int(pair[0].asInt() + pair[1].asInt() * (level - 1)); }
bool compatible(const std::string& a, const std::string& b) {
    return a != b && !contains(ench()[a]["exclusive"], b) && !contains(ench()[b]["exclusive"], a);
}

// Item mit Metadaten wie die Python-Version: Schlüssel sortiert, kompakt ("name@{...}")
json::Value sortedObj(const std::map<std::string, json::Value>& m) {
    json::Object o;
    for (const auto& [k, v] : m) o[k] = v;
    return o;
}
std::string withMeta(const std::string& item, std::map<std::string, json::Value> meta) {
    for (auto it = meta.begin(); it != meta.end();) {
        const json::Value& v = it->second;
        bool empty = v.isNull() || (v.isObject() && v.size() == 0) || (v.isString() && v.asString().empty()) ||
                     (v.isNumber() && v.asDouble() == 0);
        it = empty ? meta.erase(it) : std::next(it);
    }
    std::string base = data::baseItem(item);
    return meta.empty() ? base : base + "@" + json::write(sortedObj(meta));
}
std::map<std::string, json::Value> metaMap(const std::string& item) {
    std::map<std::string, json::Value> out;
    json::Value m = vanilla::itemMeta(item);
    for (const auto& [k, v] : m.asObject()) out[k] = v;
    return out;
}
std::map<std::string, int> enchMap(const json::Value& v) {
    std::map<std::string, int> out;
    if (v.isObject())
        for (const auto& [k, x] : v.asObject()) out[k] = int(x.asInt());
    return out;
}
json::Value enchJson(const std::map<std::string, int>& m) {
    json::Object o;
    for (const auto& [k, v] : m) o[k] = v;
    return o;
}

class MemStorage : public blocks::Storage {
public:
    explicit MemStorage(int n) : items_(size_t(n)) {}
    int size() const override { return int(items_.size()); }
    Stack get(int i) const override { return items_[size_t(i)]; }
    void set(int i, const Stack& st) override { items_[size_t(i)] = st.second > 0 ? st : Stack{}; }

private:
    std::vector<Stack> items_;
};

// ---- Zaubertisch ------------------------------------------------------------

int countBookshelves(World& w, int x, int y, int z) {
    // Bücherregale im Ring 2 Blöcke um den Tisch (Ebene 0 und +1), mit Luft dazwischen
    static int shelf = data::blockState("bookshelf");
    int n = 0;
    for (int dx = -2; dx <= 2; ++dx)
        for (int dz = -2; dz <= 2; ++dz) {
            if (std::max(std::abs(dx), std::abs(dz)) != 2) continue;
            for (int dy = 0; dy <= 1; ++dy)
                if (w.getBlock(x + dx, y + dy, z + dz) == shelf && w.getBlock(x + dx / 2, y + dy, z + dz / 2) == 0) ++n;
        }
    return n;
}

int rint(std::mt19937& r, int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(r); }
double rdouble(std::mt19937& r) { return std::uniform_real_distribution<double>(0, 1)(r); }

std::vector<int> tableCosts(std::mt19937& rng, int shelves, int enchVal) {
    if (!enchVal) return {0, 0, 0};
    int b = std::min(shelves, 15);
    std::vector<int> out;
    for (int slot = 0; slot < 3; ++slot) {
        int i = rint(rng, 1, 8) + (b >> 1) + rint(rng, 0, b);
        int c = slot == 0 ? std::max(i / 3, 1) : slot == 1 ? i * 2 / 3 + 1 : std::max(i, b * 2);
        out.push_back(c >= slot + 1 ? c : 0);
    }
    return out;
}

std::vector<std::pair<std::string, int>> selectEnchantments(std::mt19937& rng, const std::string& item, int level) {
    std::string base = data::baseItem(item);
    bool isBook = base == BOOK;
    int enchVal = isBook ? 1 : enchantability(base);
    if (enchVal <= 0) return {};
    level += 1 + rint(rng, 0, enchVal / 4) + rint(rng, 0, enchVal / 4);
    double f = (rdouble(rng) + rdouble(rng) - 1.0) * 0.15;
    level = std::max(1, int(std::lround(level + level * f)));
    struct Opt {
        std::string name;
        int lvl, weight;
    };
    std::vector<Opt> pool;
    std::vector<std::string> names;
    for (const auto& [n, v] : ench().asObject()) names.push_back(n);
    std::sort(names.begin(), names.end());
    for (const auto& n : names) {
        const json::Value& info = ench()[n];
        if (!info["table"].asBool() || (!isBook && !contains(info["primary"], base))) continue;
        for (int lvl = int(info["max"].asInt()); lvl > 0; --lvl)
            if (cost(info["min"], lvl) <= level && level <= cost(info["max_cost"], lvl)) {
                pool.push_back({n, lvl, int(info["weight"].asInt())});
                break;
            }
    }
    auto pick = [&](const std::vector<Opt>& opts) {
        int total = 0;
        for (const auto& o : opts) total += o.weight;
        int r = total > 0 ? rint(rng, 0, total - 1) : 0;
        for (const auto& o : opts) {
            r -= o.weight;
            if (r < 0) return std::make_pair(o.name, o.lvl);
        }
        return std::make_pair(opts.back().name, opts.back().lvl);
    };
    if (pool.empty()) return {};
    std::vector<std::pair<std::string, int>> result{pick(pool)};
    while (rint(rng, 0, 49) <= level) {
        std::vector<Opt> next;
        for (const auto& o : pool) {
            bool ok = true;
            for (const auto& r : result)
                if (!compatible(o.name, r.first)) ok = false;
            if (ok) next.push_back(o);
        }
        pool = next;
        if (pool.empty()) break;
        result.push_back(pick(pool));
        level /= 2;
    }
    if (isBook && result.size() > 1) result.erase(result.begin() + rint(rng, 0, int(result.size()) - 1));
    return result;
}

int seedOf(Player& p) {
    if (!p.pluginData.isObject()) p.pluginData = json::Object();
    json::Object& o = p.pluginData.asObject();
    if (!o.contains("ench_seed")) o["ench_seed"] = randInt(0, 0x7FFFFFFF);
    return int(o["ench_seed"].asInt());
}

class EnchantWindow : public blocks::ContainerWindow {
public:
    EnchantWindow(Player& p, int id, int x, int y, int z)
        : ContainerWindow(p, id, std::make_unique<MemStorage>(2), MENU_ENCHANTMENT, "Verzaubern"), px(x), py(y), pz(z) {
        temporary = true;
    }
    int px, py, pz;
    int costs[3] = {0, 0, 0};
    int hintId[3] = {-1, -1, -1}, hintLvl[3] = {-1, -1, -1};

    bool canPlace(int i, const std::string& item) const override { return i != 1 || data::baseItem(item) == LAPIS; }

    void updateResult() override {
        Stack st = storage->get(0);
        if (!st.first.empty() && st.second > 1) {   // nur ein Item auf einmal (wie Vanilla)
            player.addItem(st.first, st.second - 1, false);
            storage->set(0, {st.first, 1});
            st = storage->get(0);
        }
        for (int i = 0; i < 3; ++i) {
            costs[i] = 0;
            hintId[i] = hintLvl[i] = -1;
        }
        if (!st.first.empty() && vanilla::itemMeta(st.first)["ench"].isNull()) {
            std::string base = data::baseItem(st.first);
            int enchVal = base == BOOK ? 1 : enchantability(base);
            int shelves = countBookshelves(player.server.world(), px, py, pz);
            std::mt19937 r(uint32_t(seedOf(player)));
            auto c = tableCosts(r, shelves, enchVal);
            for (int slot = 0; slot < 3; ++slot) {
                costs[slot] = c[size_t(slot)];
                if (!costs[slot]) continue;
                std::mt19937 rs(uint32_t(seedOf(player) + slot));
                auto picks = selectEnchantments(rs, st.first, costs[slot]);
                if (picks.empty()) {
                    costs[slot] = 0;
                } else {
                    hintId[slot] = player.version->enchantmentId(picks[0].first);
                    hintLvl[slot] = picks[0].second;
                }
            }
        }
        if (player.window.get() != this) return;
        for (int i = 0; i < 3; ++i) player.sendWindowProperty(i, costs[i]);
        player.sendWindowProperty(3, seedOf(player) & 0x7FF0);
        for (int i = 0; i < 3; ++i) {
            player.sendWindowProperty(4 + i, hintId[i]);
            player.sendWindowProperty(7 + i, hintLvl[i]);
        }
    }

    void button(int slot) {
        Stack st = storage->get(0), lapis = storage->get(1);
        if (slot < 0 || slot > 2 || st.first.empty() || !costs[slot]) return;
        bool creative = player.gamemode == 1;
        if (!creative && (player.level() < costs[slot] || lapis.first.empty() || lapis.second < slot + 1)) return;
        std::mt19937 rs(uint32_t(seedOf(player) + slot));
        auto picks = selectEnchantments(rs, st.first, costs[slot]);
        if (picks.empty()) return;
        std::map<std::string, int> chosen(picks.begin(), picks.end());
        std::string item;
        if (data::baseItem(st.first) == BOOK) {
            item = withMeta(ENCHANTED_BOOK, {{"stored", enchJson(chosen)}});
        } else {
            auto meta = metaMap(st.first);
            meta["ench"] = enchJson(chosen);
            item = withMeta(st.first, meta);
        }
        storage->set(0, {item, 1});
        if (!creative) {
            storage->set(1, {LAPIS, lapis.second - slot - 1});
            player.takeLevels(slot + 1);
        }
        player.pluginData.asObject()["ench_seed"] = randInt(0, 0x7FFFFFFF);
        player.server.playSound("minecraft:block.enchantment_table.use", px + .5, py + .5, pz + .5, 1.0f, float(randDouble(0.9, 1.0)), 4);
        player.syncInventory();
    }
};

// ---- Amboss -----------------------------------------------------------------

class AnvilWindow : public blocks::ContainerWindow {
public:
    AnvilWindow(Player& p, int id, int x, int y, int z)
        : ContainerWindow(p, id, std::make_unique<MemStorage>(3), MENU_ANVIL, "Reparieren & Benennen"), px(x), py(y), pz(z) {
        temporary = true;
        resultSlot = 2;
    }
    int px, py, pz;
    int cost = 0;
    std::optional<std::string> name;   // vom Client gesendet

    bool canPlace(int i, const std::string& item) const override { (void)item; return i != 2; }

    void updateResult() override {
        compute();
        if (player.window.get() == this) player.sendWindowProperty(0, cost);
    }

    void compute() {
        result = {};
        cost = 0;
        Stack left = storage->get(0), right = storage->get(1);
        if (left.first.empty()) return;
        std::string base = data::baseItem(left.first);
        auto meta = metaMap(left.first);
        auto out = meta;
        int c = 0;
        int prior = meta.count("repair") ? int(meta["repair"].asInt()) : 0;
        if (!right.first.empty()) {
            std::string rbase = data::baseItem(right.first);
            auto rmeta = metaMap(right.first);
            prior += rmeta.count("repair") ? int(rmeta["repair"].asInt()) : 0;
            bool fromBook = rbase == ENCHANTED_BOOK;
            if (!fromBook && rbase != base) return;
            std::string key = base == ENCHANTED_BOOK ? "stored" : "ench";
            auto current = enchMap(out.count(key) ? out[key] : json::Value());
            auto incoming = enchMap(rmeta.count("ench") && rmeta["ench"].isObject() ? rmeta["ench"] : rmeta["stored"]);
            bool added = false;
            for (const auto& [n, lvl] : incoming) {
                const json::Value& info = ench()[n];
                if (info.isNull()) continue;
                if (base != ENCHANTED_BOOK && !contains(info["supported"], base)) continue;
                bool clash = false;
                for (const auto& [other, ol] : current)
                    if (other != n && !compatible(n, other)) clash = true;
                if (clash) {
                    c += 1;   // Vanilla: unverträgliche Verzauberung kostet trotzdem 1
                    continue;
                }
                int have = current.count(n) ? current[n] : 0;
                int nw = std::min(int(info["max"].asInt()), have == lvl ? have + 1 : std::max(have, lvl));
                current[n] = nw;
                int anvil = int(info["anvil"].asInt(1));
                c += std::max(1, fromBook ? anvil / 2 : anvil) * nw;
                added = true;
            }
            if (!added) return;
            out[key] = enchJson(current);
        }
        int merged = c;
        if (name) {
            std::string current = meta.count("name") ? meta["name"].str() : "";
            if (trim(*name).empty()) {
                if (out.count("name")) {   // leerer Name entfernt einen eigenen Namen
                    out.erase("name");
                    c += 1;
                }
            } else if (*name != current) {
                out["name"] = *name;
                c += 1;
            }
        }
        if (c == 0) return;
        int total = c + prior;
        if (!merged && total >= 40) total = 39;   // nur Umbenennen ist nie "zu teuer"
        cost = total;
        if (total >= 40 && player.gamemode != 1) return;   // "Zu teuer!"
        int repair = std::max(meta.count("repair") ? int(meta["repair"].asInt()) : 0,
                              right.first.empty() ? 0 : int(vanilla::itemMeta(right.first)["repair"].asInt(0)));
        out["repair"] = merged ? repair * 2 + 1 : repair;
        result = {withMeta(base, out), 1};
    }

    bool takeResult() override {
        if (result.first.empty()) return false;
        if (player.gamemode != 1 && player.level() < cost) return false;
        if (player.gamemode != 1) player.takeLevels(cost);
        storage->set(0, {});
        Stack right = storage->get(1);
        if (!right.first.empty()) storage->set(1, {right.first, right.second - 1});
        player.server.playSound("minecraft:block.anvil.use", px + .5, py + .5, pz + .5, 1.0f, float(randDouble(0.9, 1.0)), 4);
        updateResult();
        return true;
    }
};

}  // namespace

bool open(Player& p, const std::string& block, int x, int y, int z) {
    if (block == "minecraft:enchanting_table") {
        p.openWindow(std::make_unique<EnchantWindow>(p, p.newWindowId(), x, y, z));
        return true;
    }
    if (block == "minecraft:anvil" || block == "minecraft:chipped_anvil" || block == "minecraft:damaged_anvil") {
        p.openWindow(std::make_unique<AnvilWindow>(p, p.newWindowId(), x, y, z));
        return true;
    }
    return false;
}

void button(Player& p, int windowId, int b) {
    auto* w = dynamic_cast<EnchantWindow*>(p.window.get());
    if (w && w->id == windowId && !p.dead) w->button(b);
}

void rename(Player& p, const std::string& n) {
    auto* w = dynamic_cast<AnvilWindow*>(p.window.get());
    if (!w) return;
    w->name = n.substr(0, 50);
    p.syncInventory();
}

}  // namespace enchanting
