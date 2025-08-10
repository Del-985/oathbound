#pragma once
#include <algorithm>
#include "game/weapon.hpp"

namespace game {

// Expected damage per swing = avg roll * (1+pctDamage) * crit factor
inline double expectedDamagePerSwing(const Weapon& w) {
    const double avg = (w.minDmg() + w.maxDmg()) / 2.0;
    const double scaled = avg * (1.0 + w.pctDamage());
    const double critFactor = 1.0 + w.critChance() * (w.critMult() - 1.0);
    return scaled * critFactor;
}

// Attacks per round (already clamped in Weapon::attackSpeed)
inline double expectedAPS(const Weapon& w) {
    return std::max(0.2, w.attackSpeed());
}

// “DPS” here = expected damage per round (EDPR)
inline double expectedDPR(const Weapon& w) {
    return expectedDamagePerSwing(w) * expectedAPS(w);
}

} // namespace game
