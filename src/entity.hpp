// Entities: herumliegende Items und Mobs, mit einfacher Physik (wie die Python-Version).
#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "util.hpp"

class Server;
class Version;
class Player;

class Entity {
public:
    Entity(Server& server, const std::string& type, double x, double y, double z);
    virtual ~Entity() = default;

    Server& server;
    std::string type;          // ohne "minecraft:"
    int entityId;
    Uuid uuid;
    double x, y, z, vx = 0, vy = 0, vz = 0;
    float yaw = 0, pitch = 0;
    bool onGround = false, inWater = false, inLava = false, horizontalCollision = false, removed = false;
    int age = 0, fireTicks = 0;
    double width = 0.6, height = 1.8;
    double gravity = 0.08, drag = 0.98;

    virtual bool isMob() const { return false; }
    virtual std::vector<std::pair<std::string, Bytes>> spawnPackets(const Version& v) const;
    virtual void tick();
    void sync();                                  // Position/Flags an alle schicken, wenn geändert
    void remove();                                // aus der Welt nehmen (wird am Tick-Ende gelöscht)
    bool visibleTo(const Player* p) const;        // gibt es den Typ in der Version des Spielers?
    double distanceTo(double ox, double oy, double oz) const;
    void move(double dx, double dy, double dz);   // mit Block-Kollision

protected:
    bool solid(int bx, int by, int bz) const;
    bool collides(double nx, double ny, double nz) const;
    int flags() const { return fireTicks > 0 ? 0x01 : 0; }
    double sentX_, sentY_, sentZ_;
    float sentYaw_ = 0;
    int sentFlags_ = 0;
};

class ItemEntity : public Entity {
public:
    ItemEntity(Server& server, double x, double y, double z, const std::string& item, int count, int pickupDelay = 10);
    std::string item;
    int count;
    int pickupDelay;
    std::vector<std::pair<std::string, Bytes>> spawnPackets(const Version& v) const override;
    void tick() override;
};

class Mob : public Entity {
public:
    Mob(Server& server, const std::string& type, double x, double y, double z, float health);
    float health, maxHealth;
    int hurtCooldown = 0, deathTicks = 0;
    float lastDamage = 0;
    bool dead = false;
    int lastAttacker = 0;      // Entity-ID (Spieler oder Mob), 0 = keiner
    std::function<void(Mob&)> ai;

    // Freier Speicher für die KI
    bool hasWander = false;
    double wanderX = 0, wanderZ = 0;
    int wanderUntil = 0, panic = 0, attackCooldown = 0, fuse = 0;

    bool isMob() const override { return true; }
    void tick() override;
    void walkTowards(double tx, double tz, double speed = 0.1);
    void jump();
    void lookAt(double tx, double tz);
    // attacker: Entity-ID des Angreifers (0 = keiner); -> true, wenn Schaden genommen
    bool damage(float amount, int attacker = 0, const std::string& cause = "generic");
    void knockback(double fromX, double fromZ, double strength = 0.4);
    void die(int killer);
};
