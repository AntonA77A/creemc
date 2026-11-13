#include "commands.hpp"

#include <algorithm>
#include <cmath>
#include <map>

#include "data.hpp"
#include "player.hpp"
#include "server.hpp"
#include "text.hpp"
#include "util.hpp"
#include "version.hpp"

namespace {
std::string S(const char* code) { return std::string(text::SECTION) + code; }

const std::map<std::string, int> GAMEMODES = {
    {"0", 0}, {"s", 0}, {"survival", 0}, {"überleben", 0}, {"1", 1}, {"c", 1}, {"creative", 1}, {"kreativ", 1},
    {"2", 2}, {"a", 2}, {"adventure", 2}, {"abenteuer", 2}, {"3", 3}, {"sp", 3}, {"spectator", 3}, {"zuschauer", 3}};
const char* GAMEMODE_NAMES[4] = {"Überleben", "Kreativ", "Abenteuer", "Zuschauer"};

Player& needPlayer(CommandSender& s) {
    Player* p = s.asPlayer();
    if (!p) throw CommandError("Nur Spieler können das benutzen.");
    return *p;
}

Player& findPlayer(Server& server, const std::string& name) {
    Player* p = server.getPlayer(name);
    if (!p) throw CommandError("Spieler '" + name + "' nicht gefunden.");
    return *p;
}

double coord(const std::string& v, double current) {
    try {
        if (startsWith(v, "~")) return current + (v.size() > 1 ? std::stod(v.substr(1)) : 0.0);
        return std::stod(v);
    } catch (const std::exception&) {
        throw CommandError("Ungültige Koordinate: " + v);
    }
}

std::string join(const std::vector<std::string>& a, size_t from, const char* sep = " ") {
    std::string out;
    for (size_t i = from; i < a.size(); ++i) {
        if (i > from) out += sep;
        out += a[i];
    }
    return out;
}
}  // namespace

void ConsoleSender::sendMessage(const std::string& msg) {
    for (const auto& line : split(text::stripColors(msg), '\n')) logx::info(line);
}

Commands::Commands(Server& server) : server_(server) { registerBuiltins(); }

void Commands::add(Command c) {
    c.name = toLower(c.name);
    if (c.usage.empty()) c.usage = "/" + c.name;
    lookup_[c.name] = c.name;
    for (auto& a : c.aliases) {
        a = toLower(a);
        lookup_.emplace(a, c.name);
    }
    commands_[c.name] = std::move(c);
}

const Command* Commands::get(const std::string& name) const {
    auto it = lookup_.find(toLower(removePrefix(name, "minecraft:")));
    if (it == lookup_.end()) return nullptr;
    auto c = commands_.find(it->second);
    return c == commands_.end() ? nullptr : &c->second;
}

std::vector<std::string> Commands::visibleNames(CommandSender& sender) const {
    std::vector<std::string> out;
    for (const auto& [n, target] : lookup_) {
        const Command& c = commands_.at(target);
        if (!c.opOnly || sender.senderIsOp()) out.push_back(n);
    }
    return out;
}

bool Commands::dispatch(CommandSender& sender, const std::string& line) {
    auto parts = split(trim(line), ' ', true);
    if (parts.empty()) return false;
    const Command* c = get(parts[0]);
    if (!c) {
        sender.sendMessage(S("cUnbekannter Befehl. Tippe ") + S("e/help") + S("c für eine Liste."));
        return false;
    }
    if (c->opOnly && !sender.senderIsOp()) {
        sender.sendMessage(S("cDazu hast du keine Berechtigung."));
        return false;
    }
    try {
        c->func(sender, std::vector<std::string>(parts.begin() + 1, parts.end()));
    } catch (const CommandError& e) {
        sender.sendMessage(S("c") + e.what());
    } catch (const std::exception& e) {
        logx::error("Fehler beim Ausführen von /" + c->name + ": " + e.what());
        sender.sendMessage(S("cBeim Ausführen des Befehls ist ein Fehler aufgetreten."));
    }
    return true;
}

