// Inventar-Fenster und Klick-Logik (serverseitig, wie Vanilla).
// Fenster 0 (Inventar): 0 Ergebnis, 1-4 Crafting 2x2, 5-8 Rüstung, 9-35 Inventar, 36-44 Hotbar, 45 Zweithand.
#pragma once
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class Player;
using Stack = std::pair<std::string, int>;

class Window {
public:
    Window(Player& p, int id, int gridW, int size) : player(p), id(id), gridW(gridW), size(size) {}
    virtual ~Window() = default;

    Player& player;
    int id, gridW, size;
    int typeId = -1;
    std::string title;
    int resultSlot = -1;
    Stack result;

    virtual std::optional<int> invSlot(int i) const = 0;   // Fenster-Slot -> Inventar-Slot
    virtual std::vector<int> quickMoveTargets(int i) const = 0;
    virtual bool canPlace(int i, const std::string& item) const { return i != resultSlot; }
    virtual Stack getGrid(int) const { return {}; }
    virtual void setGrid(int, const Stack&) {}
    virtual std::vector<int> gridSlots() const;
    virtual void updateResult();
    virtual bool takeResult();
    virtual std::vector<Stack> returnItems();

    Stack get(int i) const;
    void set(int i, Stack st);
    std::vector<Stack> allSlots() const;
    void click(int slot, int button, int mode);

private:
    void clickSlot(int slot, int button);
    void clickResult();
    int insert(const std::string& item, int count, const std::vector<int>& targets);
    void quickMove(int slot);
    bool fitsIn(const Stack& st, const std::vector<int>& targets) const;
    void swap(int slot, int button);
    void dragClick(int slot, int button);
    void collect(Stack cursor);
    std::optional<std::pair<int, std::vector<int>>> drag_;
};

class PlayerWindow : public Window {
public:
    explicit PlayerWindow(Player& p) : Window(p, 0, 2, 46) { resultSlot = 0; }
    std::optional<int> invSlot(int i) const override;
    std::vector<int> quickMoveTargets(int i) const override;
};

class CraftingWindow : public Window {
public:
    CraftingWindow(Player& p, int id) : Window(p, id, 3, 46) {
        resultSlot = 0;
        typeId = 12;
        title = "Werkbank";
    }
    std::optional<int> invSlot(int i) const override;
    std::vector<int> quickMoveTargets(int i) const override;
    Stack getGrid(int i) const override { return i >= 1 && i <= 9 ? grid_[size_t(i - 1)] : Stack{}; }
    void setGrid(int i, const Stack& st) override {
        if (i >= 1 && i <= 9) grid_[size_t(i - 1)] = st;
    }

private:
    Stack grid_[9];
};

std::optional<int> armorSlotFor(const std::string& item);
