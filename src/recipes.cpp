#include "recipes.hpp"

#include <algorithm>

#include "data.hpp"
#include "util.hpp"

static std::set<std::string> toSet(const json::Value& v) {
    std::set<std::string> out;
    if (v.isArray()) {
        for (const auto& e : v.asArray()) out.insert(ns(e.asString()));
    } else if (v.isString()) {
        out.insert(ns(v.asString()));
    }
    return out;
}

int RecipeBook::loadVanilla() {
    for (const auto& r : data::game()["recipes"].asArray()) {
        const json::Value& res = r["result"];
        if (!res[0].isString() || res[0].asString().empty()) continue;
        std::string item = res[0].asString();
        int count = int(res[1].asInt(1));
        std::string type = r["type"].asString();
        if (type == "shaped") {
            std::vector<std::string> pattern;
            for (const auto& row : r["pattern"].asArray()) pattern.push_back(row.asString());
            std::map<char, std::set<std::string>> key;
            for (const auto& [k, v] : r["key"].asObject())
                if (!k.empty()) key[k[0]] = toSet(v);
            addShaped(pattern, key, item, count);
        } else if (type == "shapeless") {
            std::vector<std::set<std::string>> ings;
            for (const auto& i : r["ingredients"].asArray()) ings.push_back(toSet(i));
            addShapeless(ings, item, count);
        } else if (type == "smelting" || type == "blasting" || type == "smoking") {
            for (const auto& ing : r["ingredient"].asArray())
                cooking_[type][ns(ing.asString())] = {ns(item), count, int(r["time"].asInt(200)), r["xp"].asDouble(0)};
        }
    }
    return int(shaped_.size() + shapeless_.size());
}

void RecipeBook::addShaped(const std::vector<std::string>& pattern, const std::map<char, std::set<std::string>>& key,
                           const std::string& result, int count) {
    Shaped s;
    s.h = int(pattern.size());
    s.w = 0;
    for (const auto& row : pattern) s.w = std::max(s.w, int(row.size()));
    for (const auto& row : pattern)
        for (int i = 0; i < s.w; ++i) {
            char c = i < int(row.size()) ? row[size_t(i)] : ' ';
            if (c == ' ' || !key.count(c)) s.cells.push_back(std::nullopt);
            else s.cells.push_back(key.at(c));
        }
    s.result = {ns(result), count};
    shaped_.push_back(std::move(s));
}

void RecipeBook::addShapeless(const std::vector<std::set<std::string>>& ings, const std::string& result, int count) {
    shapeless_.push_back({ings, {ns(result), count}});
}

static bool fits(const std::vector<std::string>& grid, int width, int height, int rw, int rh,
                 const std::vector<std::optional<std::set<std::string>>>& cells, int ox, int oy, bool mirror) {
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            const std::string& item = grid[size_t(y * width + x)];
            int px = x - ox, py = y - oy;
            if (px >= 0 && px < rw && py >= 0 && py < rh) {
                const auto& want = cells[size_t(py * rw + (mirror ? rw - 1 - px : px))];
                if (!want) {
                    if (!item.empty()) return false;
                } else if (!want->count(item)) {
                    return false;
                }
            } else if (!item.empty()) {
                return false;
            }
        }
    return true;
}

static bool shapelessFits(std::vector<std::set<std::string>> ings, const std::vector<std::string>& items, size_t from) {
    if (from == items.size()) return ings.empty();
    for (size_t i = 0; i < ings.size(); ++i) {
        if (ings[i].count(items[from])) {
            auto rest = ings;
            rest.erase(rest.begin() + long(i));
            if (shapelessFits(rest, items, from + 1)) return true;
        }
    }
    return false;
}

std::optional<RecipeBook::Result> RecipeBook::match(const std::vector<std::string>& rawGrid, int width) const {
    std::vector<std::string> grid;
    std::vector<std::string> filled;
    for (const auto& g : rawGrid) {
        grid.push_back(g.empty() ? g : data::baseItem(g));
        if (!g.empty()) filled.push_back(grid.back());
    }
    if (filled.empty()) return std::nullopt;
    int height = int(grid.size()) / width;
    for (const auto& s : shaped_) {
        if (s.w > width || s.h > height) continue;
        size_t n = size_t(std::count_if(s.cells.begin(), s.cells.end(), [](const auto& c) { return c.has_value(); }));
        if (n != filled.size()) continue;
        for (int ox = 0; ox <= width - s.w; ++ox)
            for (int oy = 0; oy <= height - s.h; ++oy)
                if (fits(grid, width, height, s.w, s.h, s.cells, ox, oy, false) ||
                    fits(grid, width, height, s.w, s.h, s.cells, ox, oy, true))
                    return s.result;
    }
    for (const auto& s : shapeless_)
        if (s.ings.size() == filled.size() && shapelessFits(s.ings, filled, 0)) return s.result;
    return std::nullopt;
}

const RecipeBook::Cooking* RecipeBook::cook(const std::string& kind, const std::string& item) const {
    auto k = cooking_.find(kind);
    if (k == cooking_.end()) return nullptr;
    auto it = k->second.find(data::baseItem(item));
    return it == k->second.end() ? nullptr : &it->second;
}
