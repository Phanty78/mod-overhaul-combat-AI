#pragma once

#include "Common.h"

class Creature;
struct ModCombatAIContext;

class ModCombatAIDecisionEngine
{
public:
    static ModCombatAIDecisionEngine& Instance();

    // Evaluates the four tactics, applies individual/group intelligence and a
    // bounded random variation, then executes at most one action.
    bool Decide(Creature* creature, ModCombatAIContext& context, uint32 now);
};

extern ModCombatAIDecisionEngine g_ModCombatAIDecisionEngine;
