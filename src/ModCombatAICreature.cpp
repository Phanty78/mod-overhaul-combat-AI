#include "ModCombatAICreature.h"
#include "ModCombatAIConfig.h"
#include "ModCombatAIDecisionEngine.h"
#include "ModCombatAIEligibility.h"
#include "ModCombatAIState.h"
#include "Creature.h"
#include "Log.h"
#include "Random.h"
#include "Timer.h"

#include <chrono>

ModCombatAICreatureScript::ModCombatAICreatureScript() : AllCreatureScript("ModCombatAICreatureScript")
{
}

void ModCombatAICreatureScript::OnAllCreatureUpdate(Creature* creature, uint32 diff)
{
    (void)diff;

    if (!g_ModCombatAIConfig.Enabled())
        return;

    if (!creature || !creature->IsInWorld() || !creature->IsAlive() || !creature->IsEngaged())
        return;

    ModCombatAIState& state = ModCombatAIState::Instance();
    ModCombatAIContext* context = state.Find(creature->GetGUID());

    if (!context)
    {
        bool const eligible = g_ModCombatAIEligibility.IsEligible(creature);
        context = state.GetOrCreate(creature, eligible);

        if (!eligible)
        {
            ++state.Counters().skippedNotEligible;
            return;
        }
    }

    if (!context->eligible)
    {
        ++state.Counters().skippedNotEligible;
        return;
    }

    uint32 const now = getMSTime();
    if (now < context->nextDecisionMs)
        return;

    uint32 const jitter = g_ModCombatAIConfig.DecisionJitterMs();
    context->nextDecisionMs = now + g_ModCombatAIConfig.DecisionIntervalMs() + (jitter ? urand(0, jitter) : 0);

    if (!state.ConsumeDecisionBudget())
    {
        ++state.Counters().skippedBudget;
        return;
    }

    context->lastDecisionMs = now;
    ++state.Counters().decisionsEvaluated;

    // The decision engine scores the four tactics and executes at most one action.
    if (g_ModCombatAIConfig.Debug())
    {
        auto const start = std::chrono::steady_clock::now();
        ModCombatAIDecisionEngine::Instance().Decide(creature, *context, now);
        auto const end = std::chrono::steady_clock::now();
        state.Counters().decisionTimeNs += uint64(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
        ++state.Counters().decisionTimeSamples;
    }
    else
    {
        ModCombatAIDecisionEngine::Instance().Decide(creature, *context, now);
    }

    if (g_ModCombatAIConfig.Debug())
    {
        LOG_DEBUG("scripts.ai", "mod-combat-ai: decision evaluated for creature entry {} guid {} map {}",
            creature->GetEntry(), creature->GetGUID().GetCounter(), creature->GetMapId());
    }
}

void ModCombatAICreatureScript::OnCreatureAddWorld(Creature* creature)
{
    (void)creature;
}

void ModCombatAICreatureScript::OnCreatureRemoveWorld(Creature* creature)
{
    if (!creature)
        return;

    ModCombatAIState::Instance().Erase(creature->GetGUID());
}
