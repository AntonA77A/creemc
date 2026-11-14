#include "objects.hpp"

#include <algorithm>
#include <cmath>
#include <map>

#include "buffer.hpp"
#include "data.hpp"
#include "placement.hpp"
#include "player.hpp"
#include "server.hpp"
#include "vanilla.hpp"
#include "version.hpp"

namespace objects {
namespace {

const double PI = 3.14159265358979;
int fl(double v) { return int(std::floor(v)); }

const char* WOODS[] = {"oak", "spruce", "birch", "jungle", "acacia", "cherry", "dark_oak", "mangrove", "bamboo", "pale_oak"};

bool isBoatItem(const std::string& n) {
    return endsWith(n, "_boat") || endsWith(n, "_raft");
}
bool isMinecartItem(const std::string& n) {
    return n == "minecraft:minecart" || n == "minecraft:chest_minecart" || n == "minecraft:furnace_minecart" ||
           n == "minecraft:tnt_minecart" || n == "minecraft:hopper_minecart";
}
bool isRail(int state) {
    std::string n = data::blockName(state);
    return n == "minecraft:rail" || n == "minecraft:powered_rail" || n == "minecraft:detector_rail" || n == "minecraft:activator_rail";
}

// Richtungs-Nummer wie im Spawn-Paket: unten 0, oben 1, Norden 2, Süden 3, Westen 4, Osten 5
int faceToDirection(int face) { return face; }   // Vanilla-Seiten haben dieselbe Reihenfolge

Bytes intMeta(const Version& v, int eid, int index, int value) {
    Writer w;
    auto it = v.metaTypes.find("int");
    w.varint(eid).u8(uint8_t(index)).varint(it == v.metaTypes.end() ? 1 : it->second).varint(value).u8(0xFF);
    return w.out;
}

}  // namespace

// ---- Basis ------------------------------------------------------------------

ObjectEntity::ObjectEntity(Server& s, const std::string& t, double px, double py, double pz, std::string drop)
    : Entity(s, t, px, py, pz), dropItem(std::move(drop)) {
    if (const json::Value& i = data::game()["entity_info"]["minecraft:" + type]; i.isObject()) {
        width = i["w"].asDouble(width);
        height = i["h"].asDouble(height);
    }
}

void ObjectEntity::breakIt(Player* p, bool drop) {
    if (removed) return;
    if (drop && !dropItem.empty()) server.dropItem(x, y + 0.3, z, dropItem, 1);
    if (p) server.playSound("minecraft:entity." + std::string(type == "armor_stand" ? "armor_stand.break" : "item_frame.break"), x, y, z, 1, 1, 6);
    remove();
}

void ObjectEntity::attacked(Player& p) { breakIt(&p, p.gamemode != 1); }

json::Object ObjectEntity::save() const {
    json::Object o;
    o["type"] = type;
    o["item"] = dropItem;
    o["pos"] = json::Array{x, y, z, double(yaw)};
    return o;
}

namespace {

// ---- Rüstungsständer --------------------------------------------------------

class ArmorStand : public ObjectEntity {
public:
    ArmorStand(Server& s, double px, double py, double pz) : ObjectEntity(s, "armor_stand", px, py, pz, "minecraft:armor_stand") {}
    Stack equip[6];   // Haupthand, Nebenhand, Füße, Beine, Brust, Kopf

