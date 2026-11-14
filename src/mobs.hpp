// Alle Minecraft-Mobs: Werte aus den Vanilla-Daten (Größe, Leben, Schaden, Tempo, Kategorie, Sounds, Loot),
// Verhalten nach Gruppen (friedlich, feindlich, neutral, Fernkampf, fliegend, schwimmend, hüpfend, Creeper),
// Geschosse (Pfeile, Feuerbälle, Schneebälle ...), natürliches Spawnen, Bogen für Spieler.
#pragma once
#include <string>
#include <vector>

class Server;
class Player;
class Mob;

namespace mobs {

struct Info {
    std::string type;          // ohne "minecraft:"
    double w = 0.6, h = 1.8, eye = 1.6;
    float health = 10, damage = 0;
    double speed = 0.25, fly = 0;
    int xp = 0;
    std::string category;      // MONSTER, CREATURE, AMBIENT, WATER_CREATURE, ...
    bool mob = false, fireImmune = false, summonable = true;
    std::string spawn;         // ON_GROUND, IN_WATER, ...
    std::string hurtSound, deathSound;
};

const Info* info(const std::string& type);                    // nullptr = unbekannt
bool isMob(const std::string& type);
std::vector<std::string> types();                              // alle Mob-Arten
Mob* spawn(Server& s, const std::string& type, double x, double y, double z);
void tick(Server& s);                                          // natürliches Spawnen und Verschwinden
void onDeath(Server& s, Mob& mob, int killer);                 // Loot und Erfahrung

// Geschosse
void shoot(Server& s, const std::string& kind, int owner, double x, double y, double z, double vx, double vy, double vz,
           float damage, bool pickup = false);
bool releaseBow(Player& p);                                    // Bogen losgelassen -> true, wenn geschossen

}  // namespace mobs
