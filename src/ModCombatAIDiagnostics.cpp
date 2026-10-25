#include "ModCombatAIDiagnostics.h"
#include "ModCombatAIState.h"
#include "Log.h"
#include "Timer.h"

namespace
{
    // Minimum spacing between two summary lines.
    constexpr uint32 SummaryIntervalMs = 60000;
}

ModCombatAIDiagnostics g_ModCombatAIDiagnostics;

ModCombatAIDiagnostics& ModCombatAIDiagnostics::Instance()
{
    static ModCombatAIDiagnostics instance;
    return instance;
}

void ModCombatAIDiagnostics::RecordSelected(ModCombatAITactic tactic)
{
    ModCombatAICounters& counters = ModCombatAIState::Instance().Counters();

    switch (tactic)
    {
        case ModCombatAITactic::Retreat:
            ++counters.actionsRetreat;
            break;
        case ModCombatAITactic::ProtectAlly:
            ++counters.actionsProtectAlly;
            break;
        case ModCombatAITactic::CallForHelp:
            ++counters.actionsCallForHelp;
            break;
        case ModCombatAITactic::Reposition:
            ++counters.actionsReposition;
            break;
        default:
            break;
    }
}

void ModCombatAIDiagnostics::RecordNoAction()
{
    ++ModCombatAIState::Instance().Counters().decisionsNoAction;
}

void ModCombatAIDiagnostics::LogSummary(uint32 now, bool enabled)
{
    if (!enabled)
        return;

    if (_lastSummaryMs != 0 && getMSTimeDiff(_lastSummaryMs, now) < SummaryIntervalMs)
        return;

    _lastSummaryMs = now;

    ModCombatAICounters const& counters = ModCombatAIState::Instance().Counters();
    uint64 const avgDecisionUs = counters.decisionTimeSamples
        ? counters.decisionTimeNs / counters.decisionTimeSamples / 1000
        : 0;

    LOG_INFO("scripts.ai", "mod-combat-ai diagnostics: decisions={} noAction={} retreat={} protect={} callForHelp={} reposition={} skippedBudget={} contexts={} avgDecisionUs={}",
        counters.decisionsEvaluated, counters.decisionsNoAction, counters.actionsRetreat,
        counters.actionsProtectAlly, counters.actionsCallForHelp, counters.actionsReposition,
        counters.skippedBudget, counters.contextsCreated, avgDecisionUs);
}

void ModCombatAIDiagnostics::Reset()
{
    _lastSummaryMs = 0;
}
