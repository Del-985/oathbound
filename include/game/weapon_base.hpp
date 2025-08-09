#pragma once
#include <string>

namespace game {

struct WeaponBase {
    std::string name;
    int baseMin = 1;
    int baseMax = 1;
};

} // namespace game
