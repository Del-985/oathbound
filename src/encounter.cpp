#include "game/encounter.hpp"
#include "game/combat_math.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace game {

void Encounter::run() {
    // Ensure actor weapon matches inventory at start if equipped
    if (const Weapon* eq = inventory.equipped()) {
        player.weapon = *eq;
    }
    std::cout << "You wield " << player.weapon.label() << "\n\n";

    int round = 1;
    auto anyAlive = [&]{ for (const auto& e : enemies) if (e.alive()) return true; return false; };

    while (player.alive() && anyAlive()) {
        std::cout << "=== Round " << round++ << " ===\n";

        // Player turn
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

                // Drop → add to inventory
                Weapon drop = loots.rollWeapon(rng, /*level*/1);
                std::cout << "Loot dropped: " << drop.label() << "\n";
                std::size_t idx = inventory.add(std::move(drop));

                // Compare DPR and auto-equip if better
                const double cur = expectedDPR(player.weapon);
                const double cand = expectedDPR(inventory.at(idx));
                if (cand > cur) {
                    inventory.equip(idx);
                    player.weapon = *inventory.equipped(); // sync
                    std::cout << "Auto-equipped better weapon ("
                              << cand << " DPR > " << cur << " DPR).\n";
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

    std::cout << (player.alive() ? "Victory! You survived with " + std::to_string(player.hp) + " HP.\n"
                                 : "Defeat. You died.\n");
}

} // namespace game
