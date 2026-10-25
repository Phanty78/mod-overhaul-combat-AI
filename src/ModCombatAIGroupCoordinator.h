#pragma once

#include "Common.h"

#include <cstddef>
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
    struct GroupKey
    {
        uint32 mapId = 0;
        int32 cellX = 0;
        int32 cellY = 0;

        bool operator==(GroupKey const& other) const = default;
    };

    struct GroupKeyHash
    {
        std::size_t operator()(GroupKey const& key) const;
    };

    struct GroupState
    {
        uint32 cooldownEnd = 0;
    };

    static GroupKey MakeKey(Creature* creature);

    std::unordered_map<GroupKey, GroupState, GroupKeyHash> _groups;
    uint32 _lastPruneMs = 0;
};

extern ModCombatAIGroupCoordinator g_ModCombatAIGroupCoordinator;
