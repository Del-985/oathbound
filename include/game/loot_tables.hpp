#pragma once
#include <vector>
#include <string>
#include "core/weighted_table.hpp"
#include "game/rarity.hpp"
#include "game/weapon_base.hpp"
#include "game/affix.hpp"
#include "game/item.hpp"
#include "core/rng.hpp"
#include "game/slots.hpp"

namespace game {

struct GearBase {
    Slot        slot;
    std::string name;
    int         armorMin = 0;
    int         armorMax = 0;
};

struct LootTables {
    core::WeightedTable<int>        dropType;   // 0 = weapon, 1 = gear
    core::WeightedTable<Rarity>     rarity;
    core::WeightedTable<WeaponBase> bases;
    core::WeightedTable<GearBase>   gearBases;
    std::vector<Affix> prefixes;
    std::vector<Affix> suffixes;

    Item rollWeapon(core::RNG& rng, int level) const;  // kind==Weapon
    Item rollGear(core::RNG& rng, int level) const;    // kind==Gear
    bool rollIsGear(core::RNG& rng) const;             // uses dropType
};

LootTables makeDefaultLoot();

} // namespace game