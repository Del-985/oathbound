#include "game/inventory.hpp"
#include "game/combat_math.hpp"
#include <limits>

namespace game {

std::size_t Inventory::add(Weapon w) {
    items_.push_back(std::move(w));
    return items_.size() - 1;
}

bool Inventory::equip(std::size_t idx) {
    if (idx >= items_.size()) return false;
    equippedIndex_ = idx;
    return true;
}

bool Inventory::equipBest() {
    if (items_.empty()) return false;
    double best = -std::numeric_limits<double>::infinity();
    std::size_t bestIdx = 0;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const double dpr = expectedDPR(items_[i]);
        if (dpr > best) { best = dpr; bestIdx = i; }
    }
    return equip(bestIdx);
}

const Weapon* Inventory::equipped() const {
    if (equippedIndex_ == npos || equippedIndex_ >= items_.size()) return nullptr;
    return &items_[equippedIndex_];
}

Weapon* Inventory::equipped() {
    if (equippedIndex_ == npos || equippedIndex_ >= items_.size()) return nullptr;
    return &items_[equippedIndex_];
}

} // namespace game

