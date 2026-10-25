#pragma once

#include "Common.h"
#include "ModCombatAIGroupKey.h"

#include <unordered_map>

class Creature;

class ModCombatAIGroupCoordinator
{
public:
    static ModCombatAIGroupCoordinator& Instance();

    [[nodiscard]] bool CanCallForHelp(Creature* creature, uint32 now) const;
    void OnCallForHelp(Creature* creature, uint32 now, uint32 cooldownMs);
    void Prune(uint32 now, uint32 cooldownMs);
    void Reset();

private:
    struct GroupState
    {
        uint32 cooldownEnd = 0;
    };

    std::unordered_map<ModCombatAIGroupKey, GroupState, ModCombatAIGroupKeyHash> _groups;
    uint32 _lastPruneMs = 0;
};

extern ModCombatAIGroupCoordinator g_ModCombatAIGroupCoordinator;
