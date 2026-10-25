#pragma once

#include "Common.h"

class Creature;
struct ModCombatAIContext;

namespace mod_combat_ai
{
    bool TryCallForHelp(Creature* creature, ModCombatAIContext& context, uint32 now);
}
