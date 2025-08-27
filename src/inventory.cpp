#include "game/inventory.hpp"
#include <limits>
#include <algorithm>

namespace game {

std::size_t Inventory::addWeapon(Item w) {
    if (!w.isWeapon()) return npos;
    weapons_.push_back(std::move(w));
    return weapons_.size() - 1;
}

std::size_t Inventory::addGear(Item g) {
    if (g.isWeapon()) return npos;
    gear_.push_back(std::move(g));
    return gear_.size() - 1;
}

bool Inventory::equip(std::size_t idx) {
    if (idx >= weapons_.size()) return false;
    eq_.mainHand = idx;
    if (weapons_[idx].twoHanded) { // occupy both hands
        eq_.offHandWpn    = npos;
        eq_.offHandShield = npos;
    }
    return true;
}

bool Inventory::equipOffhand(std::size_t idx) {
    if (idx >= weapons_.size()) return false;
    if (weapons_[idx].twoHanded) return false; // can't put a 2H in off-hand
    eq_.offHandWpn    = idx;
    eq_.offHandShield = npos;
    return true;
}

const Item* Inventory::equipped() const {
    if (eq_.mainHand == npos || eq_.mainHand >= weapons_.size()) return nullptr;
    return &weapons_[eq_.mainHand];
}

const Item* Inventory::equippedOffhand() const {
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
        case Slot::Offhand: ok = false; return; // handled via offHandShield
        case Slot::Weapon:  ok = false; return; // use equip()
    }
    ok = true;
}

bool Inventory::equipGear(std::size_t gearIdx) {
    if (gearIdx >= gear_.size()) return false;
    const Item& g = gear_[gearIdx];

    if (g.slot == Slot::Offhand) { // shield / off-hand gear
        eq_.offHandShield = gearIdx;
        eq_.offHandWpn    = npos;
        return true;
    }

    if (g.slot == Slot::Ring1 || g.slot == Slot::Ring2) {
        if (eq_.ring1 == npos) { eq_.ring1 = gearIdx; return true; }
        if (eq_.ring2 == npos) { eq_.ring2 = gearIdx; return true; }
        eq_.ring1 = gearIdx; // replace Ring1 by convention
        return true;
    }

    bool ok=false; setSlotIndex(eq_, g.slot, gearIdx, ok);
    return ok;
}

const Item* Inventory::equipped(Slot slot) const {
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

game::GearBonuses Inventory::bonuses() const {
    game::GearBonuses b{0,0.0,0.0,0.0};
    auto addFrom = [&](const Item* g){
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
    addFrom(equipped(Slot::Offhand));
    return b;
}

bool Inventory::equipBest() {
    if (weapons_.empty()) return false;
    game::GearBonuses b = bonuses();
    double best = -std::numeric_limits<double>::infinity();
    std::size_t bestIdx = 0;
    for (std::size_t i=0;i<weapons_.size();++i) {
        const double dpr = expectedDPR(weapons_[i], b.pctDamage, b.critChance, b.attackSpeed);
        if (dpr > best) { best = dpr; bestIdx = i; }
    }
    return equip(bestIdx);
}

} // namespace game