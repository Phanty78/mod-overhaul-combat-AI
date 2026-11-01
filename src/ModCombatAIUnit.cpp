#include "ModCombatAIUnit.h"
#include "ModCombatAIConfig.h"
#include "ModCombatAIEligibility.h"
#include "ModCombatAIState.h"
#include "Creature.h"
#include "Log.h"

ModCombatAIUnitScript::ModCombatAIUnitScript() : UnitScript("ModCombatAIUnitScript", true,
    {
        UNITHOOK_ON_UNIT_ENTER_COMBAT,
        UNITHOOK_ON_UNIT_EXIT_COMBAT,
        UNITHOOK_ON_UNIT_ENTER_EVADE_MODE,
        UNITHOOK_ON_UNIT_DEATH
    })
{
}

void ModCombatAIUnitScript::OnUnitEnterCombat(Unit* unit, Unit* victim)
{
    (void)victim;

    if (!g_ModCombatAIConfig.Enabled() || !unit || !unit->IsCreature())
        return;

    Creature* creature = unit->ToCreature();
    if (!creature->IsInWorld() || !creature->IsAlive())
        return;

    bool const eligible = g_ModCombatAIEligibility.IsEligible(creature);
    ModCombatAIState::Instance().OnEnterCombat(creature, eligible);

    if (eligible && g_ModCombatAIConfig.Debug())
    {
        LOG_DEBUG("scripts.ai", "mod-combat-ai: creature entry {} guid {} entered combat (eligible)", creature->GetEntry(), creature->GetGUID().ToString().c_str());
    }
}

void ModCombatAIUnitScript::OnUnitExitCombat(Unit* unit)
{
    if (unit && unit->IsCreature())
    {
        ModCombatAIState::Instance().Erase(unit->GetGUID());
    }
}

void ModCombatAIUnitScript::OnUnitEnterEvadeMode(Unit* unit, uint8 evadeReason)
{
    (void)evadeReason;

    if (unit && unit->IsCreature())
    {
        ModCombatAIState::Instance().Erase(unit->GetGUID());
    }
}

void ModCombatAIUnitScript::OnUnitDeath(Unit* unit, Unit* killer)
{
    (void)killer;

    if (unit && unit->IsCreature())
    {
        ModCombatAIState::Instance().Erase(unit->GetGUID());
    }
}