    Bytes equipment(const Version& v) const {
        Writer w;
        w.varint(entityId);
        for (int i = 0; i < 6; ++i) {
            w.u8(uint8_t(i | (i < 5 ? 0x80 : 0)));
            w.raw(equip[i].first.empty() ? Bytes(1, '\0') : v.slot(equip[i].first, equip[i].second));
        }
        return w.out;
    }
    std::vector<std::pair<std::string, Bytes>> spawnPackets(const Version& v) const override {
        auto out = Entity::spawnPackets(v);
        if (!out.empty()) out.emplace_back("entity_equipment", equipment(v));
        return out;
    }
    void interact(Player& p, int hand) override {
        if (hand != 0 || p.gamemode == 3) return;
        std::string held = p.heldItem();
        int slot = 0;
        if (!held.empty()) {
            auto a = armorSlotFor(held);   // 5 Kopf, 6 Brust, 7 Beine, 8 Füße
            slot = a ? (*a == 5 ? 5 : *a == 6 ? 4 : *a == 7 ? 3 : 2) : 0;
        } else {   // leere Hand: oberstes belegtes Teil nehmen
            for (int s : {5, 4, 3, 2, 0, 1})
                if (!equip[s].first.empty()) {
                    slot = s;
                    break;
                }
        }
        Stack old = equip[slot];
        auto it = p.inventory.find(36 + p.heldSlot);
        Stack newOne = it == p.inventory.end() ? Stack{} : Stack{it->second.first, 1};
        if (old.first.empty() && newOne.first.empty()) return;
        equip[slot] = newOne;
        if (!newOne.first.empty() && p.gamemode != 1) p.setSlot(36 + p.heldSlot, it->second.first, it->second.second - 1);
        if (!old.first.empty() && p.addItem(old.first, old.second) > 0) p.dropStack(old.first, old.second);
        server.broadcastPacketV("entity_equipment", [this](const Version& v) { return equipment(v); });
        server.playSound("minecraft:item.armor.equip_generic", x, y, z, 1, 1, 6);
    }
    void attacked(Player& p) override {
        int now = int(server.tick());
        if (p.gamemode != 1 && now - lastHitTick > 5) {   // Überleben: zweimal schnell schlagen
            lastHitTick = now;
            server.playSound("minecraft:entity.armor_stand.hit", x, y, z, 1, 1, 6);
            return;
        }
        for (auto& e : equip)
            if (!e.first.empty()) server.dropItem(x, y + 0.5, z, e.first, e.second);
        breakIt(&p, p.gamemode != 1);
    }
    json::Object save() const override {
        json::Object o = ObjectEntity::save();
        json::Array eq;
        for (const auto& e : equip) eq.push_back(e.first.empty() ? json::Value() : json::Value(json::Array{e.first, e.second}));
        o["equip"] = json::Value(std::move(eq));
        return o;
    }
    void load(const json::Value& v) override {
        for (int i = 0; i < 6 && v["equip"].isArray() && size_t(i) < v["equip"].size(); ++i) {
            const json::Value& e = v["equip"][size_t(i)];
            if (e.isArray() && e.size() >= 2) equip[i] = {e[0].str(), int(e[1].asInt(1))};
        }
    }
};

// ---- Rahmen und Gemälde -----------------------------------------------------

class Hanging : public ObjectEntity {
public:
    Hanging(Server& s, const std::string& t, double px, double py, double pz, int dir, std::string drop)
        : ObjectEntity(s, t, px, py, pz, std::move(drop)), direction(dir) {
        gravity = 0;
    }
    int direction;
    Stack item;
    int rotation = 0;
    bool isFrame() const { return type == "item_frame" || type == "glow_item_frame"; }

