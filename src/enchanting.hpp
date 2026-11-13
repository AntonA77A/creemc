// Zaubertisch und Amboss (Vanilla-Regeln, Daten aus den Original-Verzauberungsdateien in game.json).
#pragma once
#include <string>

class Player;

namespace enchanting {

bool open(Player& p, const std::string& block, int x, int y, int z);   // -> true bei Zaubertisch/Amboss
void button(Player& p, int windowId, int button);                       // Angebot im Zaubertisch gewählt
void rename(Player& p, const std::string& name);                        // Name im Amboss geändert

}  // namespace enchanting
