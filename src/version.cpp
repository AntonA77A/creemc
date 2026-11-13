#include "version.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>

#include "data.hpp"
#include "nbt.hpp"
#include "text.hpp"

Bytes profileProperties(const std::vector<ProfileProperty>& props) {
    Writer w;
    w.varint(int32_t(props.size()));
    for (const auto& p : props) {
        w.string(p.name).string(p.value);
        if (!p.signature.empty()) w.boolean(true).string(p.signature);
        else w.boolean(false);
    }
    return w.out;
}

Bytes legacyVelocity(double vx, double vy, double vz) {
    auto c = [](double v) { return int16_t(std::max(-3.9, std::min(3.9, v)) * 8000); };
    Writer w;
    w.i16(c(vx)).i16(c(vy)).i16(c(vz));
    return w.out;
}

Bytes lpVec3(double vx, double vy, double vz) {
    auto clamp = [](double v) { return std::isnan(v) ? 0.0 : std::max(-1.7179869183e10, std::min(1.7179869183e10, v)); };
    vx = clamp(vx);
    vy = clamp(vy);
    vz = clamp(vz);
    double m = std::max({std::fabs(vx), std::fabs(vy), std::fabs(vz)});
    if (m < 3.051944088384301e-5) return Bytes(1, '\0');
    int64_t scale = int64_t(std::ceil(m));
    bool extended = scale > 3;
    uint64_t header = extended ? uint64_t((scale & 3) | 4) : uint64_t(scale);
    auto q = [&](double v) {
        long long r = std::llround((v / double(scale) * 0.5 + 0.5) * 32766.0);
        return uint64_t(std::max(0LL, std::min(32766LL, r)));
    };
    uint64_t packed = header | q(vx) << 3 | q(vy) << 18 | q(vz) << 33;
    Writer w;
    w.u8(uint8_t(packed & 0xFF)).u8(uint8_t((packed >> 8) & 0xFF)).u32(uint32_t((packed >> 16) & 0xFFFFFFFF));
    if (extended) w.varint(int32_t(scale >> 2));
    return w.out;
}

Bytes soundPacket(const std::string& name, double x, double y, double z, float volume, float pitch, int category) {
    Writer w;
    w.varint(0).string(name).boolean(false).varint(category).i32(int32_t(x * 8)).i32(int32_t(y * 8)).i32(int32_t(z * 8));
    w.f32(volume).f32(pitch).i64(0);
    return w.out;
}

// ---- Version ----------------------------------------------------------------

const Version* Version::get(int protocol) {
    static std::mutex m;
    static std::map<int, std::unique_ptr<Version>> cache;
    if (std::find(data::SUPPORTED.begin(), data::SUPPORTED.end(), protocol) == data::SUPPORTED.end()) return nullptr;
    std::lock_guard<std::mutex> lock(m);
    auto& v = cache[protocol];
    if (!v) v.reset(new Version(protocol));
    return v.get();
}

Version::Version(int p) : protocol(p) {
    const json::Value& d = data::version(p);
    for (const auto& n : d["names"].asArray()) names.push_back(n.asString());
    name = names.back();
    for (const auto& [k, v] : d["packets"].asObject()) {
        ids[k] = int(v.asInt());
        if (startsWith(k, "c_")) idNames[int(v.asInt())] = k;   // Play-Pakete vom Client
    }
    playerEntity = int(d["player_entity"].asInt());
    auto loadMap = [](const json::Value& o, std::unordered_map<std::string, int>& out) {
        for (const auto& [k, v] : o.asObject()) out[k] = int(v.asInt());
    };
    loadMap(d["entities"], entities);
    loadMap(d["meta_types"], metaTypes);
    loadMap(d["block_entities"], blockEntities);
    loadMap(d["components"], components);
    loadMap(d["items"], items);
    for (const auto& [k, v] : items) itemNames[v] = k;
    for (const auto& [reg, entries] : d["registries"].asObject()) {
        registryOrder_.push_back(reg);
        auto& list = registries_[reg];
        for (const auto& e : entries.asArray()) list.push_back(e.asString());
    }
    tags_ = d["tags"];

    // Blöcke: Registry-IDs und Umrechnung kanonisch -> diese Version
    std::unordered_map<std::string, data::BlockRange> own;
    int maxState = 0;
    for (const auto& [n, b] : d["blocks"].asObject()) {
        data::BlockRange r{int(b[0].asInt()), int(b[1].asInt()), int(b[2].asInt()), int(b[3].asInt())};
        own[n] = r;
        blockIds["minecraft:" + n] = r.id;
        maxState = std::max(maxState, r.hi);
    }
    stateBits = int(std::ceil(std::log2(double(maxState + 1))));
    int fallback = own.count("stone") ? own["stone"].def : 1;
    remap.assign(size_t(data::MAX_STATE) + 1, fallback);
    for (const auto& [n, c] : data::BLOCKS) {
        auto it = own.find(n);
        if (it == own.end()) continue;
        const auto& o = it->second;
        if (o.hi - o.lo == c.hi - c.lo) {
            for (int s = c.lo; s <= c.hi; ++s) remap[size_t(s)] = o.lo + (s - c.lo);   // gleiche Zustände
        } else {
            for (int s = c.lo; s <= c.hi; ++s) remap[size_t(s)] = o.def;
        }
    }
    plains = registryIndex("minecraft:worldgen/biome", "minecraft:plains");
    overworld = registryIndex("minecraft:dimension_type", "minecraft:overworld");
    overworldClock = registryIndex("minecraft:world_clock", "minecraft:overworld");
}