    std::vector<std::pair<std::string, Bytes>> spawnPackets(const Version& v) const override {
        std::vector<std::pair<std::string, Bytes>> out;
        auto t = v.entityType(type);
        if (!t) return out;
        out.emplace_back("spawn_entity", v.spawnEntity(entityId, uuid, *t, x, y, z, 0, 0, 0, 0, 0, faceToDirection(direction)));
        if (isFrame() && !item.first.empty()) {
            out.emplace_back("entity_metadata", v.itemMetadata(entityId, item.first, 1));
            out.emplace_back("entity_metadata", intMeta(v, entityId, 9, rotation));
        }
        return out;
    }
    void tick() override {}   // hängt fest
    void interact(Player& p, int hand) override {
        if (!isFrame() || hand != 0 || p.gamemode == 3) return;
        if (item.first.empty()) {
            auto it = p.inventory.find(36 + p.heldSlot);
            if (it == p.inventory.end()) return;
            item = {it->second.first, 1};
            if (p.gamemode != 1) p.setSlot(36 + p.heldSlot, it->second.first, it->second.second - 1);
            server.playSound("minecraft:entity.item_frame.add_item", x, y, z, 1, 1, 6);
        } else {
            rotation = (rotation + 1) % 8;
            server.playSound("minecraft:entity.item_frame.rotate_item", x, y, z, 1, 1, 6);
        }
        server.broadcastPacketV("entity_metadata", [this](const Version& v) { return v.itemMetadata(entityId, item.first, 1); });
        server.broadcastPacketV("entity_metadata", [this](const Version& v) { return intMeta(v, entityId, 9, rotation); });
    }
    void attacked(Player& p) override {
        if (isFrame() && !item.first.empty()) {   // erst das Item herausschlagen
            if (p.gamemode != 1) server.dropItem(x, y, z, item.first, 1);
            item = {};
            rotation = 0;
            server.broadcastPacketV("entity_metadata", [this](const Version& v) { return v.itemMetadata(entityId, "", 0); });
            server.playSound("minecraft:entity.item_frame.remove_item", x, y, z, 1, 1, 6);
            return;
        }
        if (!isFrame()) server.playSound("minecraft:entity.painting.break", x, y, z, 1, 1, 6);
        breakIt(&p, p.gamemode != 1);
    }
    json::Object save() const override {
        json::Object o = ObjectEntity::save();
        o["dir"] = direction;
        if (!item.first.empty()) o["frame_item"] = item.first;
        o["rotation"] = rotation;
        return o;
    }
    void load(const json::Value& v) override {
        if (v["frame_item"].isString()) item = {v["frame_item"].asString(), 1};
        rotation = int(v["rotation"].asInt(0));
    }
};

// ---- Fahrzeuge --------------------------------------------------------------

class Vehicle : public ObjectEntity {
public:
    using ObjectEntity::ObjectEntity;
    int passenger = 0;     // Entity-ID des Spielers
    float damage = 0;
    bool clientDriven = false;

