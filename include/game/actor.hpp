#pragma once
#include <string>
#include "game/weapon.hpp"
#include "core/rng.hpp"

namespace game {

struct Actor {
    std::string name;
    int maxHP = 1;
    int hp    = 1;
    int armor = 0;        // flat reduction
    Weapon weapon;

    bool alive() const { return hp > 0; }
    int  attack(Actor& target, core::RNG& rng) const; // returns damage dealt (post-armor)
};

} // namespace game
