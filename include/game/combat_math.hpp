#pragma once
#include <algorithm>
#include <cmath>
#include "game/item.hpp"
#include "core/rng.hpp"

namespace game {

struct GearBonuses {
    int    armor = 0;
    double pctDamage = 0.0;
    double critChance = 0.0;
    double attackSpeed = 0.0;
};

inline double expectedDamagePerSwing(const Item& w, double extraPct=0.0, double extraCrit=0.0) {
    const double avg    = (w.minDmg() + w.maxDmg()) / 2.0;
    const double scaled = avg * (1.0 + w.pctDamage() + extraPct);
    const double critC  = std::clamp(w.critChance() + extraCrit, 0.0, 0.95);
    return scaled * (1.0 + critC * (w.critMult() - 1.0));
}

inline double expectedAPS(const Item& w, double extraAS=0.0) {
    return std::max(0.2, 1.0 + w.attackSpeed() + extraAS);
}

inline double expectedDPR(const Item& w, double extraPct=0.0, double extraCrit=0.0, double extraAS=0.0) {
    return expectedDamagePerSwing(w, extraPct, extraCrit) * expectedAPS(w, extraAS);
}

inline int rollDamageWithBonuses(const Item& w, core::RNG& rng, double extraPct=0.0, double extraCrit=0.0) {
    int baseRoll   = rng.i(w.minDmg(), w.maxDmg());
    double scaled  = baseRoll * (1.0 + w.pctDamage() + extraPct);
    double critC   = std::clamp(w.critChance() + extraCrit, 0.0, 0.95);
    if (rng.chance(critC)) scaled *= w.critMult();
    return std::max(0, static_cast<int>(std::round(scaled)));
}

} // namespace game