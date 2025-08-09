#pragma once
#include <string>
#include <vector>
#include "game/weapon_base.hpp"
#include "game/rarity.hpp"
#include "game/affix.hpp"
#include "core/rng.hpp"

namespace game {

struct Weapon {
    WeaponBase base;
    Rarity rarity;
    std::vector<Affix> affixes;

    int    minDmg() const;
    int    maxDmg() const;
    double pctDamage() const;
    double critChance() const;
    double critMult() const;
    double attackSpeed() const;

    int         rollDamage(core::RNG& rng) const;
    std::string label() const;
};

} // namespace game
