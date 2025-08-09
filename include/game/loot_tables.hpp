#pragma once
#include <vector>
#include "core/weighted_table.hpp"
#include "game/rarity.hpp"
#include "game/weapon_base.hpp"
#include "game/affix.hpp"
#include "game/weapon.hpp"
#include "core/rng.hpp"

namespace game {

struct LootTables {
    core::WeightedTable<Rarity>     rarity;
    core::WeightedTable<WeaponBase> bases;
    std::vector<Affix> prefixes;
    std::vector<Affix> suffixes;

    Weapon rollWeapon(core::RNG& rng, int level) const;
};

LootTables makeDefaultLoot();

} // namespace game
