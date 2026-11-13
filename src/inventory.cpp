#include "inventory.hpp"

#include <algorithm>

#include "data.hpp"
#include "player.hpp"
#include "server.hpp"
#include "util.hpp"

static constexpr int OUTSIDE = -999;

std::optional<int> armorSlotFor(const std::string& item) {
    std::string b = data::baseItem(item);
    if (endsWith(b, "_helmet") || endsWith(b, "carved_pumpkin") || endsWith(b, "_head") || endsWith(b, "_skull")) return 5;
    if (endsWith(b, "_chestplate") || endsWith(b, "elytra")) return 6;
    if (endsWith(b, "_leggings")) return 7;
    if (endsWith(b, "_boots")) return 8;
    return std::nullopt;
}

static bool empty(const Stack& s) { return s.first.empty() || s.second <= 0; }

std::vector<int> Window::gridSlots() const {
    std::vector<int> out;
    for (int i = 1; i <= gridW * gridW; ++i) out.push_back(i);
    return out;
}

Stack Window::get(int i) const {
    if (i == resultSlot) return result;
    auto n = invSlot(i);
    if (n) {
        auto it = player.inventory.find(*n);
        return it == player.inventory.end() ? Stack{} : it->second;
    }
    return getGrid(i);
}

void Window::set(int i, Stack st) {
    if (st.second <= 0) st = {};
    if (i == resultSlot) return;
    auto n = invSlot(i);
    if (n) {
        if (empty(st)) player.inventory.erase(*n);
        else player.inventory[*n] = st;
    } else {
        setGrid(i, st);
    }
}

std::vector<Stack> Window::allSlots() const {
    std::vector<Stack> out;
    for (int i = 0; i < size; ++i) out.push_back(get(i));
    return out;
}

void Window::updateResult() {
    if (!gridW) return;
    std::vector<std::string> grid;
    for (int i : gridSlots()) grid.push_back(get(i).first);
    auto r = player.server.recipes().match(grid, gridW);
    result = r ? Stack{r->first, r->second} : Stack{};
}

bool Window::takeResult() {
    if (empty(result)) return false;
    for (int i : gridSlots()) {
        Stack st = get(i);
        if (!empty(st)) set(i, {st.first, st.second - 1});
    }
    updateResult();
    return true;
}

void Window::click(int slot, int button, int mode) {
    Stack& cur = player.cursor;
    if (mode == 0) {
        if (slot == OUTSIDE) {
            if (!empty(cur)) {
                int n = button == 0 ? cur.second : 1;
                Stack drop{cur.first, n};
                cur = cur.second > n ? Stack{cur.first, cur.second - n} : Stack{};
                player.dropStack(drop.first, drop.second);
            }
        } else if (slot >= 0 && slot < size) {
            if (slot == resultSlot) clickResult();
            else clickSlot(slot, button);
        }
    } else if (mode == 1 && slot >= 0 && slot < size) {
        quickMove(slot);
    } else if (mode == 2 && slot >= 0 && slot < size) {
        swap(slot, button);
    } else if (mode == 3 && player.gamemode == 1 && slot >= 0 && slot < size && empty(cur)) {
        Stack st = get(slot);
        if (!empty(st)) cur = {st.first, data::stackSize(st.first)};
    } else if (mode == 4 && slot >= 0 && slot < size) {
        Stack st = get(slot);
        if (slot == resultSlot) {
            if (!empty(st) && takeResult()) player.dropStack(st.first, st.second);
        } else if (!empty(st)) {
            int n = button == 0 ? 1 : st.second;
            set(slot, {st.first, st.second - n});
            player.dropStack(st.first, n);
        }
    } else if (mode == 5) {
        dragClick(slot, button);
    } else if (mode == 6 && !empty(cur)) {
        collect(cur);
    }
    updateResult();
}

