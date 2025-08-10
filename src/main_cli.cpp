#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "core/rng.hpp"
#include "game/rarity.hpp"
#include "game/weapon.hpp"
#include "game/weapon_base.hpp"
#include "game/affix.hpp"
#include "game/actor.hpp"
#include "game/inventory.hpp"
#include "game/loot_tables.hpp"
#include "game/combat_math.hpp"

using namespace game;

static void print_help() {
    std::cout <<
    "Commands:\n"
    "  h / help              - show this help\n"
    "  p / player            - show player stats\n"
    "  e / enemies           - list enemies\n"
    "  t / target <idx>      - select enemy index to focus (see 'enemies')\n"
    "  n / next              - run next round (you then enemies)\n"
    "  i / inventory         - list inventory items with DPR\n"
    "  q / equip <idx>       - equip item by index\n"
    "  b / best              - equip best-by-DPR item\n"
    "  a / auto              - toggle auto-equip-on-drop\n"
    "  r / reset             - reset battle (keeps inventory)\n"
    "  x / exit              - quit\n";
}

static void print_player(const Actor& player) {
    std::cout << "Player HP: " << player.hp << "/" << player.maxHP
              << "  Armor: " << player.armor << "\n"
              << "Equipped: " << player.weapon.label()
              << "  | DPR: " << expectedDPR(player.weapon) << "\n";
}

static void print_enemies(const std::vector<Actor>& enemies, int selected) {
    std::cout << "Enemies:\n";
    for (size_t i = 0; i < enemies.size(); ++i) {
        const auto& en = enemies[i];
        std::cout << "  [" << i << "] " << en.name
                  << (static_cast<int>(i) == selected ? "  <target>" : "")
                  << "  HP " << std::max(0, en.hp) << "/" << en.maxHP
                  << "  Armor " << en.armor
                  << (en.alive() ? "" : "  (dead)")
                  << "\n";
    }
}

static void print_inventory(const Inventory& inv) {
    std::cout << "Inventory (" << inv.size() << " items):\n";
    for (size_t i = 0; i < inv.size(); ++i) {
        const auto& w = inv.at(i);
        const bool eq = (inv.equippedIndex() == i);
        std::cout << "  [" << (i < 10 ? "0" : "") << i << "] "
                  << (eq ? "* " : "  ")
                  << w.label()
                  << "  | DPR: " << expectedDPR(w) << "\n";
    }
}

static std::vector<Actor> make_enemies() {
    Weapon goblinW{ WeaponBase{"Shiv",   1, 4}, Rarity::Common, {} };
    Weapon bruteW { WeaponBase{"Club",   3, 7}, Rarity::Common, {} };
    Weapon raiderW{ WeaponBase{"Hatchet",2, 6}, Rarity::Common, {} };
    return {
        Actor{"Goblin", 20, 20, 0, goblinW},
        Actor{"Brute",  35, 35, 1, bruteW },
        Actor{"Raider", 25, 25, 0, raiderW}
    };
}

