#pragma once

#include "UnitScript.h"

class Unit;

class ModCombatAIUnitScript final : public UnitScript
{
public:
    ModCombatAIUnitScript();

    void OnUnitEnterCombat(Unit* unit, Unit* victim) override;
    void OnUnitExitCombat(Unit* unit) override;
    void OnUnitEnterEvadeMode(Unit* unit, uint8 evadeReason) override;
    void OnUnitDeath(Unit* unit, Unit* killer) override;
};
