#include "ModCombatAIState.h"

#include "Creature.h"
#include "Unit.h"

ModCombatAIState& ModCombatAIState::Instance()
{
    static ModCombatAIState instance;
    return instance;
}

ModCombatAIContext* ModCombatAIState::GetOrCreate(Creature* creature, bool eligible)
{
    if (!creature)
        return nullptr;

    auto [it, inserted] = _contexts.try_emplace(creature->GetGUID());
    if (inserted)
        ++_counters.contextsCreated;

    it->second.eligible = eligible;
    return &it->second;
}

ModCombatAIContext* ModCombatAIState::Find(ObjectGuid guid)
{
    auto it = _contexts.find(guid);
    return it == _contexts.end() ? nullptr : &it->second;
}

void ModCombatAIState::Erase(ObjectGuid guid)
{
    _contexts.erase(guid);
}

void ModCombatAIState::OnEnterCombat(Creature* creature, bool eligible)
{
    ModCombatAIContext* context = GetOrCreate(creature, eligible);
    if (!context)
        return;

    context->nextDecisionMs = 0;
}

void ModCombatAIState::OnExitCombat(Unit* unit)
{
    if (unit && unit->IsCreature())
        Erase(unit->GetGUID());
}

void ModCombatAIState::OnDeath(Unit* unit)
{
    if (unit && unit->IsCreature())
        Erase(unit->GetGUID());
}

void ModCombatAIState::OnRemoveFromWorld(Creature* creature)
{
    if (!creature)
        return;

    Erase(creature->GetGUID());
}

void ModCombatAIState::ResetOnConfigReload()
{
    _contexts.clear();
    _decisionBudgetRemaining = 0;
}

void ModCombatAIState::BeginWorldTick(uint32 maxDecisions)
{
    _decisionBudgetRemaining = maxDecisions;
}

bool ModCombatAIState::ConsumeDecisionBudget()
{
    if (_decisionBudgetRemaining == 0)
        return false;

    --_decisionBudgetRemaining;
    return true;
}
