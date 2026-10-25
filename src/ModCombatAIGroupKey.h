#pragma once

#include "Common.h"

#include <cstddef>

class Creature;

struct ModCombatAIGroupKey
{
    uint32 mapId = 0;
    int32 cellX = 0;
    int32 cellY = 0;

    bool operator==(ModCombatAIGroupKey const& other) const = default;
};

struct ModCombatAIGroupKeyHash
{
    std::size_t operator()(ModCombatAIGroupKey const& key) const;
};

ModCombatAIGroupKey MakeModCombatAIGroupKey(Creature* creature);
