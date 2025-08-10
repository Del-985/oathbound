#include <vector>
#include "core/rng.hpp"
#include "game/loot_tables.hpp"
#include "game/encounter.hpp"
#include "game/inventory.hpp"

using namespace game;

int main() {
    core::RNG rng(1337);
    LootTables loot = makeDefaultLoot();

    // Inventory + starter
    Inventory inv;
    std::size_t starterIdx = inv.add(Weapon{ WeaponBase{"Rusty Sword", 2, 6}, Rarity::Common, {} });
    inv.equip(starterIdx);

    // Player uses equipped item
    Actor player{ "Player", 60, 60, 1, *inv.equipped() };

    // Enemies
    Weapon goblinW{ WeaponBase{"Shiv",   1, 4}, Rarity::Common, {} };
    Weapon bruteW { WeaponBase{"Club",   3, 7}, Rarity::Common, {} };
    Weapon raiderW{ WeaponBase{"Hatchet",2, 6}, Rarity::Common, {} };

    std::vector<Actor> pack = {
        Actor{"Goblin", 20, 20, 0, goblinW},
        Actor{"Brute",  35, 35, 1, bruteW },
        Actor{"Raider", 25, 25, 0, raiderW}
    };

    Encounter enc{player, pack, loot, inv, rng}; // pass Inventory&
    enc.run();
    return 0;
}
