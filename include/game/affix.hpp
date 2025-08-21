#pragma once
#include <string>

namespace game {

struct Affix {
    std::string name;
    int    flatMin      = 0;   // adds to min damage
    int    flatMax      = 0;   // adds to max damage
    double pctDamage    = 0.0; // +0.15 = +15% damage
    double critChance   = 0.0; // +0.05 = +5% crit chance
    double attackSpeed  = 0.0; // +0.10 = +10% attacks/round

    static Affix Prefix(std::string n, int fmin=0,int fmax=0,double pd=0,double cc=0,double as=0){
        return Affix{std::move(n), fmin, fmax, pd, cc, as};
    }
    static Affix Suffix(std::string n, int fmin=0,int fmax=0,double pd=0,double cc=0,double as=0){
        return Affix{std::move(n), fmin, fmax, pd, cc, as};
    }
};

} // namespace game