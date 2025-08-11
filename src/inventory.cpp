#include "game/inventory.hpp"
#include "game/combat_math.hpp"
#include <limits>
#include <algorithm>

namespace game {

std::size_t Inventory::add(Weapon w) {
    weapons_.push_back(std::move(w));
    return weapons_.size() - 1;
}

std::size_t Inventory::add(Gear g) {
    gear_.push_back(std::move(g));
    return gear_.size() - 1;
}

bool Inventory::equip(std::size_t idx) {
    if (idx >= weapons_.size()) return false;
    eq_.mainHand = idx;
    return true;
}

bool Inventory::equipOffhand(std::size_t idx) {
    if (idx >= weapons_.size()) return false;
    eq_.offHandWpn = idx;
    eq_.offHandShield = npos; // cannot use shield at same time
    return true;
}

const Weapon* Inventory::equipped() const {
    if (eq_.mainHand == npos || eq_.mainHand >= weapons_.size()) return nullptr;
    return &weapons_[eq_.mainHand];
}
const Weapon* Inventory::equippedOffhand() const {
    if (eq_.offHandWpn == npos || eq_.offHandWpn >= weapons_.size()) return nullptr;
    return &weapons_[eq_.offHandWpn];
}

static void setSlotIndex(Inventory::Equipped& eq, Slot slot, std::size_t idx, bool& ok) {
    switch (slot) {
        case Slot::Armor:  eq.armor  = idx; break;
        case Slot::Helmet: eq.helmet = idx; break;
        case Slot::Boots:  eq.boots  = idx; break;
        case Slot::Belt:   eq.belt   = idx; break;
        case Slot::Amulet: eq.amulet = idx; break;
        case Slot::Ring1:  eq.ring1  = idx; break;
        case Slot::Ring2:  eq.ring2  = idx; break;
        case Slot::Offhand: ok = false; return; // handled via eq_.offHandShield below
        case Slot::Weapon:  ok = false; return; // use equip()
    }
    ok = true;
}

bool Inventory::equipGear(std::size_t gearIdx) {
    if (gearIdx >= gear_.size()) return false;
    const Gear& g = gear_[gearIdx];
    if (g.slot == Slot::Offhand) {
        eq_.offHandShield = gearIdx;
        eq_.offHandWpn = npos;
        return true;
    }
    if (g.slot == Slot::Ring1 || g.slot == Slot::Ring2) {
        // Allow any Ring item to occupy Ring1 or Ring2: fill empty first
        if (eq_.ring1 == npos) { eq_.ring1 = gearIdx; return true; }
        if (eq_.ring2 == npos) { eq_.ring2 = gearIdx; return true; }
        // replace Ring1 by default
        eq_.ring1 = gearIdx;
        return true;
    }
    bool ok=false; setSlotIndex(eq_, g.slot, gearIdx, ok);
    return ok;
}

const Gear* Inventory::equipped(Slot slot) const {
    std::size_t idx = npos;
    switch (slot) {
        case Slot::Armor:   idx = eq_.armor; break;
        case Slot::Helmet:  idx = eq_.helmet; break;
        case Slot::Boots:   idx = eq_.boots; break;
        case Slot::Belt:    idx = eq_.belt; break;
        case Slot::Amulet:  idx = eq_.amulet; break;
        case Slot::Ring1:   idx = eq_.ring1; break;
        case Slot::Ring2:   idx = eq_.ring2; break;
        case Slot::Offhand: idx = eq_.offHandShield; break;
        case Slot::Weapon:  return nullptr;
    }
    if (idx == npos || idx >= gear_.size()) return nullptr;
    return &gear_[idx];
}

GearBonuses Inventory::bonuses() const {
    GearBonuses b;
    auto addFrom = [&](const Gear* g){
        if (!g) return;
        b.armor       += g->armorBonus;
        b.pctDamage   += g->pctDamage();
        b.critChance  += g->critChance();
        b.attackSpeed += g->attackSpeed();
    };
    addFrom(equipped(Slot::Armor));
    addFrom(equipped(Slot::Helmet));
    addFrom(equipped(Slot::Boots));
    addFrom(equipped(Slot::Belt));
    addFrom(equipped(Slot::Amulet));
    addFrom(equipped(Slot::Ring1));
    addFrom(equipped(Slot::Ring2));
    addFrom(equipped(Slot::Offhand)); // if shield in offhand; if weapon in offhand, not added here
    return b;
}

bool Inventory::equipBest() {
    if (weapons_.empty()) return false;
    GearBonuses b = bonuses();
    double best = -std::numeric_limits<double>::infinity();
    std::size_t bestIdx = 0;
    for (std::size_t i=0;i<weapons_.size();++i) {
        const double dpr = expectedDPR(weapons_[i], b.pctDamage, b.critChance, b.attackSpeed);
        if (dpr > best) { best = dpr; bestIdx = i; }
    }
    return equip(bestIdx);
}

} // namespace game
