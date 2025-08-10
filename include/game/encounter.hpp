#pragma once
#include <vector>
#include "game/actor.hpp"
#include "game/loot_tables.hpp"
#include "game/inventory.hpp"
#include "core/rng.hpp"

namespace game {

struct Encounter {
    Actor player;
    std::vector<Actor> enemies;
    LootTables loots;
    Inventory& inventory;     // NEW
    core::RNG& rng;

    void run();
};

} // namespace game
