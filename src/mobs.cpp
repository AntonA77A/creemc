#include "mobs.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <unordered_map>

#include "api.hpp"
#include "buffer.hpp"
#include "data.hpp"
#include "entity.hpp"
#include "player.hpp"
#include "server.hpp"
#include "vanilla.hpp"
#include "version.hpp"

namespace mobs {
namespace {

const double PI = 3.14159265358979;
int fl(double v) { return int(std::floor(v)); }
bool in(const std::string& t, std::initializer_list<const char*> l) {
    for (const char* s : l)
        if (t == s) return true;
    return false;
}

// ---- Daten ------------------------------------------------------------------

const std::unordered_map<std::string, Info>& table() {
    static const std::unordered_map<std::string, Info> t = [] {
        std::unordered_map<std::string, Info> out;
        const json::Value& all = data::game()["entity_info"];
        if (!all.isObject()) return out;
        for (const auto& [k, v] : all.asObject()) {
            Info i;
            i.type = removePrefix(k, "minecraft:");
            i.w = v["w"].asDouble(0.6);
            i.h = v["h"].asDouble(1.8);
            i.eye = v["eye"].asDouble(i.h * 0.85);
            i.health = float(v["health"].asDouble(10));
            i.damage = float(v["damage"].asDouble(0));
            i.speed = v["speed"].asDouble(0.25);
            i.fly = v["fly"].asDouble(0);
            i.xp = int(v["xp"].asInt(0));
            i.category = v["cat"].str("MISC");
            i.mob = v["mob"].asBool();
            i.fireImmune = v["fire_immune"].asBool();
            i.summonable = v["summonable"].asBool(true);
            i.spawn = v["spawn"].str("NO_RESTRICTIONS");
            i.hurtSound = v["hurt"].str();
            i.deathSound = v["death"].str();
            out[i.type] = i;
        }
        return out;
    }();
    return t;
}

// ---- Verhaltensgruppen --------------------------------------------------------

enum class Kind { PASSIVE, HOSTILE, NEUTRAL, RANGED, RANGED_NEUTRAL, CREEPER, SLIME, STILL_RANGED, GUARDIAN, BOSS };

bool isFlyer(const std::string& t) {
    return in(t, {"bat", "parrot", "allay", "bee", "vex", "phantom", "ghast", "happy_ghast", "blaze", "wither", "ender_dragon"});
}
bool isSwimmer(const std::string& t, const Info& i) {
    return i.category.rfind("WATER", 0) == 0 || i.category == "UNDERGROUND_WATER_CREATURE" || i.category == "AXOLOTLS" ||
           in(t, {"guardian", "elder_guardian", "zombie_nautilus", "nautilus"});
}

Kind kindOf(const std::string& t, const Info& i) {
    if (t == "creeper") return Kind::CREEPER;
    if (in(t, {"slime", "magma_cube"})) return Kind::SLIME;
    if (t == "shulker") return Kind::STILL_RANGED;
    if (in(t, {"guardian", "elder_guardian"})) return Kind::GUARDIAN;
    if (in(t, {"wither", "ender_dragon"})) return Kind::BOSS;
    if (in(t, {"skeleton", "stray", "bogged", "parched", "pillager", "illusioner", "witch", "ghast", "blaze", "breeze", "evoker"}))
        return Kind::RANGED;
    if (in(t, {"llama", "trader_llama"})) return Kind::RANGED_NEUTRAL;
    if (in(t, {"enderman", "zombified_piglin", "wolf", "iron_golem", "polar_bear", "panda", "goat", "piglin", "bee", "dolphin",
               "spider", "cave_spider"}))
        return t == "cave_spider" ? Kind::HOSTILE : Kind::NEUTRAL;
    if (i.category == "MONSTER") return Kind::HOSTILE;
    if (in(t, {"zombie_nautilus"})) return Kind::HOSTILE;
    return Kind::PASSIVE;
}

std::string mainHandOf(const std::string& t) {
    if (in(t, {"skeleton", "stray", "bogged", "parched", "illusioner"})) return "minecraft:bow";
    if (t == "pillager") return "minecraft:crossbow";
    if (in(t, {"piglin", "zombified_piglin"})) return "minecraft:golden_sword";
    if (t == "piglin_brute") return "minecraft:golden_axe";
    if (t == "vindicator") return "minecraft:iron_axe";
    if (t == "wither_skeleton") return "minecraft:stone_sword";
    if (t == "vex") return "minecraft:iron_sword";
    return "";
}

bool burnsInDaylight(const std::string& t) {
    return in(t, {"zombie", "zombie_villager", "drowned", "skeleton", "stray", "bogged", "phantom"});
}

bool isNight(Server& s) {
    int64_t t = s.world().time % 24000;
    return t >= 13000 && t <= 23000;
}

float scaledDamage(Server& s, float d) {
    switch (s.config().difficulty) {
        case 0: return 0;
        case 1: return std::min(d / 2 + 1, d);
        case 3: return d * 1.5f;
        default: return d;
    }
}

// ---- Geschosse --------------------------------------------------------------

class Projectile : public Entity {
public:
    Projectile(Server& s, const std::string& kind, int owner_, double px, double py, double pz, float dmg, bool pick)
        : Entity(s, kind, px, py, pz), owner(owner_), damage(dmg), pickup(pick) {
        if (kind == "arrow" || kind == "spectral_arrow" || kind == "trident") gravity = 0.05;
        else if (kind == "snowball" || kind == "egg" || kind == "potion" || kind == "llama_spit" || kind == "ender_pearl") gravity = 0.03;
        else gravity = 0;
        drag = 0.99;
        width = height = kind == "fireball" ? 1.0 : 0.3;
    }
    int owner;
    float damage;
    bool pickup, stuck = false;
    int stuckTicks = 0;

