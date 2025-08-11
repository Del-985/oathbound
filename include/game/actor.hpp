#pragma once
#include <string>
#include "game/weapon.hpp"
#include "core/rng.hpp"

namespace game {

struct Actor {
    std::string name;
    int maxHP = 1;
    int hp    = 1;
    int armor = 0;        // flat reduction (will include gear bonuses)
    Weapon weapon;

    bool alive() const { return hp > 0; }
    // extraPct/extraCrit allow gear effects to influence damage
    int  attack(Actor& target, core::RNG& rng, double extraPct=0.0, double extraCrit=0.0) const;
};

} // namespace game
