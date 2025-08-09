#include "game/actor.hpp"
#include <algorithm>

namespace game {

int Actor::attack(Actor& target, core::RNG& rng) const {
    int dmg = weapon.rollDamage(rng);
    dmg = std::max(0, dmg - target.armor);
    return dmg;
}

} // namespace game