int main() {
    // ---- Game state
    core::RNG rng(1337);
    LootTables loot = makeDefaultLoot();

    Inventory inv;
    size_t starterIdx = inv.add(Weapon{ WeaponBase{"Rusty Sword", 2, 6}, Rarity::Common, {} });
    inv.equip(starterIdx);

    Actor player{ "Player", 60, 60, 1, *inv.equipped() };
    std::vector<Actor> enemies = make_enemies();
    int selectedEnemy = 0;
    bool autoEquipBetter = true;

    auto any_alive = [&](){ for (auto& e: enemies) if (e.alive()) return true; return false; };

    auto reset_battle = [&](){
        player = Actor{ "Player", 60, 60, 1, *inv.equipped() };
        enemies = make_enemies();
        selectedEnemy = 0;
        std::cout << "Battle reset.\n";
    };

    auto do_player_turn = [&](){
        // Choose target: preferred selectedEnemy if alive; else first alive.
        auto it = enemies.end();
        if (selectedEnemy >= 0 && selectedEnemy < static_cast<int>(enemies.size()) && enemies[selectedEnemy].alive()) {
            it = enemies.begin() + selectedEnemy;
        } else {
            it = std::find_if(enemies.begin(), enemies.end(), [](const Actor& e){ return e.alive(); });
        }
        if (it == enemies.end()) return;

        Actor& target = *it;
        int hits = std::max(1, static_cast<int>(std::round(player.weapon.attackSpeed())));
        for (int h = 0; h < hits && target.alive(); ++h) {
            int dmg = player.attack(target, rng);
            target.hp -= dmg;
            std::cout << "You hit " << target.name << " for " << dmg
                      << "  (" << std::max(0, target.hp) << "/" << target.maxHP << ")\n";
        }
        if (!target.alive()) {
            std::cout << target.name << " is slain!\n";
            Weapon drop = loot.rollWeapon(rng, 1);
            std::cout << "Loot dropped: " << drop.label() << "\n";
            size_t idx = inv.add(drop);

            if (autoEquipBetter) {
                double cur = expectedDPR(player.weapon);
                double cand = expectedDPR(inv.at(idx));
                if (cand > cur) {
                    inv.equip(idx);
                    player.weapon = *inv.equipped();
                    std::cout << "Auto-equipped better weapon (" << cand << " DPR > " << cur << " DPR).\n";
                }
            }
        }
    };

    auto do_enemies_turn = [&](){
        for (auto& e : enemies) {
            if (!e.alive() || !player.alive()) continue;
            int hits = std::max(1, static_cast<int>(std::round(e.weapon.attackSpeed())));
            for (int h=0; h<hits && player.alive(); ++h) {
                int dmg = e.attack(player, rng);
                player.hp -= dmg;
                std::cout << e.name << " hits you for " << dmg
                          << "  (You: " << std::max(0, player.hp) << "/" << player.maxHP << ")\n";
            }
        }
    };

    // ---- Intro & help
    std::cout << "Castle-like Combat (Console Prototype)\n";
    print_help();
    print_player(player);
    print_enemies(enemies, selectedEnemy);

    // ---- Input loop
    std::string line;
    while (true) {
        std::cout << "\n> ";
        if (!std::getline(std::cin, line)) break;
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        for (auto& c : cmd) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (cmd == "h" || cmd == "help") {
            print_help();

        } else if (cmd == "p" || cmd == "player") {
            print_player(player);

        } else if (cmd == "e" || cmd == "enemies") {
            print_enemies(enemies, selectedEnemy);

        } else if (cmd == "t" || cmd == "target") {
            int idx = -1;
            if (!(iss >> idx)) {
                std::cout << "Usage: target <index>\n";
            } else if (idx < 0 || idx >= static_cast<int>(enemies.size())) {
                std::cout << "Invalid index. Use 'enemies' to list.\n";
            } else {
                selectedEnemy = idx;
                std::cout << "Target set to [" << idx << "] " << enemies[idx].name << ".\n";
            }

        } else if (cmd == "n" || cmd == "next") {
            if (!player.alive()) { std::cout << "You are dead. Use 'reset'.\n"; continue; }
            if (!any_alive())    { std::cout << "No enemies alive. Use 'reset'.\n"; continue; }
            do_player_turn();
            do_enemies_turn();
            if (!player.alive()) std::cout << "Defeat. You died.\n";
            else if (!any_alive()) std::cout << "Victory! All enemies defeated.\n";

        } else if (cmd == "i" || cmd == "inventory") {
            print_inventory(inv);

        } else if (cmd == "q" || cmd == "equip") {
            int idx = -1;
            if (!(iss >> idx)) {
                std::cout << "Usage: equip <index>\n";
            } else if (idx < 0 || idx >= static_cast<int>(inv.size())) {
                std::cout << "Invalid index. Use 'inventory' to list.\n";
            } else {
                inv.equip(static_cast<size_t>(idx));
                player.weapon = *inv.equipped();
                std::cout << "Equipped: " << inv.at(static_cast<size_t>(idx)).label() << "\n";
            }

        } else if (cmd == "b" || cmd == "best") {
            if (inv.equipBest()) { player.weapon = *inv.equipped(); std::cout << "Equipped best-by-DPR.\n"; }
            else std::cout << "Inventory is empty.\n";

        } else if (cmd == "a" || cmd == "auto") {
            autoEquipBetter = !autoEquipBetter;
            std::cout << "Auto-equip on drop: " << (autoEquipBetter ? "ON" : "OFF") << "\n";

        } else if (cmd == "r" || cmd == "reset") {
            reset_battle();

        } else if (cmd == "x" || cmd == "exit") {
            break;

        } else if (cmd.empty()) {
            // ignore

        } else {
            std::cout << "Unknown command. Type 'help'.\n";
        }
    }

    return 0;
}
