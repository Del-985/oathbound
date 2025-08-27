#pragma once

namespace game {

enum class Slot { Weapon, Offhand, Armor, Helmet, Boots, Belt, Amulet, Ring1, Ring2 };

inline const char* slotName(Slot s) {
    switch (s) {
        case Slot::Weapon:  return "Weapon";
        case Slot::Offhand: return "Off-hand";
        case Slot::Armor:   return "Armor";
        case Slot::Helmet:  return "Helmet";
        case Slot::Boots:   return "Boots";
        case Slot::Belt:    return "Belt";
        case Slot::Amulet:  return "Amulet";
        case Slot::Ring1:   return "Ring 1";
        case Slot::Ring2:   return "Ring 2";
    }
    return "?";
}

} // namespace game