int Version::id(const std::string& packet) const {
    auto it = ids.find(packet);
    if (it == ids.end()) throw ProtocolError("Paket '" + packet + "' gibt es in " + name + " nicht");
    return it->second;
}

int Version::registryIndex(const std::string& registry, const std::string& entry) const {
    auto it = registries_.find(registry);
    if (it == registries_.end()) return 0;
    auto pos = std::find(it->second.begin(), it->second.end(), entry);
    return pos == it->second.end() ? 0 : int(pos - it->second.begin());
}

std::optional<int> Version::itemId(const std::string& n) const {
    if (n.empty()) return std::nullopt;
    auto it = items.find(removePrefix(n, "minecraft:"));
    if (it == items.end()) return std::nullopt;
    return it->second;
}

std::string Version::itemName(int id) const {
    auto it = itemNames.find(id);
    return it == itemNames.end() ? "" : "minecraft:" + it->second;
}

std::optional<int> Version::entityType(const std::string& n) const {
    auto it = entities.find(removePrefix(n, "minecraft:"));
    if (it == entities.end()) return std::nullopt;
    return it->second;
}

// ---- Konfiguration ----------------------------------------------------------

Bytes Version::knownPacks() const {
    // Der Client antwortet nur mit dem Eintrag, der genau zu seiner Version passt; spätere
    // Patch-Versionen (z.B. 26.3.1) haben dasselbe Protokoll -> mit anbieten.
    std::vector<std::string> cands = names;
    const std::string& base = names.back();
    auto parts = split(base, '.');
    for (int i = 1; i <= 3; ++i) {
        if (parts.size() == 3) cands.push_back(parts[0] + "." + parts[1] + "." + std::to_string(std::stoi(parts[2]) + i));
        else cands.push_back(base + "." + std::to_string(i));
    }
    Writer w;
    w.varint(int32_t(cands.size()));
    for (const auto& c : cands) w.string("minecraft").string("core").string(c);
    return w.out;
}

std::vector<Bytes> Version::registryPackets() const {
    std::vector<Bytes> out;
    for (const auto& reg : registryOrder_) {
        const auto& entries = registries_.at(reg);
        Writer w;
        w.string(reg).varint(int32_t(entries.size()));
        for (const auto& e : entries) w.string(e).u8(0);
        out.push_back(std::move(w.out));
    }
    return out;
}

const Bytes& Version::tagsPayload() const {
    if (tagsPayload_.empty()) {
        Writer w;
        w.varint(int32_t(tags_.size()));
        for (const auto& [reg, tags] : tags_.asObject()) {
            w.string(reg).varint(int32_t(tags.size()));
            for (const auto& [tag, idsList] : tags.asObject()) {
                w.string(tag).varint(int32_t(idsList.size()));
                for (const auto& i : idsList.asArray()) w.varint(int32_t(i.asInt()));
            }
        }
        tagsPayload_ = std::move(w.out);
    }
    return tagsPayload_;
}

// ---- Pakete -----------------------------------------------------------------

Bytes Version::loginSuccess(const Uuid& uuid, const std::string& n, const std::vector<ProfileProperty>& props) const {
    Writer w;
    w.uuid(uuid).string(n).raw(profileProperties(props));
    if (protocol < V1_21_2) w.boolean(false);   // strictErrorHandling
    if (protocol >= V26_2) w.uuid(uuid);         // Session-ID
    return w.out;
}

static void spawnInfo(Writer& w, const Version& v, int gamemode, bool isFlat) {
    w.varint(v.overworld).string("minecraft:overworld").i64(0);
    if (v.protocol >= V26_3) w.varint(gamemode).varint(0);
    else w.i8(int8_t(gamemode)).i8(-1);
    w.boolean(false).boolean(isFlat).boolean(false).varint(0);
    if (v.protocol >= V1_21_2) w.varint(63);   // Meeresspiegel
}

