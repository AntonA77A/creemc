// Blöcke mit Funktion: Kisten, Fässer, Öfen, Trichter, Spender/Werfer, Endertruhe, Werkbank,
// Türen, Falltüren, Zauntore, Knöpfe, Hebel, Druckplatten, Verstärker, Komparator, Notenblock.
// Inhalte liegen in World::blockData (gleiches Format wie die Python-Version: {"items": [[item, n] | null, ...]}).
#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "inventory.hpp"
#include "world.hpp"

class Server;
class Player;

namespace blocks {

// Ein Speicher mit festen Slots (mehrere Spieler können ihn gleichzeitig ansehen)
class Storage {
public:
    virtual ~Storage() = default;
    virtual int size() const = 0;
    virtual Stack get(int i) const = 0;
    virtual void set(int i, const Stack& st) = 0;
    virtual bool covers(const BlockPos& p) const { (void)p; return false; }
};

std::unique_ptr<Storage> blockStorage(Server& server, const BlockPos& pos, int size);

class ContainerWindow : public Window {
public:
    ContainerWindow(Player& p, int id, std::unique_ptr<Storage> storage, int menuType, const std::string& title);
    std::unique_ptr<Storage> storage;
    int n;
    std::vector<BlockPos> positions;   // Blöcke, zu denen das Fenster gehört (Doppelkiste: zwei)
    bool temporary = false;            // eigene Slots gehören niemandem -> beim Schließen zurückgeben
    std::optional<int> invSlot(int i) const override;
    std::vector<int> quickMoveTargets(int i) const override;
    Stack getGrid(int i) const override { return i >= 0 && i < n ? storage->get(i) : Stack{}; }
    void setGrid(int i, const Stack& st) override {
        if (i >= 0 && i < n) storage->set(i, st);
    }
    std::vector<int> gridSlots() const override { return {}; }
    std::vector<Stack> returnItems() override;
};

class FurnaceWindow : public ContainerWindow {
public:
    FurnaceWindow(Player& p, int id, std::unique_ptr<Storage> storage, int menuType, const std::string& title, std::string kind);
    std::string kind;
    bool canPlace(int i, const std::string& item) const override;
    std::vector<int> quickMoveTargets(int i) const override;
};

int fuelTicks(const std::string& item);
bool isInteractive(const std::string& block);
// Rechtsklick auf einen Block -> true, wenn etwas geöffnet/geschaltet wurde
bool openBlock(Player& p, const std::string& block, int x, int y, int z);
void onClose(Player& p, Window& w);
void onPlaced(Server& server, int x, int y, int z, const std::string& block);
void dropContents(Server& server, int x, int y, int z);   // beim Abbauen fällt der Inhalt heraus
void syncViewers(Server& server, Player* except, const std::vector<BlockPos>& positions);
void dispense(Server& server, int x, int y, int z);
void tick(Server& server);

}  // namespace blocks