    void sendPassengers() {
        Writer w;
        w.varint(entityId);
        if (passenger) w.varint(1).varint(passenger);
        else w.varint(0);
        server.broadcastPacket("set_passengers", w.out);
    }
    void interact(Player& p, int hand) override {
        if (hand != 0 || passenger || p.vehicle || p.sneaking) return;
        mount(p, this);
    }
    void attacked(Player& p) override {
        damage += p.gamemode == 1 ? 100 : 10;
        Writer hurt;
        hurt.varint(entityId).f32(yaw);
        server.broadcastPacket("hurt_animation", hurt.out);
        if (damage > 40) {
            if (Player* rider = server.playerById(passenger)) dismount(*rider);
            breakIt(nullptr, p.gamemode != 1);
        }
    }
    void syncExcept(Player* driver) {   // der Fahrer steuert selbst: ihm nichts zurückschicken
        double dx = std::abs(x - sentX_) + std::abs(y - sentY_) + std::abs(z - sentZ_);
        if (dx < 0.001 && std::abs(yaw - sentYaw_) < 1) return;
        sentX_ = x;
        sentY_ = y;
        sentZ_ = z;
        sentYaw_ = yaw;
        server.broadcastVersioned([this](const Version& v) { return v.entityPosition(entityId, x, y, z, yaw, pitch, onGround); }, driver,
                                  [this](Player* q) { return visibleTo(q); });
    }
    void riderFollows() {
        Player* rider = server.playerById(passenger);
        if (!rider) {
            passenger = 0;
            return;
        }
        rider->x = x;
        rider->y = y + 0.3;
        rider->z = z;
        rider->fallDistance = 0;
    }
};

class Boat : public Vehicle {
public:
    Boat(Server& s, const std::string& t, double px, double py, double pz, std::string drop, int wood)
        : Vehicle(s, t, px, py, pz, std::move(drop)), woodIndex(wood) {}
    int woodIndex;
    std::vector<std::pair<std::string, Bytes>> spawnPackets(const Version& v) const override {
        std::vector<std::pair<std::string, Bytes>> out;
        bool chest = dropItem.find("chest") != std::string::npos;
        std::string t = type;
        if (!v.entityType(t)) t = chest ? "chest_boat" : "boat";   // bis 1.21.1 ein Typ mit Holz-Variante
        auto id = v.entityType(t);
        if (!id) return out;
        out.emplace_back("spawn_entity", v.spawnEntity(entityId, uuid, *id, x, y, z, 0, yaw, 0, 0, 0));
        if (t == "boat" || t == "chest_boat") out.emplace_back("entity_metadata", intMeta(v, entityId, 11, woodIndex));
        if (passenger) {
            Writer w;
            w.varint(entityId).varint(1).varint(passenger);
            out.emplace_back("set_passengers", w.out);
        }
        return out;
    }
    void tick() override {
        ++age;
        if (passenger) {
            riderFollows();
            if (passenger) {
                syncExcept(server.playerById(passenger));
                return;
            }
        }
        // ohne Fahrer: auf dem Wasser schwimmen, an Land fallen
        static int water = data::blockState("water");
        bool wet = server.world().getBlock(fl(x), fl(y), fl(z)) == water;
        bool below = server.world().getBlock(fl(x), fl(y - 0.1), fl(z)) == water;
        if (wet) vy = std::min(vy + 0.04, 0.1);
        else if (below) vy = 0;
        else vy -= 0.04;
        move(vx, vy, vz);
        vx *= 0.9;
        vz *= 0.9;
        vy *= 0.9;
        sync();
        if (damage > 0) damage = std::max(0.0f, damage - 1);
    }
};

class Minecart : public Vehicle {
public:
    using Vehicle::Vehicle;
    double speed = 0;     // entlang der Schiene
    int dirX = 0, dirZ = 1;
    std::vector<std::pair<std::string, Bytes>> spawnPackets(const Version& v) const override {
        auto out = Entity::spawnPackets(v);
        if (!out.empty() && passenger) {
            Writer w;
            w.varint(entityId).varint(1).varint(passenger);
            out.emplace_back("set_passengers", w.out);
        }
        return out;
    }
    void push(double px, double pz) {
        if (dirX != 0 || dirZ != 0) {
            double d = px * dirX + pz * dirZ;
            speed += d;
        }
    }
    void tick() override {
        ++age;
        World& w = server.world();
        int bx = fl(x), by = fl(y), bz = fl(z);
        int st = w.getBlock(bx, by, bz);
        if (!isRail(st)) {
            int below = w.getBlock(bx, by - 1, bz);
            if (isRail(below)) {
                --by;
                st = below;
            }
        }
        if (isRail(st)) {
            std::string shape = data::prop(st, "shape");
            // Ausgänge der Schiene
            int ax = 0, az = 0, bx2 = 0, bz2 = 0;
            double slope = 0;
            if (shape == "north_south" || shape == "ascending_north" || shape == "ascending_south") { az = -1; bz2 = 1; }
            else if (shape == "east_west" || shape == "ascending_east" || shape == "ascending_west") { ax = 1; bx2 = -1; }
            else if (shape == "north_east") { az = -1; bx2 = 1; }
            else if (shape == "north_west") { az = -1; bx2 = -1; }
            else if (shape == "south_east") { az = 1; bx2 = 1; }
            else if (shape == "south_west") { az = 1; bx2 = -1; }
            if (shape == "ascending_east") slope = 1;
            if (shape == "ascending_west") slope = -1;
            // Fahrtrichtung: der Ausgang, der am besten zur bisherigen Richtung passt (Kurven drehen mit)
            double s = speed;
            int fx = dirX, fz = dirZ;
            if (s < 0) {
                fx = -fx;
                fz = -fz;
                s = -s;
            }
            int dotA = fx * ax + fz * az, dotB = fx * bx2 + fz * bz2;
            int ex = dotA >= dotB ? ax : bx2, ez = dotA >= dotB ? az : bz2;
            dirX = ex;
            dirZ = ez;
            speed = s;
            // angetriebene Schiene
            if (data::blockName(st) == "minecraft:powered_rail") {
                if (data::prop(st, "powered") == "true") speed = std::min(0.4, speed + 0.06);
                else speed *= 0.5;
            }
            // Gefälle
            if (shape.rfind("ascending_", 0) == 0) {
                int upX = shape == "ascending_east" ? 1 : shape == "ascending_west" ? -1 : 0;
                int upZ = shape == "ascending_south" ? 1 : shape == "ascending_north" ? -1 : 0;
                speed += (dirX * upX + dirZ * upZ) > 0 ? -0.0078125 : 0.0078125;
            }
            (void)slope;
            speed *= passenger ? 0.997 : 0.96;
            speed = std::min(speed, 0.4);
            if (speed < 0.003) speed = 0;
            // auf der Schienenmitte halten
            if (dirX != 0) z = bz + 0.5;
            if (dirZ != 0) x = bx + 0.5;
            x += dirX * speed;
            z += dirZ * speed;
            y = by + 0.0625;
            if (shape.rfind("ascending_", 0) == 0) {   // Steigung: Höhe innerhalb des Blocks
                double fx2 = x - bx, fz2 = z - bz;
                double frac = shape == "ascending_east" ? fx2 : shape == "ascending_west" ? 1 - fx2 : shape == "ascending_south" ? fz2 : 1 - fz2;
                y = by + 0.0625 + std::max(0.0, std::min(1.0, frac));
            }
            yaw = float(std::atan2(-double(dirX), double(dirZ)) * 180 / PI);
            vx = vz = vy = 0;
            onGround = true;
        } else {
            Entity::tick();
            age--;
        }
        if (passenger) riderFollows();
        sync();
        if (damage > 0) damage = std::max(0.0f, damage - 1);
    }
};

std::vector<ObjectEntity*> all(Server& s) {
    std::vector<ObjectEntity*> out;
    for (Entity* e : s.entities())
        if (auto* o = dynamic_cast<ObjectEntity*>(e)) out.push_back(o);
    return out;
}

ObjectEntity* create(Server& s, const std::string& type, const std::string& item, double x, double y, double z, float yaw, int dir) {
    std::unique_ptr<ObjectEntity> e;
    if (type == "armor_stand") e = std::make_unique<ArmorStand>(s, x, y, z);
    else if (type == "item_frame" || type == "glow_item_frame" || type == "painting") e = std::make_unique<Hanging>(s, type, x, y, z, dir, item);
    else if (endsWith(type, "_boat") || endsWith(type, "_raft") || type == "boat" || type == "chest_boat") {
        int wood = 0;
        std::string w = removePrefix(item, "minecraft:");
        for (int i = 0; i < 10; ++i)
            if (startsWith(w, WOODS[i])) wood = i;
        e = std::make_unique<Boat>(s, type, x, y, z, item, wood);
    } else if (endsWith(type, "minecart")) {
        e = std::make_unique<Minecart>(s, type, x, y, z, item);
    }
    if (!e) return nullptr;
    e->yaw = yaw;
    return static_cast<ObjectEntity*>(s.spawnEntity(std::move(e)));
}

void takeOne(Player& p) {
    if (p.gamemode == 1) return;
    auto it = p.inventory.find(36 + p.heldSlot);
    if (it != p.inventory.end()) p.setSlot(36 + p.heldSlot, it->second.first, it->second.second - 1);
}

}  // namespace

bool isObject(Entity* e) { return dynamic_cast<ObjectEntity*>(e) != nullptr; }

bool summon(Server& s, const std::string& rawType, double x, double y, double z, float yaw) {
    std::string type = removePrefix(rawType, "minecraft:");
    std::string item = "minecraft:" + type;
    if (type == "boat") item = "minecraft:oak_boat", type = "oak_boat";
    if (type == "chest_boat") item = "minecraft:oak_chest_boat", type = "oak_chest_boat";
    if (type != "armor_stand" && !isBoatItem(item) && !isMinecartItem(item)) return false;
    return create(s, type, item, x, y, z, yaw, 0) != nullptr;
}

bool useItemOnBlock(Player& p, const std::string& rawItem, int cx, int cy, int cz, int face) {
    Server& s = p.server;
    std::string item = data::baseItem(rawItem);
    int tx = cx + placement::FACE_OFFSETS[face][0], ty = cy + placement::FACE_OFFSETS[face][1], tz = cz + placement::FACE_OFFSETS[face][2];
    if (item == "minecraft:armor_stand") {
        if (s.world().getBlock(tx, ty, tz) != 0 || s.world().getBlock(tx, ty + 1, tz) != 0) return true;
        float yaw = float(std::round((p.yaw + 180) / 45.0) * 45.0);   // zum Spieler gedreht, in 45°-Schritten
        create(s, "armor_stand", item, tx + 0.5, ty, tz + 0.5, yaw, 0);
        s.playSound("minecraft:entity.armor_stand.place", tx + 0.5, ty, tz + 0.5, 1, 1, 6);
        takeOne(p);
        return true;
    }
    if (item == "minecraft:item_frame" || item == "minecraft:glow_item_frame" || item == "minecraft:painting") {
        if (item == "minecraft:painting" && (face == 0 || face == 1)) return true;   // Gemälde nur an Wände
        if (s.world().getBlock(tx, ty, tz) != 0) return true;
        double ox = placement::FACE_OFFSETS[face][0] * -0.46875, oy = placement::FACE_OFFSETS[face][1] * -0.46875,
               oz = placement::FACE_OFFSETS[face][2] * -0.46875;
        create(s, removePrefix(item, "minecraft:"), item, tx + 0.5 + ox, ty + (face == 0 || face == 1 ? 0.5 + oy : 0.0), tz + 0.5 + oz, 0, face);
        s.playSound(item == "minecraft:painting" ? "minecraft:entity.painting.place" : "minecraft:entity.item_frame.place", tx + .5, ty + .5,
                    tz + .5, 1, 1, 6);
        takeOne(p);
        return true;
    }
    if (isMinecartItem(item)) {
        if (!isRail(s.world().getBlock(cx, cy, cz))) return false;
        create(s, removePrefix(item, "minecraft:"), item, cx + 0.5, cy + 0.0625, cz + 0.5, 0, 0);
        takeOne(p);
        return true;
    }
    if (isBoatItem(item)) {
        if (face != 1) return true;   // Boote auf die Oberseite setzen
        create(s, removePrefix(item, "minecraft:"), item, tx + 0.5, ty, tz + 0.5, p.yaw, 0);
        takeOne(p);
        return true;
    }
    return false;
}

bool useItem(Player& p, const std::string& rawItem) {
    std::string item = data::baseItem(rawItem);
    if (!isBoatItem(item)) return false;
    // Blickstrahl bis zum Wasser
    Server& s = p.server;
    static int water = data::blockState("water");
    double ry = p.yaw * PI / 180, rp = p.pitch * PI / 180;
    double dx = -std::sin(ry) * std::cos(rp), dy = -std::sin(rp), dz = std::cos(ry) * std::cos(rp);
    for (double t = 0; t <= 5; t += 0.1) {
        double px = p.x + dx * t, py = p.y + 1.62 + dy * t, pz = p.z + dz * t;
        int st = s.world().getBlock(fl(px), fl(py), fl(pz));
        if (st == water) {
            create(s, removePrefix(item, "minecraft:"), item, px, fl(py) + 0.9, pz, p.yaw, 0);
            takeOne(p);
            return true;
        }
        if (data::SOLID[size_t(st)]) return false;
    }
    return false;
}

void interact(Player& p, Entity* e, int hand) {
    if (auto* o = dynamic_cast<ObjectEntity*>(e)) o->interact(p, hand);
}

void attack(Player& p, Entity* e) {
    if (auto* o = dynamic_cast<ObjectEntity*>(e)) o->attacked(p);
}

void mount(Player& p, Entity* v) {
    auto* veh = dynamic_cast<Vehicle*>(v);
    if (!veh || veh->passenger) return;
    if (p.vehicle) dismount(p);
    veh->passenger = p.entityId;
    p.vehicle = veh->entityId;
    veh->sendPassengers();
}

void dismount(Player& p) {
    if (!p.vehicle) return;
    auto* veh = dynamic_cast<Vehicle*>(p.server.getEntity(p.vehicle));
    p.vehicle = 0;
    if (!veh) return;
    veh->passenger = 0;
    veh->sendPassengers();
    p.teleport(veh->x, veh->y + 1.0, veh->z);
}

void vehicleMove(Player& p, double x, double y, double z, float yaw, float pitch) {
    auto* boat = dynamic_cast<Boat*>(p.server.getEntity(p.vehicle));
    if (!boat || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return;
    boat->x = x;
    boat->y = y;
    boat->z = z;
    boat->yaw = yaw;
    boat->pitch = pitch;
    p.x = x;
    p.y = y + 0.3;
    p.z = z;
}

void input(Player& p, bool forward, bool backward, bool left, bool right) {
    (void)left;
    (void)right;
    auto* cart = dynamic_cast<Minecart*>(p.server.getEntity(p.vehicle));
    if (!cart || (!forward && !backward)) return;
    double ry = p.yaw * PI / 180;
    double f = forward ? 0.02 : -0.02;
    double px = -std::sin(ry) * f, pz = std::cos(ry) * f;
    if (cart->speed == 0 && cart->dirX == 0 && cart->dirZ == 0) cart->dirZ = 1;
    if (cart->speed == 0) {   // aus dem Stand: Richtung nach Blick
        if (std::abs(px) > std::abs(pz)) {
            cart->dirX = px > 0 ? 1 : -1;
            cart->dirZ = 0;
        } else {
            cart->dirX = 0;
            cart->dirZ = pz > 0 ? 1 : -1;
        }
    }
    cart->push(px, pz);
}

void saveAll(Server& s) {
    json::Array a;
    for (ObjectEntity* o : all(s)) a.push_back(json::Value(o->save()));
    writeFileAtomic(joinPath(s.world().folder, "entities.json"), json::write(json::Value(std::move(a))));
}

void loadAll(Server& s) {
    std::string txt;
    if (!readFile(joinPath(s.world().folder, "entities.json"), txt)) return;
    try {
        json::Value list = json::parse(txt);
        int n = 0;
        for (const auto& v : list.asArray()) {
            const json::Value& pos = v["pos"];
            if (!pos.isArray() || pos.size() < 3) continue;
            ObjectEntity* o = create(s, v["type"].str(), v["item"].str(), pos[0].asDouble(), pos[1].asDouble(), pos[2].asDouble(),
                                     float(pos.size() > 3 ? pos[3].asDouble() : 0), int(v["dir"].asInt(2)));
            if (o) {
                o->load(v);
                ++n;
            }
        }
        if (n) logx::info(std::to_string(n) + " Objekte geladen (Rüstungsständer, Rahmen, Boote ...)");
    } catch (const std::exception& e) {
        logx::warn(std::string("entities.json fehlerhaft: ") + e.what());
    }
}

}  // namespace objects