void Window::clickSlot(int slot, int button) {
    Stack& cur = player.cursor;
    Stack st = get(slot);
    if (slot >= 5 && slot <= 8 && !empty(cur) && armorSlotFor(cur.first) != slot && id == 0) return;
    if (!empty(cur) && !canPlace(slot, cur.first)) {
        if (empty(st) && button == 0) return;
        if (!empty(st) && st.first == cur.first && button == 0) {
            int take = std::min(data::stackSize(st.first) - cur.second, st.second);
            cur.second += take;
            set(slot, {st.first, st.second - take});
            return;
        }
        if (!empty(st) && st.first != cur.first) return;
    }
    if (button == 0) {
        if (empty(cur)) {
            cur = st;
            st = {};
        } else if (empty(st)) {
            st = cur;
            cur = {};
        } else if (st.first == cur.first) {
            int move = std::min(data::stackSize(st.first) - st.second, cur.second);
            st.second += move;
            cur = cur.second > move ? Stack{cur.first, cur.second - move} : Stack{};
        } else {
            std::swap(st, cur);
        }
    } else {
        if (empty(cur)) {
            if (!empty(st)) {
                int half = (st.second + 1) / 2;
                cur = {st.first, half};
                st = st.second > half ? Stack{st.first, st.second - half} : Stack{};
            }
        } else if (empty(st)) {
            st = {cur.first, 1};
            cur = cur.second > 1 ? Stack{cur.first, cur.second - 1} : Stack{};
        } else if (st.first == cur.first) {
            if (st.second < data::stackSize(st.first)) {
                st.second += 1;
                cur = cur.second > 1 ? Stack{cur.first, cur.second - 1} : Stack{};
            }
        } else {
            std::swap(st, cur);
        }
    }
    set(slot, st);
}

void Window::clickResult() {
    Stack& cur = player.cursor;
    Stack res = result;
    if (empty(res)) return;
    if (!empty(cur) && (cur.first != res.first || cur.second + res.second > data::stackSize(res.first))) return;
    if (takeResult()) cur = {res.first, res.second + (empty(cur) ? 0 : cur.second)};
}

int Window::insert(const std::string& item, int count, const std::vector<int>& targets) {
    int maxStack = data::stackSize(item);
    for (bool wantEmpty : {false, true}) {
        for (int t : targets) {
            if (count <= 0) return 0;
            if (!canPlace(t, item)) continue;
            Stack st = get(t);
            if (wantEmpty && empty(st)) {
                int move = std::min(count, maxStack);
                set(t, {item, move});
                count -= move;
            } else if (!wantEmpty && !empty(st) && st.first == item && st.second < maxStack) {
                int move = std::min(count, maxStack - st.second);
                set(t, {item, st.second + move});
                count -= move;
            }
        }
    }
    return count;
}

bool Window::fitsIn(const Stack& st, const std::vector<int>& targets) const {
    int room = 0;
    for (int t : targets) {
        Stack s = get(t);
        if (empty(s)) room += data::stackSize(st.first);
        else if (s.first == st.first) room += data::stackSize(st.first) - s.second;
        if (room >= st.second) return true;
    }
    return false;
}

void Window::quickMove(int slot) {
    if (slot == resultSlot) {
        for (int i = 0; i < 64; ++i) {
            Stack res = result;
            if (empty(res) || !fitsIn(res, quickMoveTargets(0))) break;
            if (!takeResult()) break;
            insert(res.first, res.second, quickMoveTargets(0));
        }
        return;
    }
    Stack st = get(slot);
    if (empty(st)) return;
    int rest = insert(st.first, st.second, quickMoveTargets(slot));
    set(slot, rest ? Stack{st.first, rest} : Stack{});
}

void Window::swap(int slot, int button) {
    int invN = button == 40 ? 45 : (button >= 0 && button <= 8 ? 36 + button : -1);
    if (invN < 0) return;
    auto& inv = player.inventory;
    if (slot == resultSlot) {
        Stack res = result;
        if (!empty(res) && !inv.count(invN) && takeResult()) inv[invN] = res;
        return;
    }
    Stack st = get(slot);
    Stack other = inv.count(invN) ? inv[invN] : Stack{};
    if (invSlot(slot) == invN || (!empty(other) && !canPlace(slot, other.first))) return;
    set(slot, other);
    if (!empty(st)) inv[invN] = st;
    else inv.erase(invN);
}

