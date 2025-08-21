#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include "game/rarity.hpp"
#include "game/affix.hpp"
#include "game/slots.hpp"

namespace game {

enum class ItemKind { Weapon, Gear };

struct Item {
    // Identity
    std::string name;
    Rarity      rarity = Rarity::Common;

    // Kind + placement
    ItemKind    kind   = ItemKind::Gear;   // Weapon or Gear
    Slot        slot   = Slot::Armor;      // Weapon uses Slot::Weapon; shield uses Slot::Offhand

    // Weapon stats (if kind==Weapon)
    int baseMin = 0;
    int baseMax = 0;

    // Gear stats (if kind==Gear)
    int armorBonus = 0;

    // Modifiers
    std::vector<Affix> affixes;

    // Helpers
    bool isWeapon() const { return kind == ItemKind::Weapon; }
    bool isShield() const { return kind == ItemKind::Gear && slot == Slot::Offhand && armorBonus > 0; }

    int minDmg() const {
        if (!isWeapon()) return 0;
        int m = baseMin;
        for (auto& a: affixes) m += a.flatMin;
        return std::max(1, m);
    }
    int maxDmg() const {
        if (!isWeapon()) return 0;
        int M = baseMax;
        for (auto& a: affixes) M += a.flatMax;
        return std::max(minDmg(), M);
    }
    double pctDamage() const { double p=0; for (auto& a: affixes) p += a.pctDamage; return p; }
    double critChance() const { double c=0; for (auto& a: affixes) c += a.critChance; return std::clamp(c,0.0,0.95); }
    double attackSpeed() const { double s=0; for (auto& a: affixes) s += a.attackSpeed; return s; }
    double critMult() const { return 1.5; }

    std::string label() const {
        std::ostringstream os;
        os << "[" << rarityName(rarity) << "] " << name;
        if (isWeapon()) {
            os << " {" << minDmg() << "-" << maxDmg() << "}";
        } else if (armorBonus) {
            os << " (Armor +" << armorBonus << ")";
        }
        if (!affixes.empty()) {
            os << " (";
            for (size_t i=0;i<affixes.size();++i) {
                os << affixes[i].name;
                if (i+1<affixes.size()) os << ", ";
            }
            os << ")";
        }
        return os.str();
    }
};

} // namespace game