Bytes Version::login(int eid, int gamemode, int maxPlayers, int viewDistance, bool isFlat, bool onlineMode) const {
    Writer w;
    w.i32(eid).boolean(false).varint(1).string("minecraft:overworld");
    w.varint(maxPlayers).varint(viewDistance).varint(viewDistance);
    w.boolean(false).boolean(true).boolean(false);
    spawnInfo(w, *this, gamemode, isFlat);
    if (protocol >= V26_2) w.boolean(onlineMode);
    w.boolean(false);   // sicherer Chat
    return w.out;
}

Bytes Version::respawn(int gamemode, bool isFlat) const {
    Writer w;
    spawnInfo(w, *this, gamemode, isFlat);
    w.i8(0);
    return w.out;
}

Bytes Version::syncPlayerPosition(int tid, double x, double y, double z, float yaw, float pitch) const {
    Writer w;
    if (protocol < V1_21_2) {
        w.f64(x).f64(y).f64(z).f32(yaw).f32(pitch).u8(0).varint(tid);
    } else {
        w.varint(tid).f64(x).f64(y).f64(z).f64(0).f64(0).f64(0).f32(yaw).f32(pitch).u32(0);
    }
    return w.out;
}

std::pair<std::string, Bytes> Version::entityPosition(int eid, double x, double y, double z, float yaw, float pitch,
                                                      bool onGround) const {
    Writer w;
    if (protocol < V1_21_2) {
        w.varint(eid).f64(x).f64(y).f64(z).angle(yaw).angle(pitch).boolean(onGround);
        return {"entity_teleport", w.out};
    }
    if (protocol >= V26_3) {   // 26.3: "PositionPath", 0 = linear, nur Endposition
        w.varint(eid).varint(0).f64(x).f64(y).f64(z).f32(yaw).f32(pitch).boolean(onGround);
        return {"sync_entity_position", w.out};
    }
    w.varint(eid).f64(x).f64(y).f64(z).f64(0).f64(0).f64(0).f32(yaw).f32(pitch).boolean(onGround);
    return {"sync_entity_position", w.out};
}

Bytes Version::spawnEntity(int eid, const Uuid& uuid, int type, double x, double y, double z, float yaw, float pitch,
                           double vx, double vy, double vz, int extra) const {
    Writer w;
    w.varint(eid).uuid(uuid).varint(type).f64(x).f64(y).f64(z);
    if (protocol >= V1_21_9) {
        w.raw(lpVec3(vx, vy, vz)).angle(pitch).angle(yaw).angle(yaw).varint(extra);
    } else {
        w.angle(pitch).angle(yaw).angle(yaw).varint(extra).raw(legacyVelocity(vx, vy, vz));
    }
    return w.out;
}

Bytes Version::spawnPlayer(int eid, const Uuid& uuid, double x, double y, double z, float yaw, float pitch) const {
    return spawnEntity(eid, uuid, playerEntity, x, y, z, yaw, pitch);
}

Bytes Version::velocity(int eid, double vx, double vy, double vz) const {
    Writer w;
    w.varint(eid).raw(protocol >= V1_21_9 ? lpVec3(vx, vy, vz) : legacyVelocity(vx, vy, vz));
    return w.out;
}

static int meta(const Version& v, const char* type) {
    auto it = v.metaTypes.find(type);
    return it == v.metaTypes.end() ? 0 : it->second;
}

Bytes Version::itemMetadata(int eid, const std::string& item, int count) const {
    Writer w;
    w.varint(eid).u8(8).varint(meta(*this, "item_stack")).raw(slot(item, count)).u8(0xFF);
    return w.out;
}

Bytes Version::flagsMetadata(int eid, int flags) const {
    Writer w;
    w.varint(eid).u8(0).varint(meta(*this, "byte")).i8(int8_t(flags)).u8(0xFF);
    return w.out;
}

Bytes Version::airMetadata(int eid, int air) const {
    Writer w;
    w.varint(eid).u8(1).varint(meta(*this, "int")).varint(air).u8(0xFF);
    return w.out;
}

Bytes Version::playerMetadata(int eid, bool sneaking, int skinParts, bool burning) const {
    bool fresh = protocol >= V1_21_9;
    Writer w;
    w.varint(eid);
    w.u8(0).varint(0).i8(int8_t((sneaking ? 0x02 : 0) | (burning ? 0x01 : 0)));
    w.u8(6).varint(fresh ? 20 : 21).varint(sneaking ? 5 : 0);
    w.u8(fresh ? 16 : 17).varint(0).u8(uint8_t(skinParts));
    w.u8(0xFF);
    return w.out;
}