void Window::dragClick(int slot, int button) {
    int stage = button & 3, kind = button >> 2;   // 0 links, 1 rechts, 2 Mitte (Kreativ)
    if (stage == 0) {
        drag_ = std::make_pair(kind, std::vector<int>{});
    } else if (stage == 1 && drag_ && slot >= 0 && slot < size && slot != resultSlot) {
        auto& v = drag_->second;
        if (std::find(v.begin(), v.end(), slot) == v.end()) v.push_back(slot);
    } else if (stage == 2 && drag_) {
        auto [k, slots] = *drag_;
        drag_.reset();
        Stack& cur = player.cursor;
        if (empty(cur) || slots.empty()) return;
        std::string item = cur.first;
        int count = cur.second, maxStack = data::stackSize(item);
        std::vector<int> ok;
        for (int s : slots) {
            Stack st = get(s);
            if ((empty(st) || st.first == item) && canPlace(s, item)) ok.push_back(s);
        }
        if (ok.empty()) return;
        if (k == 2) {
            if (player.gamemode == 1)
                for (int s : ok) set(s, {item, maxStack});
            return;
        }
        int per = k == 1 ? 1 : std::max(1, count / int(ok.size()));
        for (int s : ok) {
            if (count <= 0) break;
            Stack st = get(s);
            int have = empty(st) ? 0 : st.second;
            int move = std::min({per, count, maxStack - have});
            if (move > 0) {
                set(s, {item, have + move});
                count -= move;
            }
        }
        cur = count > 0 ? Stack{item, count} : Stack{};
    }
}

void Window::collect(Stack cur) {
    int maxStack = data::stackSize(cur.first);
    for (int i = 0; i < size; ++i) {
        if (i == resultSlot) continue;
        if (cur.second >= maxStack) break;
        Stack st = get(i);
        if (!empty(st) && st.first == cur.first) {
            int take = std::min(st.second, maxStack - cur.second);
            set(i, {st.first, st.second - take});
            cur.second += take;
        }
    }
    player.cursor = cur;
}

std::vector<Stack> Window::returnItems() {
    std::vector<Stack> stacks, leftovers;
    for (int i : gridSlots()) {
        stacks.push_back(get(i));
        set(i, {});
    }
    if (!empty(player.cursor)) {
        stacks.push_back(player.cursor);
        player.cursor = {};
    }
    for (const auto& st : stacks) {
        if (empty(st)) continue;
        int rest = player.addItem(st.first, st.second, false);
        if (rest) leftovers.push_back({st.first, rest});
    }
    result = {};
    return leftovers;
}

// ---- Fenster-Typen ----

std::optional<int> PlayerWindow::invSlot(int i) const {
    if (i >= 1 && i <= 45) return i;
    return std::nullopt;
}

std::vector<int> PlayerWindow::quickMoveTargets(int i) const {
    auto range = [](int a, int b) {
        std::vector<int> v;
        for (int k = a; k < b; ++k) v.push_back(k);
        return v;
    };
    if (i == 0 || (i >= 1 && i <= 8) || i == 45) return range(9, 45);
    Stack st = get(i);
    auto armor = empty(st) ? std::nullopt : armorSlotFor(st.first);
    if (armor && empty(get(*armor))) return {*armor};
    if (i >= 9 && i <= 35) return range(36, 45);
    return range(9, 36);
}

std::optional<int> CraftingWindow::invSlot(int i) const {
    if (i >= 10 && i <= 45) return i - 1;
    return std::nullopt;
}

std::vector<int> CraftingWindow::quickMoveTargets(int i) const {
    auto range = [](int a, int b) {
        std::vector<int> v;
        for (int k = a; k < b; ++k) v.push_back(k);
        return v;
    };
    if (i >= 0 && i <= 9) return range(10, 46);
    if (i >= 10 && i <= 36) return range(37, 46);
    return range(10, 37);
}