    std::vector<std::pair<std::string, Bytes>> spawnPackets(const Version& v) const override {
        std::vector<std::pair<std::string, Bytes>> out;
        auto t = v.entityType(type);
        if (!t) return out;
        out.emplace_back("spawn_entity", v.spawnEntity(entityId, uuid, *t, x, y, z, yaw, pitch, vx, vy, vz, owner + 1));
        return out;
    }

    void face() {
        double h = std::hypot(vx, vz);
        yaw = float(std::atan2(vx, vz) * 180 / PI);
        pitch = float(std::atan2(vy, h) * 180 / PI);
    }

    void hit(Entity* e, Player* p) {
        std::string k = type;
        if (k == "fireball" || k == "wither_skull") {   // Ghast / Wither: kleine Explosion
            remove();
            vanilla::explode(server, x, y, z, 1.0, server.gameRules().b("mobGriefing"));
            return;
        }
        float d = damage;
        if (p) {
            if (p->damage(d, "projectile", owner) && k == "small_fireball") {
                p->fireTicks = std::max(p->fireTicks, 100);
                p->sendMetadata();
            }
            if (k == "snowball") p->push(vx * 0.2, 0.2, vz * 0.2);
        } else if (auto* m = dynamic_cast<Mob*>(e)) {
            if (k == "snowball" && m->type == "blaze") d = 3;
            if (m->damage(d, owner, "projectile") && k == "small_fireball") m->fireTicks = std::max(m->fireTicks, 100);
        }
        remove();
    }

