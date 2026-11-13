// Wie ein Block beim Setzen ausgerichtet wird (Blickrichtung, Klickseite, zweiteilige Blöcke).
#pragma once
#include <optional>
#include <string>
#include <tuple>
#include <vector>

class Server;
class World;

namespace placement {

struct Part {
    int x, y, z, state;
};

std::string blockForItem(const std::string& item);   // "" = setzt keinen Block
std::string lookDirection(float yaw);
std::vector<Part> place(std::string name, int x, int y, int z, int face, float yaw, float pitch, float cursorY);
std::optional<std::tuple<int, int, int>> chestPartner(int x, int y, int z, int state);
void connectChest(Server& server, int x, int y, int z, bool sneaking);
std::string stairShape(World& world, int x, int y, int z, int state);
void shapesListener(Server& server, int x, int y, int z);   // Treppen-Ecken, Doppelkisten
std::optional<std::tuple<int, int, int>> otherPart(int x, int y, int z, int state);

extern const int FACE_OFFSETS[6][3];

}  // namespace placement
