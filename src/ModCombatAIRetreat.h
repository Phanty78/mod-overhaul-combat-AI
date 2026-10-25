#pragma once

#include "Common.h"

class Creature;
struct ModCombatAIContext;

namespace mod_combat_ai
{
    // Returns true when the retreat tactic consumed the decision (movement started
    // or an existing retreat is being watched), false when it did not apply.
    bool TryRetreat(Creature* creature, ModCombatAIContext& context, uint32 now);
}