std::vector<std::string> Commands::complete(CommandSender& sender, const std::string& line) {
    auto parts = split(line, ' ');
    const Command* c = get(parts.empty() ? "" : parts[0]);
    if (!c || (c->opOnly && !sender.senderIsOp())) return {};
    std::vector<std::string> args(parts.begin() + 1, parts.end());
    std::string last = args.empty() ? "" : toLower(args.back());
    std::vector<std::string> options;
    if (c->completer) options = c->completer(sender, args);
    else
        for (Player* p : server_.players()) options.push_back(p->name);
    std::vector<std::string> out;
    for (const auto& o : options)
        if (startsWith(toLower(o), last)) out.push_back(o);
    return out;
}

void Commands::registerBuiltins() {
    Server& srv = server_;
    add({"help", "Zeigt alle Befehle", "", {"?"}, false, [this](CommandSender& s, const auto&) {
             std::string msg = S("6--- ") + S("eCreeMC Befehle ") + S("6---");
             for (const auto& [n, c] : commands_)
                 if (!c.opOnly || s.senderIsOp()) msg += "\n" + S("e") + c.usage + S("7 - ") + c.description;
             s.sendMessage(msg);
         }});
    add({"list", "Zeigt alle Spieler online", "", {"online"}, false, [&srv](CommandSender& s, const auto&) {
             std::vector<std::string> names;
             for (Player* p : srv.players()) names.push_back(p->name);
             s.sendMessage(S("6Online (") + std::to_string(names.size()) + "/" + std::to_string(srv.config().maxPlayers) +
                           "): " + S("f") + (names.empty() ? "-" : join(names, 0, ", ")));
         }});
    add({"version", "Zeigt die Server-Version", "", {"ver", "about"}, false, [](CommandSender& s, const auto&) {
             std::string msg = S("aCreeMC ") + S("f0.2.0 ") + S("7(Minecraft ") + data::rangeText() + ") - C++-Server";
             if (Player* p = s.asPlayer()) msg += "\n" + S("7Dein Client: ") + S("f") + p->version->name;
             s.sendMessage(msg);
         }});
    add({"gamemode", "Ändert den Spielmodus", "/gamemode <modus> [spieler]", {"gm"}, true,
         [&srv](CommandSender& s, const std::vector<std::string>& a) {
             if (a.empty() || !GAMEMODES.count(toLower(a[0])))
                 throw CommandError("Benutzung: /gamemode <survival|creative|adventure|spectator> [spieler]");
             Player& t = a.size() > 1 ? findPlayer(srv, a[1]) : needPlayer(s);
             int mode = GAMEMODES.at(toLower(a[0]));
             if (t.setGamemode(mode)) {
                 t.sendMessage(S("aSpielmodus: ") + S("e") + GAMEMODE_NAMES[mode]);
                 if (&t != s.asPlayer()) s.sendMessage(S("a") + t.name + " ist jetzt im Modus " + S("e") + GAMEMODE_NAMES[mode]);
             }
         },
         [&srv](CommandSender&, const std::vector<std::string>& a) {
             if (a.size() <= 1) return std::vector<std::string>{"survival", "creative", "adventure", "spectator"};
             std::vector<std::string> n;
             for (Player* p : srv.players()) n.push_back(p->name);
             return n;
         }});
    add({"tp", "Teleportiert Spieler", "/tp <x y z | spieler> [ziel]", {"teleport"}, true,
         [&srv](CommandSender& s, const std::vector<std::string>& a) {
             Player* p = nullptr;
             if (a.size() == 1) {
                 p = &needPlayer(s);
                 Player& t = findPlayer(srv, a[0]);
                 p->teleport(t.x, t.y, t.z, t.yaw, t.pitch);
             } else if (a.size() == 2) {
                 p = &findPlayer(srv, a[0]);
                 Player& t = findPlayer(srv, a[1]);
                 p->teleport(t.x, t.y, t.z, t.yaw, t.pitch);
             } else if (a.size() == 3) {
                 p = &needPlayer(s);
                 p->teleport(coord(a[0], p->x), coord(a[1], p->y), coord(a[2], p->z));
             } else if (a.size() == 4) {
                 p = &findPlayer(srv, a[0]);
                 p->teleport(coord(a[1], p->x), coord(a[2], p->y), coord(a[3], p->z));
             } else {
                 throw CommandError("Benutzung: /tp <x y z> | /tp <spieler> [ziel] | /tp <spieler> <x y z>");
             }
             s.sendMessage(S("a") + p->name + " teleportiert nach " + S("e") + fmtDouble(p->x, 1) + " " + fmtDouble(p->y, 1) +
                           " " + fmtDouble(p->z, 1));
         }});
    add({"spawn", "Teleportiert dich zum Spawn", "", {}, false, [&srv](CommandSender& s, const auto&) {
             Player& p = needPlayer(s);
             double x, y, z;
             srv.world().spawnPoint(x, y, z);
             p.teleport(x, y, z);
             p.sendMessage(S("aZum Spawn teleportiert."));
         }});
    add({"say", "Sendet eine Nachricht an alle", "/say <nachricht>", {"broadcast"}, true,
         [&srv](CommandSender& s, const std::vector<std::string>& a) {
             if (a.empty()) throw CommandError("Benutzung: /say <nachricht>");
             srv.broadcast(S("d[") + s.senderName() + "] " + S("f") + join(a, 0));
         }});
    add({"msg", "Private Nachricht", "/msg <spieler> <nachricht>", {"tell", "w"}, false,
         [&srv](CommandSender& s, const std::vector<std::string>& a) {
             if (a.size() < 2) throw CommandError("Benutzung: /msg <spieler> <nachricht>");
             Player& t = findPlayer(srv, a[0]);
             std::string m = join(a, 1);
             t.sendMessage(S("7[") + s.senderName() + " -> dir] " + S("f") + m);
             s.sendMessage(S("7[du -> ") + t.name + "] " + S("f") + m);
         }});
    add({"op", "Gibt einem Spieler Operator-Rechte", "/op <spieler>", {}, true,
         [&srv](CommandSender& s, const std::vector<std::string>& a) {
             if (a.empty()) throw CommandError("Benutzung: /op <spieler>");
             srv.setOp(a[0], true);
             s.sendMessage(S("a") + a[0] + " ist jetzt Operator.");
             if (Player* p = srv.getPlayer(a[0])) p->sendMessage(S("aDu bist jetzt Operator."));
         }});
    add({"deop", "Entfernt Operator-Rechte", "/deop <spieler>", {}, true,
         [&srv](CommandSender& s, const std::vector<std::string>& a) {
             if (a.empty()) throw CommandError("Benutzung: /deop <spieler>");
             srv.setOp(a[0], false);
             s.sendMessage(S("e") + a[0] + " ist kein Operator mehr.");
         }});
    add({"kick", "Wirft einen Spieler raus", "/kick <spieler> [grund]", {}, true,
         [&srv](CommandSender& s, const std::vector<std::string>& a) {
             if (a.empty()) throw CommandError("Benutzung: /kick <spieler> [grund]");
             Player& t = findPlayer(srv, a[0]);
             std::string reason = join(a, 1);
             t.kick(reason.empty() ? S("cDu wurdest gekickt.") : reason);
             s.sendMessage(S("e") + t.name + " wurde gekickt.");
         }});
    add({"time", "Ändert die Uhrzeit", "/time <set|add> <wert|day|night>", {}, true,
         [&srv](CommandSender& s, const std::vector<std::string>& a) {
             if (a.size() != 2 || (a[0] != "set" && a[0] != "add"))
                 throw CommandError("Benutzung: /time <set|add> <wert|day|noon|night|midnight>");
             static const std::map<std::string, int> named = {{"day", 1000}, {"noon", 6000}, {"night", 13000}, {"midnight", 18000}};
             long long value;
             auto it = named.find(toLower(a[1]));
             if (it != named.end()) {
                 value = it->second;
             } else {
                 try {
                     value = std::stoll(a[1]);
                 } catch (...) {
                     throw CommandError("Ungültige Zeit");
                 }
             }
             World& w = srv.world();
             w.time = a[0] == "set" ? value : w.time + value;
             srv.sendTime();
             s.sendMessage(S("aZeit ist jetzt ") + S("e") + std::to_string(w.time % 24000));
         },
         [](CommandSender&, const std::vector<std::string>& a) {
             return a.size() <= 1 ? std::vector<std::string>{"set", "add"} : std::vector<std::string>{"day", "noon", "night", "midnight"};
         }});
    add({"setblock", "Setzt einen Block", "/setblock <x> <y> <z> <block>", {}, true,
         [&srv](CommandSender& s, const std::vector<std::string>& a) {
             if (a.size() != 4) throw CommandError("Benutzung: /setblock <x> <y> <z> <block>");
             Player* p = s.asPlayer();
             int x = int(std::floor(coord(a[0], p ? p->x : 0))), y = int(std::floor(coord(a[1], p ? p->y : 0))),
                 z = int(std::floor(coord(a[2], p ? p->z : 0)));
             // Vanilla-Schreibweise mit Zuständen: lever[face=wall,facing=west,powered=true]
             std::string spec = a[3], blockName = spec;
             std::map<std::string, std::string> stateProps;
             size_t br = spec.find('[');
             if (br != std::string::npos) {
                 blockName = spec.substr(0, br);
                 std::string inner = spec.substr(br + 1);
                 if (!inner.empty() && inner.back() == ']') inner.pop_back();
                 for (const auto& kv : split(inner, ',', true)) {
                     size_t eq = kv.find('=');
                     if (eq != std::string::npos) stateProps[trim(kv.substr(0, eq))] = trim(kv.substr(eq + 1));
                 }
             }
             int state = data::blockState(blockName);
             if (state < 0) throw CommandError("Unbekannter Block: " + blockName);
             if (!stateProps.empty()) state = data::withProps(state, stateProps);
             srv.setBlockState(x, y, z, state);
             s.sendMessage(S("aBlock bei ") + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z) + " gesetzt.");
         }});
    add({"give", "Gibt dir Items", "/give <item> [anzahl] [spieler]", {"gib", "i"}, true,
         [&srv](CommandSender& s, const std::vector<std::string>& a) {
             if (a.empty()) throw CommandError("Benutzung: /give <item> [anzahl] [spieler]");
             std::string item = ns(a[0]);
             if (!data::isItem(item)) throw CommandError("Unbekanntes Item: " + a[0]);
             int count = 1;
             if (a.size() > 1) {
                 try {
                     count = std::max(1, std::stoi(a[1]));
                 } catch (...) {
                     throw CommandError("Ungültige Anzahl: " + a[1]);
                 }
             }
             Player& t = a.size() > 2 ? findPlayer(srv, a[2]) : needPlayer(s);
             int rest = t.addItem(item, count);
             t.sendMessage(S("a") + std::to_string(count - rest) + "x " + removePrefix(item, "minecraft:") + " erhalten.");
         }});
    add({"plugins", "Zeigt geladene Plugins", "", {"pl"}, false, [&srv](CommandSender& s, const auto&) {
             const auto& pl = srv.bridge().plugins;
             std::string names;
             for (const auto& p : pl) names += (names.empty() ? "" : S("f, ")) + S("a") + p;
             s.sendMessage(S("6Plugins (") + std::to_string(pl.size()) + "): " + (names.empty() ? S("7keine") : names));
         }});
    add({"save", "Speichert die Welt", "/save", {"save-all"}, true, [&srv](CommandSender& s, const auto&) {
             srv.save();
             s.sendMessage(S("aWelt gespeichert."));
         }});
    add({"tps", "Zeigt die Server-Leistung", "/tps", {}, false, [&srv](CommandSender& s, const auto&) {
             s.sendMessage(S("6TPS: ") + S("a") + fmtDouble(std::min(20.0, srv.tps()), 1));
         }});
    add({"stop", "Stoppt den Server", "/stop", {}, true, [&srv](CommandSender&, const auto&) { srv.requestStop(); }});
}
