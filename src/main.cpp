#include <vector>
#include "core/rng.hpp"
#include "game/loot_tables.hpp"
#include "game/encounter.hpp"

using namespace game;

int main() {
    core::RNG rng(1337); // set a seed for reproducibility
    LootTables loot = makeDefaultLoot();

    // Player starting gear
    Weapon starter{ WeaponBase{"Rusty Sword", 2, 6}, Rarity::Common, {} };
    Actor player{ "Player", 60, 60, 1, starter };

    // Enemies
    Weapon goblinW{ WeaponBase{"Shiv",   1, 4}, Rarity::Common, {} };
    Weapon bruteW { WeaponBase{"Club",   3, 7}, Rarity::Common, {} };
    Weapon raiderW{ WeaponBase{"Hatchet",2, 6}, Rarity::Common, {} };

    std::vector<Actor> pack = {
        Actor{"Goblin", 20, 20, 0, goblinW},
        Actor{"Brute",  35, 35, 1, bruteW },
        Actor{"Raider", 25, 25, 0, raiderW}
    };

    Encounter enc{player, pack, loot, rng};
    enc.run();
    return 0;
}
