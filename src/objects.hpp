// Objekt-Entities: Rüstungsständer, Boote (alle Holzarten, mit Kiste, Bambusflöße), Loren (auf Schienen),
// Rahmen (auch leuchtend) und Gemälde. Reiten in Booten und Loren. Werden mit der Welt gespeichert.
#pragma once
#include <string>

#include "entity.hpp"
#include "json.hpp"

class Server;
class Player;

namespace objects {

class ObjectEntity : public Entity {
public:
    ObjectEntity(Server& s, const std::string& type, double x, double y, double z, std::string dropItem);
    std::string dropItem;            // Item beim Abbauen
    int hits = 0, lastHitTick = -100;
    virtual void interact(Player& p, int hand) { (void)p; (void)hand; }
    virtual void attacked(Player& p);    // Standard: abbauen (Kreativ ohne Drop)
    virtual void breakIt(Player* p, bool drop);
    virtual json::Object save() const;
    virtual void load(const json::Value& v) { (void)v; }
};

// Item auf einen Block benutzen (Rüstungsständer, Rahmen, Gemälde, Lore, Boot). -> true = etwas erzeugt
bool useItemOnBlock(Player& p, const std::string& item, int cx, int cy, int cz, int face);
bool useItem(Player& p, const std::string& item);   // in die Luft (Boot aufs Wasser)
bool isObject(Entity* e);
void interact(Player& p, Entity* e, int hand);
void attack(Player& p, Entity* e);

// Reiten
void mount(Player& p, Entity* vehicle);
void dismount(Player& p);
void vehicleMove(Player& p, double x, double y, double z, float yaw, float pitch);   // Boot vom Client gesteuert
void input(Player& p, bool forward, bool backward, bool left, bool right);          // Lore anschieben

// /summon: armor_stand, oak_boat ..., minecart ... -> false = kein Objekt-Typ
bool summon(Server& s, const std::string& type, double x, double y, double z, float yaw);

void saveAll(Server& s);
void loadAll(Server& s);

}  // namespace objects
