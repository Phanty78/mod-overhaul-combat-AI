#pragma once

#include "Common.h"

class Creature;
struct ModCombatAIContext;

namespace mod_combat_ai
{
    // Returns true when the protection tactic consumed the decision (heal started
    // or interception movement started), false when it did not apply.
    bool TryProtectAlly(Creature* creature, ModCombatAIContext& context, uint32 now);
}
