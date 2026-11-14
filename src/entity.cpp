#include "entity.hpp"

#include <algorithm>
#include <cmath>

#include "buffer.hpp"
#include "api.hpp"
#include "data.hpp"
#include "player.hpp"
#include "server.hpp"
#include "vanilla.hpp"
#include "version.hpp"

namespace {
struct Size {
    const char* type;
    double w, h;
};
const Size SIZES[] = {
    {"item", 0.25, 0.25}, {"pig", 0.9, 0.9},      {"cow", 0.9, 1.4},       {"sheep", 0.9, 1.3},
    {"chicken", 0.4, 0.7}, {"zombie", 0.6, 1.95}, {"spider", 1.4, 0.9},    {"creeper", 0.6, 1.7},
    {"skeleton", 0.6, 1.99}, {"tnt", 0.98, 0.98}, {"falling_block", 0.98, 0.98},
};

int waterState() {
    static int s = data::blockState("water");
    return s;
}
int lavaState() {
    static int s = data::blockState("lava");
    return s;
}
int fl(double v) { return int(std::floor(v)); }
}  // namespace

// ---- Entity -----------------------------------------------------------------

Entity::Entity(Server& s, const std::string& t, double px, double py, double pz)
    : server(s), type(removePrefix(t, "minecraft:")), entityId(s.nextEntityId()), uuid(Uuid::random()), x(px), y(py), z(pz),
      sentX_(px), sentY_(py), sentZ_(pz) {
    for (const auto& sz : SIZES)
        if (type == sz.type) {
            width = sz.w;
            height = sz.h;
        }
}

std::vector<std::pair<std::string, Bytes>> Entity::spawnPackets(const Version& v) const {
    std::vector<std::pair<std::string, Bytes>> out;
    auto typeId = v.entityType(type);
    if (!typeId) return out;   // gibt es in dieser Client-Version nicht
    out.emplace_back("spawn_entity", v.spawnEntity(entityId, uuid, *typeId, x, y, z, yaw, pitch, vx, vy, vz));
    Writer head;
    head.varint(entityId).angle(yaw);
    out.emplace_back("head_rotation", head.out);
    if (fireTicks > 0) out.emplace_back("entity_metadata", v.flagsMetadata(entityId, flags()));
    return out;
}

bool Entity::visibleTo(const Player* p) const { return p->version->entityType(type).has_value(); }

double Entity::distanceTo(double ox, double oy, double oz) const {
    return std::sqrt((x - ox) * (x - ox) + (y - oy) * (y - oy) + (z - oz) * (z - oz));
}

void Entity::sync() {
    auto only = [this](Player* p) { return visibleTo(p); };
    if (std::abs(x - sentX_) + std::abs(y - sentY_) + std::abs(z - sentZ_) > 0.001 || std::abs(yaw - sentYaw_) > 1) {
        sentX_ = x;
        sentY_ = y;
        sentZ_ = z;
        sentYaw_ = yaw;
        server.broadcastVersioned([this](const Version& v) { return v.entityPosition(entityId, x, y, z, yaw, pitch, onGround); },
                                  nullptr, only);
        Writer head;
        head.varint(entityId).angle(yaw);
        server.broadcastPacket("head_rotation", head.out, nullptr, only);
    }
    int f = flags();
    if (f != sentFlags_) {
        sentFlags_ = f;
        server.broadcastPacketV("entity_metadata", [this, f](const Version& v) { return v.flagsMetadata(entityId, f); },
                                nullptr, only);
    }
}

void Entity::remove() {
    if (removed) return;
    removed = true;
    Writer w;
    w.varint(1).varint(entityId);
    server.broadcastPacket("remove_entities", w.out);
}

bool Entity::solid(int bx, int by, int bz) const { return data::SOLID[size_t(server.world().getBlock(bx, by, bz))] == 1; }

bool Entity::collides(double nx, double ny, double nz) const {
    double hw = width / 2;
    for (int bx = fl(nx - hw); bx <= fl(nx + hw - 1e-6); ++bx)
        for (int bz = fl(nz - hw); bz <= fl(nz + hw - 1e-6); ++bz)
            for (int by = fl(ny); by <= fl(ny + height - 1e-6); ++by)
                if (solid(bx, by, bz)) return true;
    return false;
}

