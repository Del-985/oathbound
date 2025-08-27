#pragma once
#include <string>

namespace game {

struct Affix {
    std::string name;
    int    flatMin      = 0;
    int    flatMax      = 0;
    double pctDamage    = 0.0; // +0.15 = +15%
    double critChance   = 0.0; // +0.05 = +5%
    double attackSpeed  = 0.0; // +0.10 = +10%

    static Affix Prefix(std::string n, int fmin=0,int fmax=0,double pd=0,double cc=0,double as=0){
        return Affix{std::move(n), fmin, fmax, pd, cc, as};
    }
    static Affix Suffix(std::string n, int fmin=0,int fmax=0,double pd=0,double cc=0,double as=0){
        return Affix{std::move(n), fmin, fmax, pd, cc, as};
    }
};

} // namespace game