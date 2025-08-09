#include "game/encounter.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace game {

void Encounter::run() {
    std::cout << "You wield " << player.weapon.label() << "\n\n";
    int round = 1;

    auto anyEnemiesAlive = [&](){
        for (const auto& e : enemies) if (e.alive()) return true;
        return false;
    };

    while (player.alive() && anyEnemiesAlive()) {
        std::cout << "=== Round " << round++ << " ===\n";

        // Player turn: attack first alive enemy
        auto it = std::find_if(enemies.begin(), enemies.end(), [](const Actor& e){ return e.alive(); });
        if (it != enemies.end()) {
            Actor& target = *it;
            int hits = std::max(1, static_cast<int>(std::round(player.weapon.attackSpeed())));
            for (int h = 0; h < hits && target.alive(); ++h) {
                int dmg = player.attack(target, rng);
                target.hp -= dmg;
                std::cout << "You hit " << target.name << " for " << dmg
                          << " (" << std::max(0, target.hp) << "/" << target.maxHP << ")\n";
            }
            if (!target.alive()) {
                std::cout << target.name << " is slain!\n";
                // Drop and naive auto-equip if higher max damage
                Weapon drop = loots.rollWeapon(rng, /*level*/1);
                std::cout << "Loot dropped: " << drop.label() << "\n";
                if (drop.maxDmg() > player.weapon.maxDmg()) {
                    std::cout << "You equip the new weapon.\n";
                    player.weapon = drop;
                }
            }
        }

        // Enemies' turn
        for (auto& e : enemies) {
            if (!e.alive() || !player.alive()) continue;
            int hits = std::max(1, static_cast<int>(std::round(e.weapon.attackSpeed())));
            for (int h = 0; h < hits && player.alive(); ++h) {
                int dmg = e.attack(player, rng);
                player.hp -= dmg;
                std::cout << e.name << " hits you for " << dmg
                          << " (You: " << std::max(0, player.hp) << "/" << player.maxHP << ")\n";
            }
        }

        std::cout << "\n";
    }

    if (player.alive()) {
        std::cout << "Victory! You survived with " << player.hp << " HP.\n";
    } else {
        std::cout << "Defeat. You died.\n";
    }
}

} // namespace game
