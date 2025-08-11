#include "game/actor.hpp"
#include "game/combat_math.hpp"
#include <algorithm>

namespace game {

int Actor::attack(Actor& target, core::RNG& rng, double extraPct, double extraCrit) const {
    int dmg = rollDamageWithBonuses(weapon, rng, extraPct, extraCrit);
    dmg = std::max(0, dmg - target.armor);
    return dmg;
}

} // namespace game
