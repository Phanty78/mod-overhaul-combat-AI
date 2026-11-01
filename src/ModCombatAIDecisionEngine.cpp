#include "ModCombatAIDecisionEngine.h"
#include "ModCombatAICallForHelp.h"
#include "ModCombatAIConfig.h"
#include "ModCombatAICoordinator.h"
#include "ModCombatAIDiagnostics.h"
#include "ModCombatAIGroupCoordinator.h"
#include "ModCombatAIProtectAlly.h"
#include "ModCombatAIReposition.h"
#include "ModCombatAIRetreat.h"
#include "ModCombatAIState.h"
#include "ModCombatAITactic.h"
#include "Creature.h"
#include "Log.h"
#include "Random.h"

#include <algorithm>
#include <array>
#include <cstddef>

ModCombatAIDecisionEngine g_ModCombatAIDecisionEngine;

namespace
{
    struct Candidate
    {
        ModCombatAITactic tactic;
        float score;
    };

    bool IsCooldownReady(uint32 cooldownEnd, uint32 now)
    {
        // Wrap-safe as long as cooldowns stay well below the 24.8 day uint32 range.
        return int32(cooldownEnd - now) <= 0;
    }

    char const* TacticName(ModCombatAITactic tactic)
    {
        switch (tactic)
        {
            case ModCombatAITactic::Retreat:
                return "retreat";
            case ModCombatAITactic::ProtectAlly:
                return "protect ally";
            case ModCombatAITactic::CallForHelp:
                return "call for help";
            case ModCombatAITactic::Reposition:
                return "reposition";
            case ModCombatAITactic::Count:
                break;
        }

        return "unknown";
    }
}

ModCombatAIDecisionEngine& ModCombatAIDecisionEngine::Instance()
{
    static ModCombatAIDecisionEngine instance;
    return instance;
}

bool ModCombatAIDecisionEngine::Decide(Creature* creature, ModCombatAIContext& context, uint32 now)
{
    if (!creature)
        return false;

    if (context.moduleMoving)
    {
        // An ongoing retreat keeps priority until its generator finishes or expires.
        // The watchdog is maintenance, not a new action, so it is not counted.
        mod_combat_ai::TryRetreat(creature, context, now);
        return true;
    }

    ModCombatAIConfig const& config = g_ModCombatAIConfig;

    // Intelligence weights scale the base scores; 0.0 yields half weight and 1.0 full
    // weight, so the configured value never disables a tactic on its own.
    float const individualMult = 0.5f + config.IndividualIntelligence();
    float const groupMult = 0.5f + config.GroupIntelligence();

    // Bounded per-candidate jitter avoids synchronized decisions without hiding a
    // clearly dominant tactic.
    auto const variation = []() -> float
    {
        return 0.9f + float(urand(0, 20)) / 100.0f;
    };

    std::array<Candidate, static_cast<std::size_t>(ModCombatAITactic::Count)> candidates{};
    std::size_t candidateCount = 0;

    auto const addCandidate = [&candidates, &candidateCount](ModCombatAITactic tactic, float score)
    {
        if (score <= 0.0f)
            return;

        candidates[candidateCount] = { tactic, score };
        ++candidateCount;
    };

    float retreatScore = 0.0f;
    if (config.RetreatEnabled() && IsCooldownReady(context.retreatCooldownEnd, now))
    {
        float const healthPct = creature->GetHealthPct();
        float const thresholdPct = float(config.RetreatHealthThresholdPct());
        if (healthPct <= thresholdPct)
            retreatScore = 100.0f + (thresholdPct - healthPct) * 2.0f;
    }
    addCandidate(ModCombatAITactic::Retreat, retreatScore * individualMult * variation());

    float protectAllyScore = 0.0f;
    if (config.ProtectAllyEnabled() && IsCooldownReady(context.protectAllyCooldownEnd, now))
        protectAllyScore = 60.0f;
    addCandidate(ModCombatAITactic::ProtectAlly, protectAllyScore * groupMult * variation());

    float callForHelpScore = 0.0f;
    if (config.CallForHelpEnabled() && IsCooldownReady(context.callForHelpCooldownEnd, now)
        && g_ModCombatAIGroupCoordinator.CanCallForHelp(creature, now))
    {
        callForHelpScore = 40.0f;
    }
    addCandidate(ModCombatAITactic::CallForHelp, callForHelpScore * groupMult * variation());

    float repositionScore = 0.0f;
    if (config.RepositionEnabled() && IsCooldownReady(context.repositionCooldownEnd, now))
        repositionScore = 30.0f;
    addCandidate(ModCombatAITactic::Reposition, repositionScore * individualMult * variation());

    std::sort(candidates.begin(), candidates.begin() + static_cast<std::ptrdiff_t>(candidateCount),
        [](Candidate const& left, Candidate const& right) { return left.score > right.score; });

    for (std::size_t i = 0; i < candidateCount; ++i)
    {
        Candidate const& candidate = candidates[i];

        // Protect ally coordinates its own interception internally; only call for help
        // needs the per-group coordination gate here.
        if (candidate.tactic == ModCombatAITactic::CallForHelp
            && !g_ModCombatAICoordinator.CanAct(creature, candidate.tactic, now, config.GroupIntelligence()))
        {
            continue;
        }

        bool acted = false;
        switch (candidate.tactic)
        {
            case ModCombatAITactic::Retreat:
                acted = mod_combat_ai::TryRetreat(creature, context, now);
                break;
            case ModCombatAITactic::ProtectAlly:
                acted = mod_combat_ai::TryProtectAlly(creature, context, now);
                break;
            case ModCombatAITactic::CallForHelp:
                acted = mod_combat_ai::TryCallForHelp(creature, context, now);
                break;
            case ModCombatAITactic::Reposition:
                acted = mod_combat_ai::TryReposition(creature, context, now);
                break;
            case ModCombatAITactic::Count:
                break;
        }

        if (!acted)
            continue;

        if (candidate.tactic == ModCombatAITactic::CallForHelp)
            g_ModCombatAICoordinator.OnAction(creature, candidate.tactic, now);

        g_ModCombatAIDiagnostics.RecordSelected(candidate.tactic);

        if (config.Debug())
        {
            LOG_DEBUG("scripts.ai", "mod-combat-ai: creature entry {} guid {} selected {} (score {:.2f})",
                creature->GetEntry(), creature->GetGUID().ToString().c_str(), TacticName(candidate.tactic), candidate.score);
        }

        return true;
    }

    g_ModCombatAIDiagnostics.RecordNoAction();
    return false;
}