Bytes Version::windowItems(int windowId, int stateId, const std::vector<std::pair<std::string, int>>& slots,
                           const std::pair<std::string, int>& cursor) const {
    Writer w;
    w.varint(windowId).varint(stateId).varint(int32_t(slots.size()));
    for (const auto& s : slots) w.raw(slot(s.first, s.second));
    w.raw(slot(cursor.first, cursor.second));
    return w.out;
}

Bytes Version::slot(const std::string& item, int count) const {
    auto id = itemId(data::baseItem(item));
    Writer w;
    if (!id || count <= 0) {
        w.varint(0);
        return w.out;
    }
    json::Value metaV;
    size_t at = item.find('@');
    if (at != std::string::npos) {
        try {
            metaV = json::parse(item.substr(at + 1));
        } catch (...) {
        }
    }
    auto comps = buildComponents(metaV);
    w.varint(count).varint(*id).varint(int32_t(comps.size())).varint(0);
    for (const auto& c : comps) w.raw(c);
    return w.out;
}

std::vector<Bytes> Version::buildComponents(const json::Value& m) const {
    std::vector<Bytes> out;
    if (!m.isObject()) return out;
    auto comp = [&](const char* n) {
        auto it = components.find(n);
        return it == components.end() ? -1 : it->second;
    };
    if (m["name"].isString() && comp("custom_name") >= 0) {
        json::Object c;
        c["text"] = m["name"].asString();
        c["italic"] = true;
        Writer w;
        w.varint(comp("custom_name")).raw(text::toNbt(json::Value(std::move(c))));
        out.push_back(w.out);
    }
    for (auto [key, cname] : {std::pair<const char*, const char*>{"ench", "enchantments"}, {"stored", "stored_enchantments"}}) {
        const json::Value& e = m[key];
        if (!e.isObject() || !e.size() || comp(cname) < 0) continue;
        std::vector<std::pair<int, int>> entries;
        for (const auto& [en, lvl] : e.asObject()) {
            int idx = enchantmentId(en);
            if (idx >= 0) entries.emplace_back(idx, int(lvl.asInt()));
        }
        Writer w;
        w.varint(comp(cname)).varint(int32_t(entries.size()));
        for (auto [i, l] : entries) w.varint(i).varint(l);
        if (protocol < V1_21_5) w.boolean(true);   // bis 1.21.4: im Tooltip zeigen
        out.push_back(w.out);
    }
    if (m["repair"].asInt() && comp("repair_cost") >= 0) {
        Writer w;
        w.varint(comp("repair_cost")).varint(int32_t(m["repair"].asInt()));
        out.push_back(w.out);
    }
    return out;
}

int Version::enchantmentId(const std::string& n) const {
    auto it = registries_.find("minecraft:enchantment");
    if (it == registries_.end()) return -1;
    auto pos = std::find(it->second.begin(), it->second.end(), n);
    return pos == it->second.end() ? -1 : int(pos - it->second.begin());
}

std::pair<std::string, Bytes> Version::swing(int eid, bool offHand) const {
    Writer w;
    if (protocol >= V26_3) {
        w.varint(eid).varint(offHand ? 1 : 0).varint(1).varint(6);
        return {"swing_animation", w.out};
    }
    w.varint(eid).u8(offHand ? 3 : 0);
    return {"animation", w.out};
}

Bytes Version::spawnPosition(int x, int y, int z) const {
    Writer w;
    if (protocol >= V1_21_9) w.string("minecraft:overworld").position(x, y, z).f32(0).f32(0);
    else w.position(x, y, z).f32(0);
    return w.out;
}

Bytes Version::updateTime(int64_t age, int64_t time, bool ticking) const {
    Writer w;
    if (protocol >= V26_1) {
        w.i64(age).varint(1).varint(overworldClock).varlong(time).f32(0).f32(ticking ? 1.0f : 0.0f);
        return w.out;
    }
    int64_t day = time % 24000;
    if (protocol < V1_21_2) w.i64(age).i64(ticking ? day : -(day ? day : 1));
    else w.i64(age).i64(day).boolean(ticking);
    return w.out;
}

Version::Interact Version::readInteract(Reader& r) const {
    Interact i;
    i.target = r.varint();
    if (protocol >= V26_1) {
        i.hand = r.varint();
        return i;
    }
    int kind = r.varint();
    if (kind == 2) {
        r.f32();
        r.f32();
        r.f32();
    }
    i.hand = (kind == 0 || kind == 2) ? r.varint() : 0;
    i.attack = kind == 1;
    r.boolean();
    return i;
}
