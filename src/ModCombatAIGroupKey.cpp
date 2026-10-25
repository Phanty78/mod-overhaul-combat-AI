#include "ModCombatAIGroupKey.h"
#include "Creature.h"

#include <cmath>
#include <functional>

namespace
{
    // Side length of the square world cell that acts as a "group" for cooldown purposes.
    constexpr float GroupCellSize = 40.0f;
    // Mixing constant from the golden ratio, as used by boost::hash_combine.
    constexpr std::size_t HashMixConstant = 0x9e3779b9;
}

std::size_t ModCombatAIGroupKeyHash::operator()(ModCombatAIGroupKey const& key) const
{
    std::size_t seed = std::hash<uint32>{}(key.mapId);

    auto combine = [&seed](int32 value)
    {
        seed ^= std::hash<uint32>{}(uint32(value)) + HashMixConstant + (seed << 6) + (seed >> 2);
    };

    combine(key.cellX);
    combine(key.cellY);

    return seed;
}

ModCombatAIGroupKey MakeModCombatAIGroupKey(Creature* creature)
{
    ModCombatAIGroupKey key;
    key.mapId = creature->GetMapId();
    key.cellX = int32(std::floor(creature->GetPositionX() / GroupCellSize));
    key.cellY = int32(std::floor(creature->GetPositionY() / GroupCellSize));
    return key;
}
