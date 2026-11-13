// Crafting-Rezepte (Vanilla-Rezepte aus game.json, eigene können dazukommen).
#pragma once
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

class RecipeBook {
public:
    using Result = std::pair<std::string, int>;
    struct Cooking {
        std::string result;
        int count = 1, ticks = 200;
        double xp = 0;
    };

    int loadVanilla();
    void addShaped(const std::vector<std::string>& pattern, const std::map<char, std::set<std::string>>& key,
                   const std::string& result, int count);
    void addShapeless(const std::vector<std::set<std::string>>& ings, const std::string& result, int count);
    std::optional<Result> match(const std::vector<std::string>& grid, int width) const;
    const Cooking* cook(const std::string& kind, const std::string& item) const;

private:
    struct Shaped {
        int w, h;
        std::vector<std::optional<std::set<std::string>>> cells;
        Result result;
    };
    struct Shapeless {
        std::vector<std::set<std::string>> ings;
        Result result;
    };
    std::vector<Shaped> shaped_;
    std::vector<Shapeless> shapeless_;
    std::map<std::string, std::map<std::string, Cooking>> cooking_;
};
