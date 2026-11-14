// Spielregeln wie in Vanilla (/gamerule). Gespeichert in <welt>/gamerules.json.
// Namen gehen in beiden Schreibweisen: "fallDamage" (bis 1.21) und "fall_damage" (ab 26.x).
#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>

struct Config;

class GameRules {
public:
    struct Def {
        std::string name;    // camelCase wie bis 1.21
        std::string snake;   // snake_case wie ab 26.x
        bool isBool;
        int def;             // Standardwert (bool: 0/1)
        std::string description;
    };

    void load(const std::string& worldFolder, const Config& cfg);
    void save() const;

    bool b(const std::string& name) const;    // Wahrheitswert-Regel
    int i(const std::string& name) const;     // Zahl-Regel
    const Def* find(const std::string& name) const;              // beide Schreibweisen, ohne "minecraft:"
    std::string get(const std::string& name) const;              // "" = unbekannt
    bool set(const std::string& name, const std::string& value);  // false = unbekannt/ungültig
    const std::vector<Def>& defs() const;
    std::vector<std::pair<std::string, std::string>> all() const;

private:
    std::map<std::string, int> values_;   // camelCase -> Wert
    std::string file_;
};
