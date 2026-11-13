// Welt-Physik: fließendes Wasser/Lava, fallende Blöcke, Eimer, Hacke, Knochenmehl, Pflanzenwachstum.
#pragma once
#include <string>

class Server;
class Player;

namespace physics {

void init(Server& server);                                     // Block-Listener anmelden
void tick(Server& server);                                     // zufällige Ticks für Pflanzen
bool useBucket(Player& p, const std::string& item);            // Eimer füllen/ausgießen (Blickrichtung)
bool till(Server& server, int x, int y, int z);                // Hacke: Erde -> Ackerboden
bool isGrowable(int state);
bool grow(Server& server, int x, int y, int z, bool bonemeal); // Knochenmehl / Wachstum
bool canPlace(Server& server, const std::string& block, int x, int y, int z);   // Saat nur auf Ackerboden

}  // namespace physics
