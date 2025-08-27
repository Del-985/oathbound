#include "game/loot_tables.hpp"
#include "game/item.hpp"
#include <algorithm>

namespace game {

static int clampi(int v, int a, int b){ return v < a ? a : (v > b ? b : v); }

Item LootTables::rollWeapon(core::RNG& rng, int /*level*/) const {
    Rarity r = rarity.pick(rng);
    WeaponBase wb = bases.pick(rng);

    Item w;
    w.name    = wb.name;
    w.rarity  = r;
    w.kind    = ItemKind::Weapon;
    w.slot    = Slot::Weapon;
    w.baseMin = wb.baseMin;
    w.baseMax = wb.baseMax;

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

Item LootTables::rollGear(core::RNG& rng, int /*level*/) const {
    Rarity r = rarity.pick(rng);
    GearBase gb = gearBases.pick(rng);

    Item g;
    g.name        = gb.name;
    g.rarity      = r;
    g.kind        = ItemKind::Gear;
    g.slot        = gb.slot;
    g.armorBonus  = clampi(rng.i(gb.armorMin, gb.armorMax), 0, 999);

    // naive type by armor amount (tweak as you like)
    if (g.slot == Slot::Armor || g.slot == Slot::Helmet || g.slot == Slot::Boots || g.slot == Slot::Belt) {
        g.armorType = (g.armorBonus >= 3 ? ArmorType::Heavy : (g.armorBonus >= 1 ? ArmorType::Medium : ArmorType::Light));
    }

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

    for (int i = 0; i < preCount && !prefixes.empty(); ++i) g.affixes.push_back(pickAffix(prefixes));
    for (int i = 0; i < sufCount && !suffixes.empty(); ++i) g.affixes.push_back(pickAffix(suffixes));

    return g;
}

bool LootTables::rollIsGear(core::RNG& rng) const {
    if (dropType.empty()) return false;
    return dropType.pick(rng) == 1;
}

LootTables makeDefaultLoot() {
    LootTables lt;

    // Drop kind weights
    lt.dropType.add(0, 70); // weapon
    lt.dropType.add(1, 30); // gear

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

    // Base gear
    lt.gearBases.add(GearBase{Slot::Offhand, "Wooden Shield", 1, 3}, 18);
    lt.gearBases.add(GearBase{Slot::Offhand, "Bronze Shield", 2, 5}, 12);

    lt.gearBases.add(GearBase{Slot::Armor, "Leather Armor", 2, 5}, 22);
    lt.gearBases.add(GearBase{Slot::Armor, "Chainmail",     3, 7}, 12);

    lt.gearBases.add(GearBase{Slot::Helmet, "Cloth Hood",   0, 2}, 18);
    lt.gearBases.add(GearBase{Slot::Helmet, "Iron Helm",    1, 3}, 12);

    lt.gearBases.add(GearBase{Slot::Boots, "Traveler's Boots", 0, 2}, 20);
    lt.gearBases.add(GearBase{Slot::Boots, "Greaves",          1, 3}, 10);

    lt.gearBases.add(GearBase{Slot::Belt, "Rope Belt", 0, 0}, 16);
    lt.gearBases.add(GearBase{Slot::Belt, "Studded Belt", 0, 1}, 10);

    lt.gearBases.add(GearBase{Slot::Amulet, "Amulet", 0, 0}, 18);
    lt.gearBases.add(GearBase{Slot::Ring1, "Copper Ring", 0, 0}, 18);
    lt.gearBases.add(GearBase{Slot::Ring1, "Silver Ring", 0, 0}, 12);

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