#pragma once
#include <string>
namespace game {
enum class Rarity { Common, Magic, Rare, Epic, Legendary };
inline std::string rarityName(Rarity r) {
    switch(r){
        case Rarity::Common: return "Common";
        case Rarity::Magic: return "Magic";
        case Rarity::Rare: return "Rare";
        case Rarity::Epic: return "Epic";
        case Rarity::Legendary: return "Legendary";
    } return "Common";
}
} // namespace game