#include "game/loot_tables.hpp"

namespace game {

Weapon LootTables::rollWeapon(core::RNG& rng, int /*level*/) const {
    Rarity r = rarity.pick(rng);
    WeaponBase wb = bases.pick(rng);

    Weapon w{wb, r, {}};

    int preCount = 0, sufCount = 0;
    switch (r) {
        case Rarity::Common:    preCount=0; sufCount=0; break;
        case Rarity::Magic:     preCount=1; sufCount=0; break;
        case Rarity::Rare:      preCount=1; sufCount=1; break;
        case Rarity::Epic:      preCount=2; sufCount=1; break;
        case Rarity::Legendary: preCount=2; sufCount=2; break;
    }

    auto pickAffix = [&](const std::vector<Affix>& pool){
        return pool.empty() ? Affix{} : pool[ static_cast<size_t>(rng.i(0, static_cast<int>(pool.size()) - 1)) ];
    };

    for (int i = 0; i < preCount && !prefixes.empty(); ++i) w.affixes.push_back(pickAffix(prefixes));
    for (int i = 0; i < sufCount && !suffixes.empty(); ++i) w.affixes.push_back(pickAffix(suffixes));

    return w;
}

LootTables makeDefaultLoot() {
    LootTables lt;

    // Rarity weights
    lt.rarity.add(Rarity::Common,    60);
    lt.rarity.add(Rarity::Magic,     25);
    lt.rarity.add(Rarity::Rare,      10);
    lt.rarity.add(Rarity::Epic,       4);
    lt.rarity.add(Rarity::Legendary,  1);

    // Base weapons
    lt.bases.add(WeaponBase{"Shortsword", 3, 7},  25);
    lt.bases.add(WeaponBase{"Longsword",  5, 11}, 25);
    lt.bases.add(WeaponBase{"Axe",        6, 13}, 20);
    lt.bases.add(WeaponBase{"Mace",       7, 12}, 15);
    lt.bases.add(WeaponBase{"Spear",      4, 10}, 15);

    // Prefixes
    lt.prefixes = {
        Affix::Prefix("Jagged",     1, 2,  0.00, 0.00, 0.00),
        Affix::Prefix("Heavy",      2, 4,  0.10, 0.00, -0.05),
        Affix::Prefix("Keen",       0, 0,  0.00, 0.05, 0.00),
        Affix::Prefix("Swift",      0, 0,  0.00, 0.00, 0.15),
        Affix::Prefix("Brutal",     2, 3,  0.20, 0.02, -0.05),
    };

    // Suffixes
    lt.suffixes = {
        Affix::Suffix("of Embers",  0, 0,  0.12, 0.00, 0.00),
        Affix::Suffix("of Frost",   0, 0,  0.10, 0.02, 0.00),
        Affix::Suffix("of Haste",   0, 0,  0.00, 0.00, 0.20),
        Affix::Suffix("of Slaying", 1, 2,  0.08, 0.03, 0.00),
        Affix::Suffix("of Mauling", 3, 3,  0.00, 0.00, -0.05),
    };

    return lt;
}

} // namespace game
