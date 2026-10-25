#include "ModCombatAIGroupCoordinator.h"
#include "Creature.h"
#include "Timer.h"

#include <cmath>
#include <functional>

namespace
{
    // Side length of the square world cell that acts as a "group" for cooldown purposes.
    constexpr float GroupCellSize = 40.0f;
    // Minimum spacing between two full sweeps of the group map.
    constexpr uint32 GroupPruneIntervalMs = 60000;
    // Mixing constant from the golden ratio, as used by boost::hash_combine.
    constexpr std::size_t HashMixConstant = 0x9e3779b9;
}

ModCombatAIGroupCoordinator g_ModCombatAIGroupCoordinator;

ModCombatAIGroupCoordinator& ModCombatAIGroupCoordinator::Instance()
{
    static ModCombatAIGroupCoordinator instance;
    return instance;
}

ModCombatAIGroupCoordinator::GroupKey ModCombatAIGroupCoordinator::MakeKey(Creature* creature)
{
    GroupKey key;
    key.mapId = creature->GetMapId();
    key.cellX = int32(std::floor(creature->GetPositionX() / GroupCellSize));
    key.cellY = int32(std::floor(creature->GetPositionY() / GroupCellSize));
    return key;
}

std::size_t ModCombatAIGroupCoordinator::GroupKeyHash::operator()(GroupKey const& key) const
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

bool ModCombatAIGroupCoordinator::CanCallForHelp(Creature* creature, uint32 now) const
{
    if (!creature)
        return true;

    auto it = _groups.find(MakeKey(creature));
    if (it == _groups.end())
        return true;

    // Wrap-safe as long as the cooldown stays well below the 24.8 day uint32 range.
    int32 const remaining = int32(it->second.cooldownEnd - now);
    return remaining <= 0;
}

void ModCombatAIGroupCoordinator::OnCallForHelp(Creature* creature, uint32 now, uint32 cooldownMs)
{
    if (!creature)
        return;

    _groups[MakeKey(creature)].cooldownEnd = now + cooldownMs;
}

void ModCombatAIGroupCoordinator::Prune(uint32 now, uint32 /*cooldownMs*/)
{
    if (getMSTimeDiff(_lastPruneMs, now) < GroupPruneIntervalMs)
        return;

    _lastPruneMs = now;

    for (auto it = _groups.begin(); it != _groups.end();)
    {
        if (int32(it->second.cooldownEnd - now) <= 0)
            it = _groups.erase(it);
        else
            ++it;
    }
}

void ModCombatAIGroupCoordinator::Reset()
{
    _groups.clear();
    _lastPruneMs = 0;
}
