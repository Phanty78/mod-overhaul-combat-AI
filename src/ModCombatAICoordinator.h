#pragma once

#include "ModCombatAITactic.h"

#include "Common.h"
#include "ModCombatAIGroupKey.h"

#include <unordered_map>

class Creature;

class ModCombatAICoordinator
{
public:
    static ModCombatAICoordinator& Instance();

    [[nodiscard]] bool CanAct(Creature* creature, ModCombatAITactic tactic, uint32 now, float groupIntelligence) const;
    void OnAction(Creature* creature, ModCombatAITactic tactic, uint32 now);
    void Prune(uint32 now);
    void Reset();

private:
    struct GroupState
    {
        uint32 windowStart = 0;
        uint32 cooperativeActions = 0;
    };

    std::unordered_map<ModCombatAIGroupKey, GroupState, ModCombatAIGroupKeyHash> _groups;
    uint32 _lastPruneMs = 0;
};

extern ModCombatAICoordinator g_ModCombatAICoordinator;