void Entity::move(double dx, double dy, double dz) {
    onGround = false;
    if (dy != 0) {
        if (collides(x, y + dy, z)) {
            if (dy < 0) {   // auf die Oberkante des getroffenen Blocks setzen
                double ny = std::floor(y + dy) + 1.0;
                if (ny <= y && !collides(x, ny, z)) y = ny;
                onGround = true;
            }
            vy = 0;
        } else {
            y += dy;
        }
    }
    if (dx != 0) {
        if (collides(x + dx, y, z)) {
            vx = 0;
            horizontalCollision = true;
        } else {
            x += dx;
        }
    }
    if (dz != 0) {
        if (collides(x, y, z + dz)) {
            vz = 0;
            horizontalCollision = true;
        } else {
            z += dz;
        }
    }
}

void Entity::tick() {
    ++age;
    horizontalCollision = false;
    int feet = server.world().getBlock(fl(x), fl(y), fl(z));
    inWater = feet == waterState();
    inLava = feet == lavaState();
    if (inLava) fireTicks = std::max(fireTicks, 100);
    if (inWater) {
        vy = vy * 0.8 + 0.01;
        vx *= 0.8;
        vz *= 0.8;
        fireTicks = 0;
    } else {
        vy -= gravity;
    }
    move(vx, vy, vz);
    double friction = onGround ? 0.6 * 0.91 : 0.91;
    vx *= friction;
    vz *= friction;
    vy *= drag;
    if (fireTicks > 0) --fireTicks;
    if (y < -128) remove();
}

// ---- Items ------------------------------------------------------------------

ItemEntity::ItemEntity(Server& s, double px, double py, double pz, const std::string& it, int c, int delay)
    : Entity(s, "item", px, py, pz), item(ns(it)), count(c), pickupDelay(delay) {
    gravity = 0.04;
}

std::vector<std::pair<std::string, Bytes>> ItemEntity::spawnPackets(const Version& v) const {
    auto out = Entity::spawnPackets(v);
    if (!out.empty()) out.emplace_back("entity_metadata", v.itemMetadata(entityId, item, count));
    return out;
}

void ItemEntity::tick() {
    Entity::tick();
    if (removed) return;
    if (age > 6000) {   // nach 5 Minuten verschwinden
        remove();
        return;
    }
    if (pickupDelay > 0) {
        --pickupDelay;
        return;
    }
    // gleiche Items in der Nähe zusammenlegen (wie Vanilla), spart Pakete
    if (age % 20 == 0) {
        for (Entity* e : server.entities()) {
            auto* o = dynamic_cast<ItemEntity*>(e);
            if (!o || o == this || o->removed || o->item != item || o->pickupDelay > 0) continue;
            if (std::abs(o->x - x) > 0.75 || std::abs(o->y - y) > 0.5 || std::abs(o->z - z) > 0.75) continue;
            if (count + o->count > data::stackSize(item)) continue;
            count += o->count;
            o->remove();
            server.broadcastPacketV("entity_metadata", [this](const Version& v) { return v.itemMetadata(entityId, item, count); });
        }
    }
    for (Player* p : server.players()) {
        if (p->dead || p->gamemode == 3) continue;
        if (std::abs(p->x - x) < 1.5 && std::abs(p->z - z) < 1.5 && y - p->y > -0.5 && y - p->y < 2.3) {
            json::Object pd;
            pd["item"] = json::Array{item, count};
            pd["entity"] = entityId;
            if (api::fire(server, "player_pickup_item", p, std::move(pd))) continue;
            int left = p->addItem(item, count);
            int taken = count - left;
            if (taken <= 0) continue;
            Writer w;
            w.varint(entityId).varint(p->entityId).varint(taken);
            server.broadcastPacket("collect", w.out);
            server.playSound("minecraft:entity.item.pickup", x, y, z, 0.2f, float(1.4 + randDouble() * 0.6), 7);
            if (left > 0) {
                count = left;
                server.broadcastPacketV("entity_metadata", [this](const Version& v) { return v.itemMetadata(entityId, item, count); });
            } else {
                remove();
            }
            return;
        }
    }
}

