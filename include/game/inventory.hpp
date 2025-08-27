#pragma once
#include <vector>
#include <cstddef>
#include "game/item.hpp"
#include "game/slots.hpp"
#include "game/combat_math.hpp"

namespace game {

class Inventory {
public:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    // Storage / add
    std::size_t addWeapon(Item w); // requires kind==Weapon
    std::size_t addGear(Item g);   // requires kind==Gear

    // Equip (weapons)
    bool equip(std::size_t idx);            // main-hand
    bool equipOffhand(std::size_t idx);     // off-hand weapon (disables shield)

    // Equip (gear)
    bool equipGear(std::size_t gearIdx);    // slot-aware; rings fill Ring1 then Ring2

    // Queries
    const Item* equipped() const;               // main-hand weapon
    const Item* equippedOffhand() const;        // off-hand weapon
    const Item* equipped(Slot slot) const;      // specific gear slot

    // Helpers
    game::GearBonuses bonuses() const;
    bool equipBest(); // best-by-DPR considering gear bonuses

    // Access
    std::size_t weaponsCount() const { return weapons_.size(); }
    std::size_t gearCount() const    { return gear_.size(); }

    const Item& weaponAt(std::size_t i) const { return weapons_.at(i); }
    Item&       weaponAt(std::size_t i)       { return weapons_.at(i); }

    const Item& gearAt(std::size_t i)   const { return gear_.at(i); }
    Item&       gearAt(std::size_t i)         { return gear_.at(i); }

    struct Equipped {
        std::size_t mainHand     = npos;
        std::size_t offHandWpn   = npos;
        std::size_t offHandShield= npos;
        std::size_t armor  = npos, helmet=npos, boots=npos, belt=npos, amulet=npos, ring1=npos, ring2=npos;
    } eq_;

private:
    std::vector<Item> weapons_; // kind==Weapon only
    std::vector<Item> gear_;    // kind==Gear only
};

} // namespace game