#pragma once
#include <vector>
#include <cstddef>
#include <optional>
#include "game/weapon.hpp"

namespace game {

class Inventory {
public:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    // Add an item; returns its index.
    std::size_t add(Weapon w);

    // Equip by index; returns false if index invalid.
    bool equip(std::size_t idx);

    // Equip the highest expected DPR weapon. Returns false if empty.
    bool equipBest();

    // Accessors
    const Weapon* equipped() const;
    Weapon*       equipped();

    const Weapon& at(std::size_t idx) const { return items_.at(idx); }
    Weapon&       at(std::size_t idx)       { return items_.at(idx); }

    std::size_t size() const { return items_.size(); }
    std::size_t equippedIndex() const { return equippedIndex_; }

private:
    std::vector<Weapon> items_;
    std::size_t equippedIndex_ = npos;
};

} // namespace game
