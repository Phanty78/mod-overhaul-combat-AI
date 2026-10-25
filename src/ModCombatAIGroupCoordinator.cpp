#include "ModCombatAIGroupCoordinator.h"
#include "Timer.h"

namespace
{
    // Minimum spacing between two full sweeps of the group map.
    constexpr uint32 GroupPruneIntervalMs = 60000;
}

ModCombatAIGroupCoordinator g_ModCombatAIGroupCoordinator;

ModCombatAIGroupCoordinator& ModCombatAIGroupCoordinator::Instance()
{
    static ModCombatAIGroupCoordinator instance;
    return instance;
}

bool ModCombatAIGroupCoordinator::CanCallForHelp(Creature* creature, uint32 now) const
{
    if (!creature)
        return true;

    auto it = _groups.find(MakeModCombatAIGroupKey(creature));
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

    _groups[MakeModCombatAIGroupKey(creature)].cooldownEnd = now + cooldownMs;
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