// ---- Mobs -------------------------------------------------------------------

Mob::Mob(Server& s, const std::string& t, double px, double py, double pz, float hp)
    : Entity(s, t, px, py, pz), health(hp), maxHealth(hp) {
    yaw = float(randDouble(0, 360));
}

void Mob::tick() {
    if (dead) {
        if (++deathTicks >= 20) remove();
        return;
    }
    if (hurtCooldown > 0) --hurtCooldown;
    if (ai) ai(*this);
    if (removed) return;
    Entity::tick();
    if (inLava) damage(4.0f, 0, "lava");
    else if (fireTicks > 0 && fireTicks % 20 == 0) damage(1.0f, 0, "fire");
}

void Mob::walkTowards(double tx, double tz, double speed) {
    double dx = tx - x, dz = tz - z;
    double dist = std::hypot(dx, dz);
    if (dist < 0.1) return;
    vx = dx / dist * speed;
    vz = dz / dist * speed;
    yaw = float(std::atan2(-dx, dz) * 180.0 / 3.14159265358979);
    if (horizontalCollision && onGround) jump();
}

void Mob::jump() {
    if (onGround || inWater) vy = 0.42;
}

void Mob::lookAt(double tx, double tz) { yaw = float(std::atan2(-(tx - x), tz - z) * 180.0 / 3.14159265358979); }

bool Mob::damage(float amount, int attacker, const std::string& cause) {
    if (dead || amount <= 0 || (hurtCooldown > 0 && amount <= lastDamage)) return false;
    if (server.bridge().listens("entity_damage")) {   // Plugins: EntityDamage(ByEntity)Event
        json::Object d;
        d["entity"] = api::entitySnap(this);
        d["amount"] = double(amount);
        d["cause"] = cause;
        if (Player* a = server.playerById(attacker)) d["attacker"] = server.bridge().snap(a);
        else if (Entity* e = attacker ? server.getEntity(attacker) : nullptr) d["attacker"] = api::entitySnap(e);
        json::Value r = server.bridge().event("entity_damage", std::move(d));
        if (r["cancelled"].asBool()) return false;
        if (r["amount"].isNumber()) amount = float(r["amount"].asDouble());
        if (amount <= 0) return false;
    }
    // während der Unverwundbarkeit zählt nur der Teil über dem letzten Treffer (wie Vanilla)
    health -= hurtCooldown > 0 ? amount - lastDamage : amount;
    if (hurtCooldown <= 0) hurtCooldown = 10;
    lastDamage = amount;
    if (attacker) lastAttacker = attacker;
    auto only = [this](Player* p) { return visibleTo(p); };
    Writer hurt;
    hurt.varint(entityId).f32(yaw);
    server.broadcastPacket("hurt_animation", hurt.out, nullptr, only);
    server.playSound("minecraft:entity." + type + ".hurt", x, y, z, 1.0f, 1.0f, 6);
    double ax, ay, az;
    if (attacker && server.entityPosition(attacker, ax, ay, az)) knockback(ax, az);
    (void)cause;
    if (health <= 0) die(attacker);
    return true;
}

void Mob::knockback(double fromX, double fromZ, double strength) {
    double dx = x - fromX, dz = z - fromZ;
    double d = std::hypot(dx, dz);
    if (d < 1e-6) d = 1;
    vx = dx / d * strength;
    vz = dz / d * strength;
    vy = 0.36;
}

void Mob::die(int killer) {
    dead = true;
    health = 0;
    Writer st;
    st.i32(entityId).i8(3);
    server.broadcastPacket("entity_status", st.out, nullptr, [this](Player* p) { return visibleTo(p); });
    server.playSound("minecraft:entity." + type + ".death", x, y, z, 1.0f, 1.0f, 6);
    vanilla::onMobDeath(server, *this, killer);
}
