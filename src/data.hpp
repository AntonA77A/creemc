// Vanilla-Daten aller Versionen (aus den eingebetteten JSON-Dateien von tools/update_data.py).
// Intern gelten die Block-Zustands-IDs der neuesten Version ("kanonisch").
#pragma once
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "json.hpp"

namespace data {

struct BlockRange {
    int id = 0, def = 0, lo = 0, hi = 0;   // Registry-ID, Standardzustand, erster, letzter Zustand
};

struct PropDef {
    std::string name;
    std::vector<std::string> values;
};

void init();                                    // lädt alles (einmal beim Start)
std::string resource(const std::string& name);  // "versions/777.json", "game.json"

extern std::vector<int> SUPPORTED;              // Protokollnummern, aufsteigend
extern int LATEST;
const json::Value& version(int protocol);       // Daten einer Version
const json::Value& game();                      // Spielregeln (Rezepte, Loot, ...)

// Blöcke (kanonisch)
extern std::unordered_map<std::string, BlockRange> BLOCKS;   // "stone" -> Bereich
extern std::vector<uint8_t> SOLID;                           // Zustand -> hat Kollisionsbox
extern int MAX_STATE;
int blockState(const std::string& name);                     // Standardzustand, -1 = unbekannt
std::string blockName(int state);                            // "minecraft:stone"
const std::vector<PropDef>* propDefs(int state);
std::map<std::string, std::string> props(int state);
std::string prop(int state, const std::string& key);        // "" = gibt es nicht
int withProps(int state, const std::map<std::string, std::string>& changes);
bool hasProp(int state, const std::string& key);

// Items (kanonisch)
extern std::unordered_map<std::string, int> ITEMS;
bool isItem(const std::string& name);
int stackSize(const std::string& item);
std::string baseItem(const std::string& item);              // ohne "@{...}"-Metadaten

std::vector<std::string> versionNames();                    // alle Namen, z.B. "1.21", ..., "26.3"
std::string rangeText();                                     // "1.21 - 26.3"

}  // namespace data
