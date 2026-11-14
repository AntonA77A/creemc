// Vanilla-Spielregeln: Überleben (Leben, Hunger, Schaden, Tod), Block-Drops nach den Loot-Tabellen,
// Kampf (Waffen, Rüstung, Verzauberungen, Streitkolben), Mobs, TNT und Explosionen.
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "json.hpp"

class Server;
class Player;
class Mob;
class Commands;

namespace vanilla {

void init(Server& server);   // Befehle registrieren
void tick(Server& server);   // einmal pro Server-Tick

// Ereignisse vom Spieler
void onMove(Player& p, double fromX, double fromY, double fromZ);
void onLand(Player& p, double fallDistance);
void onBlockBreak(Player& p, int x, int y, int z, int state);
void onUseItem(Player& p, int hand, const std::string& item);
// Item auf einen Block benutzen (Feuerzeug, Spawn-Ei ...). (cx,cy,cz) = angeklickt, (tx,ty,tz) = davor.
// -> true, wenn das Item etwas getan hat (dann wird nichts gebaut)
bool useItemOnBlock(Player& p, int hand, const std::string& item, int cx, int cy, int cz, int tx, int ty, int tz);
void ignite(Server& server, int x, int y, int z, bool& ok);
void onAttack(Player& p, int target);
void onInteractEntity(Player& p, int target, int hand);
void onMobDeath(Server& server, Mob& mob, int killer);
float reduceDamage(Player& p, float amount, const std::string& cause, int breach);

// Items mit Metadaten: "minecraft:diamond_sword@{"ench":{"minecraft:sharpness":3}}"
json::Value itemMeta(const std::string& item);
int enchantLevel(const std::string& item, const std::string& enchantment);

// Loot
std::vector<std::pair<std::string, int>> blockDrops(const std::string& block, const std::string& tool, bool ignoreTool = false);
int blockXp(const std::string& block, const std::string& tool);
// Mob-Loot nach den Vanilla-Tabellen ("minecraft:zombie", "minecraft:sheep/white" ...)
std::vector<std::pair<std::string, int>> entityDrops(Server& s, const std::string& table, const std::string& weapon, bool killedByPlayer,
                                                     bool onFire);

// Mobs, TNT, Explosionen
bool isMobType(const std::string& type);
std::vector<std::string> mobTypes();
void spawnMobs(Server& server, const std::string& type, double x, double y, double z, int count);
void primeTnt(Server& server, int x, int y, int z, int fuse = 80);
void spawnTnt(Server& server, double x, double y, double z, int fuse = 80);
void explode(Server& server, double x, double y, double z, double power = 4.0, bool breakBlocks = true);

}  // namespace vanilla
