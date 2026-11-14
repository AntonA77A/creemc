// Erweiterte Plugin-API auf Server-Seite: Bossbars, Scoreboards, Teams, Partikel, eigene Inventare (GUIs),
// Entity-Funktionen und zusätzliche Events für die Java-Bridge.
#pragma once
#include <string>

#include "json.hpp"

class Server;
class Player;
class Entity;
class Window;

namespace api {

// Aufträge/Fragen aus Java, die hier behandelt werden. -> true, wenn erledigt
bool handleCall(Server& s, const json::Value& m);
bool handleRequest(Server& s, const json::Value& m, json::Value& out);

json::Object entitySnap(Entity* e);

// Events an Java-Plugins. -> true = abgebrochen
bool fire(Server& s, const std::string& name, Player* p, json::Object data = {});
bool inventoryClick(Player& p, int slot, int button, int mode);   // vor dem Klick
void afterInventoryClick(Player& p);                              // nach dem Klick (GUI-Inhalt an Java)
bool inventoryOpen(Player& p, Window& w);
void inventoryClose(Player& p, Window& w);

}  // namespace api
