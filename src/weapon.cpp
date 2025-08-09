#include "game/weapon.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace game {

int Weapon::minDmg() const {
    int m = base.baseMin;
    for (const auto& a : affixes) m += a.flatMin;
    return std::max(1, m);
}

int Weapon::maxDmg() const {
    int M = base.baseMax;
    for (const auto& a : affixes) M += a.flatMax;
    return std::max(minDmg(), M);
}

double Weapon::pctDamage() const {
    double p = 0.0;
    for (const auto& a : affixes) p += a.pctDamage;
    return p;
}

double Weapon::critChance() const {
    double c = 0.05; // baseline 5%
    for (const auto& a : affixes) c += a.critChance;
    return std::clamp(c, 0.0, 0.75);
}

double Weapon::critMult() const {
    return 1.5;
}

double Weapon::attackSpeed() const {
    double a = 1.0; // baseline 1 attack per round
    for (const auto& af : affixes) a += af.attackSpeed;
    return std::max(0.2, a);
}

int Weapon::rollDamage(core::RNG& rng) const {
    int baseRoll = rng.i(minDmg(), maxDmg());
    double scaled = baseRoll * (1.0 + pctDamage());
    bool crit = rng.chance(critChance());
    if (crit) scaled *= critMult();
    return std::max(0, static_cast<int>(std::round(scaled)));
}

std::string Weapon::label() const {
    std::ostringstream os;
    os << "[" << rarityName(rarity) << "] " << base.name;
    if (!affixes.empty()) {
        os << " (";
        for (size_t i = 0; i < affixes.size(); ++i) {
            os << affixes[i].name;
            if (i + 1 < affixes.size()) os << ", ";
        }
        os << ")";
    }
    os << " {" << minDmg() << "-" << maxDmg() << "}";
    return os.str();
}

} // namespace game
