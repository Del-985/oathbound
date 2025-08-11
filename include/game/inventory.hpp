#pragma once
#include <vector>
#include <cstddef>
#include "game/weapon.hpp"
#include "game/gear.hpp"
#include "game/slots.hpp"
#include "game/combat_math.hpp"

namespace game {

class Inventory {
public:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    // Weapons
    std::size_t add(Weapon w);              // add weapon (kept for backward compat)
    std::size_t addWeapon(Weapon w) { return add(std::move(w)); }
    bool equip(std::size_t idx);            // equip main-hand weapon
    bool equipOffhand(std::size_t idx);     // equip off-hand weapon
    const Weapon* equipped() const;         // main-hand
    const Weapon* equippedOffhand() const;  // off-hand

    // Gear
    std::size_t add(Gear g);
    bool equipGear(std::size_t gearIdx);        // equips to its natural slot; rings fill Ring1 then Ring2
    const Gear* equipped(Slot slot) const;

    // Helpers
    bool equipBest(); // best-by-DPR considering current gear bonuses

    // Access
    std::size_t weaponsCount() const { return weapons_.size(); }
    std::size_t gearCount() const    { return gear_.size(); }
    const Weapon& weaponAt(std::size_t i) const { return weapons_.at(i); }
    Weapon&       weaponAt(std::size_t i)       { return weapons_.at(i); }
    const Gear&   gearAt(std::size_t i)   const { return gear_.at(i); }
    Gear&         gearAt(std::size_t i)         { return gear_.at(i); }

    // Aggregate bonuses from equipped gear (not counting weapons)
    GearBonuses bonuses() const;

    // Equipped indices
    struct Equipped {
        std::size_t mainHand   = npos;  // index into weapons_
        std::size_t offHandWpn = npos;  // index into weapons_ (if using a weapon)
        std::size_t offHandShield = npos; // index into gear_ (if using a shield); mutually exclusive with offHandWpn
        std::size_t armor   = npos;
        std::size_t helmet  = npos;
        std::size_t boots   = npos;
        std::size_t belt    = npos;
        std::size_t amulet  = npos;
        std::size_t ring1   = npos;
        std::size_t ring2   = npos;
    } eq_;

private:
    std::vector<Weapon> weapons_;
    std::vector<Gear>   gear_;
};

} // namespace game
