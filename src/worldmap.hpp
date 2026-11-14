// Weltkarte als PNG (Draufsicht mit Blockfarben und Höhenschattierung), für /karte.
#pragma once
#include <string>

class World;

namespace worldmap {

// Zeichnet blocks x blocks Blöcke um (cx, cz) in eine PNG-Datei. -> false bei Schreibfehler
bool render(World& w, int cx, int cz, int size, const std::string& file, int sliceY = 99999);   // sliceY: Schnitt auf dieser Höhe

}  // namespace worldmap
