#pragma once
#include <string>
#include <vector>
#include <sstream>
#include "game/rarity.hpp"
#include "game/affix.hpp"
#include "game/slots.hpp"

namespace game {

struct Gear {
    std::string name;
    Slot        slot;
    Rarity      rarity = Rarity::Common;
    int         armorBonus = 0;       // flat armor from this piece
    std::vector<Affix> affixes;       // reuse same affix math as weapons

    double pctDamage() const {
        double p = 0.0; for (auto& a: affixes) p += a.pctDamage; return p;
    }
    double critChance() const {
        double c = 0.0; for (auto& a: affixes) c += a.critChance; return c;
    }
    double attackSpeed() const {
        double s = 0.0; for (auto& a: affixes) s += a.attackSpeed; return s;
    }

    std::string label() const {
        std::ostringstream os;
        os << "[" << rarityName(rarity) << "] " << name;
        if (armorBonus) os << " (Armor +" << armorBonus << ")";
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
