#pragma once

#include "Common.h"

class Creature;

class ModCombatAIEligibility
{
public:
    [[nodiscard]] bool IsEligible(Creature* creature) const;
};

extern ModCombatAIEligibility g_ModCombatAIEligibility;