    void tick() override {
        ++age;
        if (stuck) {   // Pfeil steckt im Block
            if (++stuckTicks > 1200) remove();
            if (pickup)
                for (Player* p : server.players()) {
                    if (p->dead || p->gamemode == 3 || std::abs(p->x - x) > 1.5 || std::abs(p->z - z) > 1.5 || std::abs(p->y + 0.9 - y) > 2) continue;
                    if (p->addItem("minecraft:arrow", 1) == 0) {
                        Writer w;
                        w.varint(entityId).varint(p->entityId).varint(1);
                        server.broadcastPacket("collect", w.out);
                        remove();
                        return;
                    }
                }
            return;
        }
        if (age > 400 && gravity == 0) {   // Feuerbälle fliegen nicht ewig
            remove();
            return;
        }
        double speed = std::sqrt(vx * vx + vy * vy + vz * vz);
        int steps = std::max(1, int(std::ceil(speed / 0.4)));
        for (int s = 0; s < steps && !removed; ++s) {
            double nx = x + vx / steps, ny = y + vy / steps, nz = z + vz / steps;
            // Treffer: Spieler und Mobs (der Schütze erst nach ein paar Ticks)
            for (Player* p : server.players()) {
                if (p->dead || p->gamemode == 3 || (p->entityId == owner && age < 10)) continue;
                if (std::abs(p->x - nx) < 0.6 && std::abs(p->z - nz) < 0.6 && ny > p->y - 0.2 && ny < p->y + 2.0) {
                    x = nx;
                    y = ny;
                    z = nz;
                    hit(nullptr, p);
                    return;
                }
            }
            for (Entity* e : server.entities()) {
                auto* m = dynamic_cast<Mob*>(e);
                if (!m || m->dead || (m->entityId == owner && age < 10)) continue;
                if (std::abs(m->x - nx) < m->width / 2 + 0.3 && std::abs(m->z - nz) < m->width / 2 + 0.3 && ny > m->y - 0.2 &&
                    ny < m->y + m->height + 0.1) {
                    hit(m, nullptr);
                    return;
                }
            }
            int st = server.world().getBlock(fl(nx), fl(ny), fl(nz));
            if (data::SOLID[size_t(st)]) {
                if (type == "arrow" || type == "trident" || type == "spectral_arrow") {
                    stuck = true;
                    vx = vy = vz = 0;
                    server.playSound("minecraft:entity.arrow.hit", x, y, z, 1.0f, 1.0f, 6);
                } else if (type == "fireball" || type == "wither_skull") {
                    hit(nullptr, nullptr);
                } else {
                    if (type == "small_fireball") {   // Blaze-Feuerball zündet den Block davor an
                        bool ok;
                        vanilla::ignite(server, fl(x), fl(y), fl(z), ok);
                    }
                    remove();
                }
                return;
            }
            x = nx;
            y = ny;
            z = nz;
        }
        vy -= gravity;
        vx *= drag;
        vy *= drag;
        vz *= drag;
        if (server.world().getBlock(fl(x), fl(y), fl(z)) == data::blockState("water")) {
            vx *= 0.6;
            vy *= 0.6;
            vz *= 0.6;
        }
        face();
        if (y < MIN_Y - 64) remove();
    }
};

// ---- KI -------------------------------------------------------------------------

double horizontalDist(const Entity& a, double x, double z) { return std::hypot(a.x - x, a.z - z); }

Player* nearestPlayer(Mob& m, double range) {
    Player* best = nullptr;
    double bd = range;
    for (Player* p : m.server.players()) {
        if (p->dead || !p->isSurvival()) continue;
        double d = m.distanceTo(p->x, p->y, p->z);
        if (d < bd) {
            best = p;
            bd = d;
        }
    }
    return best;
}

void move3d(Mob& m, double tx, double ty, double tz, double speed) {
    double dx = tx - m.x, dy = ty - m.y, dz = tz - m.z;
    double d = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (d < 0.2) return;
    m.vx = dx / d * speed;
    m.vy = dy / d * speed;
    m.vz = dz / d * speed;
    m.lookAt(tx, tz);
}

double walkSpeed(const Info& i) { return std::max(0.05, i.speed * 0.5); }

void wanderGround(Mob& m, const Info& i) {
    if (m.inWater) m.jump();
    if (m.hasWander && m.age < m.wanderUntil) {
        m.walkTowards(m.wanderX, m.wanderZ, walkSpeed(i) * 0.6);
        if (std::hypot(m.wanderX - m.x, m.wanderZ - m.z) < 0.5) m.hasWander = false;
    } else if (randDouble() < 0.02) {
        m.hasWander = true;
        m.wanderX = m.x + randDouble(-8, 8);
        m.wanderZ = m.z + randDouble(-8, 8);
        m.wanderUntil = m.age + randInt(60, 160);
    } else {
        m.vx *= 0.5;
        m.vz *= 0.5;
    }
}

void wanderAir(Mob& m, const Info& i) {
    double sp = i.fly > 0 ? i.fly * 0.25 : 0.08;
    if (!m.hasWander || m.age >= m.wanderUntil || m.distanceTo(m.wanderX, m.wanderY, m.wanderZ) < 1) {
        m.hasWander = true;
        m.wanderX = m.x + randDouble(-10, 10);
        m.wanderZ = m.z + randDouble(-10, 10);
        int ground = m.server.world().highestBlock(fl(m.wanderX), fl(m.wanderZ));
        m.wanderY = std::max(double(ground) + 2, std::min(m.y + randDouble(-4, 4), double(ground) + (m.type == "ghast" ? 20 : 10)));
        m.wanderUntil = m.age + randInt(60, 200);
    }
    move3d(m, m.wanderX, m.wanderY, m.wanderZ, sp);
}

void wanderWater(Mob& m, const Info& i) {
    if (!m.inWater) {   // an Land: zappeln
        if (m.onGround && randDouble() < 0.1) {
            m.vy = 0.3;
            m.vx = randDouble(-0.15, 0.15);
            m.vz = randDouble(-0.15, 0.15);
        }
        return;
    }
    double sp = std::max(0.05, i.speed * 0.4);
    if (!m.hasWander || m.age >= m.wanderUntil || m.distanceTo(m.wanderX, m.wanderY, m.wanderZ) < 1) {
        static int water = data::blockState("water");
        for (int tries = 0; tries < 8; ++tries) {
            double tx = m.x + randDouble(-6, 6), ty = m.y + randDouble(-3, 3), tz = m.z + randDouble(-6, 6);
            if (m.server.world().getBlock(fl(tx), fl(ty), fl(tz)) == water) {
                m.hasWander = true;
                m.wanderX = tx;
                m.wanderY = ty;
                m.wanderZ = tz;
                m.wanderUntil = m.age + randInt(40, 120);
                break;
            }
        }
    }
    if (m.hasWander) move3d(m, m.wanderX, m.wanderY, m.wanderZ, sp);
}

void wander(Mob& m, const Info& i) {
    if (m.flying) wanderAir(m, i);
    else if (m.swimming) wanderWater(m, i);
    else wanderGround(m, i);
}

void chase(Mob& m, const Info& i, Player& t, double speedMul = 1.0) {
    if (m.flying) move3d(m, t.x, t.y + 1, t.z, (i.fly > 0 ? i.fly * 0.35 : 0.15) * speedMul);
    else if (m.swimming && m.inWater) move3d(m, t.x, t.y + 0.5, t.z, std::max(0.08, i.speed * 0.5) * speedMul);
    else m.walkTowards(t.x, t.z, walkSpeed(i) * speedMul);
}

bool meleeAttack(Mob& m, const Info& i, Player& t) {
    double reach = m.width / 2 + 1.1;
    if (horizontalDist(m, t.x, t.z) > reach || std::abs(t.y - m.y) > std::max(2.0, m.height) || m.attackCooldown > 0) return false;
    m.attackCooldown = 20;
    float dmg = scaledDamage(m.server, i.damage > 0 ? i.damage : 2.0f);
    if (dmg > 0) t.damage(dmg, "attack", m.entityId);
    int id = m.entityId;   // Arm schwingen
    m.server.broadcastVersioned([id](const Version& v) { return v.swing(id, false); });
    return true;
}

void shootAt(Mob& m, Player& t, const std::string& kind, double speed, float dmg, double inaccuracy = 0.05) {
    double sx = m.x, sy = m.y + m.height * 0.85, sz = m.z;
    double dx = t.x - sx, dy = t.y + 1.2 - sy, dz = t.z - sz;
    double h = std::hypot(dx, dz);
    if (kind == "arrow" || kind == "snowball" || kind == "potion" || kind == "llama_spit") dy += h * 0.2;   // Bogen nach oben
    double d = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (d < 0.01) return;
    double vx = dx / d * speed + randDouble(-inaccuracy, inaccuracy), vy = dy / d * speed + randDouble(-inaccuracy, inaccuracy),
           vz = dz / d * speed + randDouble(-inaccuracy, inaccuracy);
    shoot(m.server, kind, m.entityId, sx + dx / d * 0.8, sy, sz + dz / d * 0.8, vx, vy, vz, dmg);
    m.lookAt(t.x, t.z);
}

void rangedAttack(Mob& m, const Info& i, Player& t) {
    double dist = m.distanceTo(t.x, t.y, t.z);
    m.lookAt(t.x, t.z);
    if (m.shootCooldown > 0) --m.shootCooldown;
    const std::string& k = m.type;
    // Abstand halten
    if (!m.flying && !m.swimming) {
        if (dist > 12) m.walkTowards(t.x, t.z, walkSpeed(i));
        else if (dist < 5) m.walkTowards(m.x * 2 - t.x, m.z * 2 - t.z, walkSpeed(i));
        else {
            m.vx *= 0.5;
            m.vz *= 0.5;
        }
    } else if (m.flying) {
        double hover = k == "ghast" ? 8 : 3;
        move3d(m, t.x + (m.x - t.x) * 0.3, t.y + hover, t.z + (m.z - t.z) * 0.3, (i.fly > 0 ? i.fly * 0.2 : 0.06));
    }
    if (m.shootCooldown > 0 || dist > (k == "ghast" ? 40 : 18)) return;
    Server& s = m.server;
    if (k == "ghast") {
        shootAt(m, t, "fireball", 1.0, 6);
        s.playSound("minecraft:entity.ghast.shoot", m.x, m.y, m.z, 1, 1, 5);
        m.shootCooldown = 60;
    } else if (k == "blaze") {
        shootAt(m, t, "small_fireball", 1.0, 5, 0.1);
        s.playSound("minecraft:entity.blaze.shoot", m.x, m.y, m.z, 1, 1, 5);
        m.shootCooldown = m.fuse++ % 3 == 2 ? 80 : 6;   // Salven aus drei Kugeln
    } else if (k == "witch") {
        shootAt(m, t, "potion", 0.75, scaledDamage(s, 6));
        m.shootCooldown = 60;
    } else if (k == "evoker") {   // Fangzähne: Schaden aus der Ferne
        if (dist < 12) t.damage(scaledDamage(s, 6), "magic", m.entityId);
        s.playSound("minecraft:entity.evoker_fangs.attack", t.x, t.y, t.z, 1, 1, 5);
        m.shootCooldown = 80;
    } else if (k == "breeze") {
        shootAt(m, t, "snowball", 1.2, scaledDamage(s, 3));
        m.shootCooldown = 40;
    } else if (k == "llama" || k == "trader_llama") {
        shootAt(m, t, "llama_spit", 1.5, 1);
        m.shootCooldown = 40;
    } else if (k == "shulker") {
        shootAt(m, t, "shulker_bullet", 0.6, scaledDamage(s, 4), 0.02);
        m.shootCooldown = randInt(20, 110);
    } else {   // Bogen und Armbrust
        shootAt(m, t, "arrow", 1.6, scaledDamage(s, 3.0f), 0.1);
        s.playSound("minecraft:entity.skeleton.shoot", m.x, m.y, m.z, 1, float(1.0 / (randDouble(0.8, 1.2))), 5);
        m.shootCooldown = s.config().difficulty >= 3 ? 20 : 40;
    }
}

void ai(Mob& m) {
    const Info* ip = info(m.type);
    if (!ip) return;
    const Info& i = *ip;
    Server& s = m.server;
    Kind k = kindOf(m.type, i);
    bool night = isNight(s);
    if (m.attackCooldown > 0) --m.attackCooldown;
    if (m.angry > 0) --m.angry;
    if (m.hurtCooldown == 9 && m.lastAttacker && s.playerById(m.lastAttacker)) m.angry = 400;   // gerade getroffen

    // Untote brennen bei Tag unter freiem Himmel
    if (burnsInDaylight(m.type) && !night && !m.inWater && s.world().highestBlock(fl(m.x), fl(m.z)) < m.y && !i.fireImmune)
        m.fireTicks = std::max(m.fireTicks, 60);
    if (i.fireImmune) m.fireTicks = 0;

    // Ziel suchen
    Player* target = nullptr;
    bool hostileNow = k == Kind::HOSTILE || k == Kind::RANGED || k == Kind::CREEPER || k == Kind::SLIME || k == Kind::STILL_RANGED ||
                      k == Kind::GUARDIAN || k == Kind::BOSS;
    if (m.type == "spider" && night) hostileNow = true;   // Spinnen sind nachts feindlich
    if (s.config().difficulty == 0) hostileNow = false;
    if (hostileNow) target = nearestPlayer(m, k == Kind::BOSS ? 64 : (k == Kind::RANGED || k == Kind::GUARDIAN) ? 24 : 16);
    if (!target && m.angry > 0 && (k == Kind::NEUTRAL || k == Kind::RANGED_NEUTRAL || k == Kind::PASSIVE)) {
        Player* a = s.playerById(m.lastAttacker);
        if (a && !a->dead && a->isSurvival() && m.distanceTo(a->x, a->y, a->z) < 32 && k != Kind::PASSIVE) target = a;
    }

    if (k == Kind::PASSIVE) {   // friedlich: bei Treffer wegrennen
        Player* a = m.angry > 0 ? s.playerById(m.lastAttacker) : nullptr;
        if (a && m.angry > 340) {
            double dx = m.x - a->x, dz = m.z - a->z, d = std::max(0.1, std::hypot(dx, dz));
            if (m.flying || (m.swimming && m.inWater)) move3d(m, m.x + dx / d * 4, m.y + 0.5, m.z + dz / d * 4, 0.2);
            else m.walkTowards(m.x + dx / d * 4, m.z + dz / d * 4, walkSpeed(i) * 1.4);
            return;
        }
        wander(m, i);
        return;
    }
    if (!target) {
        m.fuse = 0;
        wander(m, i);
        return;
    }

    switch (k) {
        case Kind::CREEPER: {
            double dist = m.distanceTo(target->x, target->y, target->z);
            if (dist < 3 || (m.fuse > 0 && dist < 7)) {
                if (m.fuse == 0) s.playSound("minecraft:entity.creeper.primed", m.x, m.y, m.z, 1.0f, 0.5f, 5);
                ++m.fuse;
                m.vx = m.vz = 0;
                m.lookAt(target->x, target->z);
                if (m.fuse >= 30) {
                    json::Object ed;
                    ed["entity"] = api::entitySnap(&m);
                    m.remove();
                    if (!api::fire(s, "entity_explode", nullptr, std::move(ed)))
                        vanilla::explode(s, m.x, m.y + 0.5, m.z, 3.0, s.gameRules().b("mobGriefing"));
                }
                return;
            }
            m.fuse = 0;
            chase(m, i, *target);
            return;
        }
        case Kind::SLIME: {   // hüpfen
            if (m.onGround && m.attackCooldown <= 0 && randDouble() < 0.08) {
                double dx = target->x - m.x, dz = target->z - m.z, d = std::max(0.1, std::hypot(dx, dz));
                m.vy = 0.42;
                m.vx = dx / d * 0.25;
                m.vz = dz / d * 0.25;
                m.lookAt(target->x, target->z);
            }
            meleeAttack(m, i, *target);
            return;
        }
        case Kind::GUARDIAN: {   // Laser: nach 2 Sekunden Zielen Schaden
            wander(m, i);
            if (m.distanceTo(target->x, target->y, target->z) < 15) {
                if (++m.fuse >= 40) {
                    target->damage(scaledDamage(s, m.type == "elder_guardian" ? 8 : 6), "magic", m.entityId);
                    m.fuse = 0;
                }
            } else {
                m.fuse = 0;
            }
            return;
        }
        case Kind::STILL_RANGED:
            m.vx = m.vz = 0;
            rangedAttack(m, i, *target);
            return;
        case Kind::BOSS:
            if (m.type == "wither") {
                rangedAttack(m, i, *target);
                if (m.shootCooldown <= 0) {
                    shootAt(m, *target, "wither_skull", 1.0, 8);
                    m.shootCooldown = 30;
                }
            } else {   // Enderdrache: fliegt auf dich zu
                chase(m, i, *target, 2.0);
                meleeAttack(m, i, *target);
            }
            return;
        case Kind::RANGED:
        case Kind::RANGED_NEUTRAL:
            rangedAttack(m, i, *target);
            return;
        default:   // Nahkampf (feindlich oder gereizt neutral)
            chase(m, i, *target);
            meleeAttack(m, i, *target);
            if (m.type == "spider" && m.horizontalCollision) m.vy = 0.2;   // Spinnen klettern
            return;
    }
}

// ---- Spawnen ------------------------------------------------------------------

struct Weighted {
    const char* type;
    int weight;
};
const Weighted DAY_LAND[] = {{"pig", 10}, {"cow", 8}, {"sheep", 12}, {"chicken", 10}, {"rabbit", 4}, {"horse", 2},
                             {"donkey", 1}, {"goat", 1}, {"fox", 2}, {"wolf", 2}, {"llama", 1}, {"cat", 1}};
const Weighted NIGHT[] = {{"zombie", 95}, {"skeleton", 100}, {"spider", 100}, {"creeper", 100}, {"enderman", 10}, {"witch", 5},
                          {"slime", 3}, {"zombie_villager", 5}};
const Weighted WATER[] = {{"cod", 10}, {"salmon", 6}, {"squid", 4}, {"tropical_fish", 4}, {"dolphin", 1}, {"drowned", 2}};

template <size_t N>
std::string pick(const Weighted (&list)[N]) {
    int total = 0;
    for (const auto& w : list)
        if (info(w.type)) total += w.weight;
    int r = randInt(0, std::max(0, total - 1));
    for (const auto& w : list) {
        if (!info(w.type)) continue;
        r -= w.weight;
        if (r < 0) return w.type;
    }
    return list[0].type;
}

void trySpawn(Server& s) {
    const Config& cfg = s.config();
    std::vector<Player*> players;
    for (Player* p : s.players())
        if (!p->dead && p->gamemode != 3) players.push_back(p);
    if (players.empty()) return;
    int monsters = 0, creatures = 0, water = 0, ambient = 0;
    for (Entity* e : s.entities()) {
        const Info* i = e->isMob() ? info(e->type) : nullptr;
        if (!i) continue;
        if (i->category == "MONSTER") ++monsters;
        else if (i->category == "CREATURE") ++creatures;
        else if (i->category.rfind("WATER", 0) == 0) ++water;
        else if (i->category == "AMBIENT") ++ambient;
    }
    Player* p = players[size_t(randInt(0, int(players.size()) - 1))];
    double a = randDouble(0, 2 * PI), dist = randDouble(24, 48);
    int x = fl(p->x + std::cos(a) * dist), z = fl(p->z + std::sin(a) * dist);
    if (!p->hasChunk(x >> 4, z >> 4)) return;
    World& w = s.world();
    static const int WATER_ST = data::blockState("water"), GRASS = data::blockState("grass_block");
    int top = w.highestBlock(x, z);
    int ground = w.getBlock(x, top, z);
    bool night = isNight(s);
    std::string kind;
    int y = top + 1;
    if (ground == WATER_ST) {
        if (water >= 8 || !cfg.passiveMobs) return;
        kind = pick(WATER);
        if (kind == "drowned" && (!night || !cfg.hostileMobs || cfg.difficulty == 0)) return;
        y = top - randInt(0, 3);
        if (w.getBlock(x, y, z) != WATER_ST) return;
    } else if (randDouble() < 0.35) {   // Höhle: Monster und Fledermäuse auch tagsüber
        int cy = randInt(MIN_Y + 4, std::max(MIN_Y + 5, top - 8));
        if (w.getBlock(x, cy, z) != 0 || w.getBlock(x, cy + 1, z) != 0 || !data::SOLID[size_t(w.getBlock(x, cy - 1, z))]) return;
        y = cy;
        if (randDouble() < 0.2) {
            if (ambient >= 4) return;
            kind = "bat";
        } else {
            if (!cfg.hostileMobs || !s.gameRules().b("spawnMonsters") || cfg.difficulty == 0 || monsters >= cfg.maxHostile) return;
            kind = pick(NIGHT);
        }
    } else {
        if (w.getBlock(x, top + 1, z) != 0 || w.getBlock(x, top + 2, z) != 0 || !data::SOLID[size_t(ground)]) return;
        if (night) {
            if (!cfg.hostileMobs || !s.gameRules().b("spawnMonsters") || cfg.difficulty == 0 || monsters >= cfg.maxHostile) return;
            kind = pick(NIGHT);
        } else {
            if (ground != GRASS || !cfg.passiveMobs || creatures >= cfg.maxPassive) return;
            kind = pick(DAY_LAND);
        }
    }
    int count = info(kind) && info(kind)->category == "CREATURE" ? randInt(1, 3) : (kind == "cod" || kind == "salmon" || kind == "tropical_fish") ? randInt(2, 4) : 1;
    for (int n = 0; n < count; ++n) {
        if (s.bridge().listens("creature_spawn")) {
            json::Object cd, ent;
            ent["id"] = -1;
            ent["type"] = kind;
            ent["x"] = x + 0.5;
            ent["y"] = double(y);
            ent["z"] = z + 0.5;
            cd["entity"] = json::Value(std::move(ent));
            if (api::fire(s, "creature_spawn", nullptr, std::move(cd))) continue;
        }
        spawn(s, kind, x + 0.5 + randDouble(-1, 1), y, z + 0.5 + randDouble(-1, 1));
    }
}

void despawn(Server& s) {
    auto players = s.players();
    bool peaceful = s.config().difficulty == 0;
    for (Entity* e : s.entities()) {
        const Info* i = e->isMob() ? info(e->type) : nullptr;
        if (!i) continue;
        if (peaceful && i->category == "MONSTER" && !in(e->type, {"ender_dragon", "wither"})) {   // friedlich: keine Monster
            e->remove();
            continue;
        }
        double best = 1e18;
        for (Player* p : players) best = std::min(best, e->distanceTo(p->x, p->y, p->z));
        bool persistent = i->category == "CREATURE" || in(e->type, {"ender_dragon", "wither", "iron_golem", "snow_golem"});
        if ((!persistent && best > 128) || best > 256 || (!persistent && best > 32 && randInt(0, 40) == 0 && best > 64)) e->remove();
    }
}

}  // namespace

const Info* info(const std::string& type) {
    auto it = table().find(removePrefix(type, "minecraft:"));
    return it == table().end() ? nullptr : &it->second;
}

bool isMob(const std::string& type) {
    const Info* i = info(type);
    return i && i->mob && i->summonable;
}

std::vector<std::string> types() {
    std::vector<std::string> out;
    for (const auto& [k, i] : table())
        if (i.mob && i.summonable) out.push_back(k);
    std::sort(out.begin(), out.end());
    return out;
}

Mob* spawn(Server& s, const std::string& rawType, double x, double y, double z) {
    std::string type = removePrefix(rawType, "minecraft:");
    const Info* i = info(type);
    if (!i || !i->mob) return nullptr;
    Mob* m = s.spawnMob(type, x, y, z, i->health > 0 ? i->health : 10);
    m->width = i->w;
    m->height = i->h;
    m->flying = isFlyer(type);
    m->swimming = isSwimmer(type, *i);
    if (m->flying) {
        m->gravity = 0;
        m->drag = 0.91;
    }
    m->mainHand = mainHandOf(type);
    if (!m->mainHand.empty()) {   // Waffe in der Hand zeigen (Bogen, Schwert ...)
        s.broadcastPacketV("entity_equipment", [m](const Version& v) {
            Writer w;
            w.varint(m->entityId).u8(0).raw(v.slot(m->mainHand, 1));
            return w.out;
        });
    }
    m->ai = ai;
    return m;
}

void tick(Server& s) {
    int64_t t = s.tick();
    if (t % 10 == 0 && s.gameRules().b("doMobSpawning")) trySpawn(s);
    if (t % 100 == 0) despawn(s);
}

void onDeath(Server& s, Mob& mob, int killer) {
    const Info* i = info(mob.type);
    Player* kp = s.playerById(killer);
    bool loot = s.gameRules().b("doMobLoot");
    std::vector<std::pair<std::string, int>> drops;
    if (loot) drops = vanilla::entityDrops(s, "minecraft:" + mob.type, kp ? kp->heldItem() : "", kp != nullptr, mob.fireTicks > 0);
    if (s.bridge().listens("entity_death")) {   // Plugins dürfen die Drops ändern
        json::Object d;
        d["entity"] = api::entitySnap(&mob);
        json::Array da;
        for (const auto& [item, n] : drops) da.push_back(json::Array{item, n});
        d["drops"] = json::Value(std::move(da));
        if (kp) d["killer"] = s.bridge().snap(kp);
        json::Value r = s.bridge().event("entity_death", std::move(d));
        if (r["drops"].isArray()) {
            drops.clear();
            for (const auto& v : r["drops"].asArray())
                if (v.isArray() && v.size() >= 2 && v[0].isString()) drops.emplace_back(v[0].asString(), int(v[1].asInt(1)));
        }
    }
    for (const auto& [item, n] : drops) s.dropItem(mob.x, mob.y + 0.5, mob.z, item, n);
    if (loot && kp && i && i->xp > 0) kp->giveXp(i->xp);
}

void shoot(Server& s, const std::string& kind, int owner, double x, double y, double z, double vx, double vy, double vz, float damage,
           bool pickup) {
    auto p = std::make_unique<Projectile>(s, kind, owner, x, y, z, damage, pickup);
    p->vx = vx;
    p->vy = vy;
    p->vz = vz;
    p->face();
    s.spawnEntity(std::move(p));
}

bool releaseBow(Player& p) {
    if (!p.usingItem || data::baseItem(p.usingItem->item) != "minecraft:bow" || p.dead) return false;
    Server& s = p.server;
    double ticks = double(s.tick() - p.usingItem->start);
    std::string bow = p.usingItem->item;
    p.usingItem.reset();
    double f = ticks / 20.0;
    f = (f * f + f * 2.0) / 3.0;   // Vanilla-Spannkraft
    if (f < 0.1) return false;
    f = std::min(1.0, f);
    bool infinite = p.gamemode == 1 || vanilla::enchantLevel(bow, "infinity") > 0;
    bool hasArrow = false;
    for (const auto& [slot, st] : p.inventory)
        if (st.first == "minecraft:arrow") hasArrow = true;
    if (!hasArrow && p.gamemode != 1) return false;
    if (!infinite) p.removeItem("minecraft:arrow", 1);
    double ry = p.yaw * PI / 180, rp = p.pitch * PI / 180;
    double speed = f * 3.0;
    double vx = -std::sin(ry) * std::cos(rp) * speed, vy = -std::sin(rp) * speed, vz = std::cos(ry) * std::cos(rp) * speed;
    float dmg = float(std::ceil(speed * 2.0));   // voll gespannt: 6
    int power = vanilla::enchantLevel(bow, "power");
    if (power) dmg += float(power) * 0.5f + 0.5f;
    if (f >= 1.0 && randInt(0, 4) == 0) dmg += float(randInt(0, int(dmg / 2) + 1));   // kritischer Schuss
    shoot(s, "arrow", p.entityId, p.x, p.y + 1.52, p.z, vx, vy, vz, dmg, !infinite);
    s.playSound("minecraft:entity.arrow.shoot", p.x, p.y, p.z, 1.0f, float(1.0 / (randDouble(0.8, 1.2) + f * 0.5)), 7);
    return true;
}

}  // namespace mobs
