#include "ModCombatAICoordinator.h"
#include "Creature.h"
#include "Timer.h"

namespace
{
    // Length of the sliding window during which cooperative actions are counted.
    constexpr uint32 CooperativeWindowMs = 5000;
    // Minimum spacing between two full sweeps of the group map.
    constexpr uint32 PruneIntervalMs = 60000;
    // A group whose window started longer ago than this is forgotten.
    constexpr uint32 StateTtlMs = 10000;
}

ModCombatAICoordinator g_ModCombatAICoordinator;

ModCombatAICoordinator& ModCombatAICoordinator::Instance()
{
    static ModCombatAICoordinator instance;
    return instance;
}

bool ModCombatAICoordinator::CanAct(Creature* creature, ModCombatAITactic tactic, uint32 now, float groupIntelligence) const
{
    switch (tactic)
    {
        case ModCombatAITactic::ProtectAlly:
        case ModCombatAITactic::CallForHelp:
            break;
        case ModCombatAITactic::Retreat:
        case ModCombatAITactic::Reposition:
        default:
            return true;
    }

    if (!creature)
        return true;

    auto it = _groups.find(MakeModCombatAIGroupKey(creature));
    if (it == _groups.end())
        return true;

    // The window elapsing clears the effective budget without touching the map.
    if (getMSTimeDiff(it->second.windowStart, now) >= CooperativeWindowMs)
        return true;

    // Group intelligence scales the number of cooperative actions a group may
    // perform inside a single window.
    uint32 const maxActions = 1 + uint32(groupIntelligence * 2.0f);
    return it->second.cooperativeActions < maxActions;
}

void ModCombatAICoordinator::OnAction(Creature* creature, ModCombatAITactic tactic, uint32 now)
{
    switch (tactic)
    {
        case ModCombatAITactic::ProtectAlly:
        case ModCombatAITactic::CallForHelp:
            break;
        case ModCombatAITactic::Retreat:
        case ModCombatAITactic::Reposition:
        default:
            return;
    }

    if (!creature)
        return;

    GroupState& state = _groups[MakeModCombatAIGroupKey(creature)];
    if (state.windowStart == 0 || getMSTimeDiff(state.windowStart, now) >= CooperativeWindowMs)
    {
        state.windowStart = now;
        state.cooperativeActions = 0;
    }

    ++state.cooperativeActions;
}

void ModCombatAICoordinator::Prune(uint32 now)
{
    if (_lastPruneMs != 0 && getMSTimeDiff(_lastPruneMs, now) < PruneIntervalMs)
        return;

    _lastPruneMs = now;

    for (auto it = _groups.begin(); it != _groups.end();)
    {
        if (getMSTimeDiff(it->second.windowStart, now) > StateTtlMs)
            it = _groups.erase(it);
        else
            ++it;
    }
}

void ModCombatAICoordinator::Reset()
{
    _groups.clear();
    _lastPruneMs = 0;
}
