#pragma once

#include "Common.h"
#include "ObjectGuid.h"

#include <unordered_map>

class Creature;
class Unit;

struct ModCombatAIContext
{
    uint32 nextDecisionMs = 0;
    uint32 lastDecisionMs = 0;
    uint32 callForHelpCooldownEnd = 0;
    uint32 retreatCooldownEnd = 0;
    uint32 protectAllyCooldownEnd = 0;
    uint32 repositionCooldownEnd = 0;
    uint32 moveStartMs = 0;
    uint32 lastProgressMs = 0;
    float lastX = 0.0f;
    float lastY = 0.0f;
    float lastZ = 0.0f;
    bool moduleMoving = false;
    bool eligible = false;
};

struct ModCombatAICounters
{
    uint64 decisionsEvaluated = 0;
    uint64 skippedDisabled = 0;
    uint64 skippedNotEligible = 0;
    uint64 skippedBudget = 0;
    uint64 contextsCreated = 0;
    uint64 callForHelpAttempts = 0;
    uint64 callForHelpAlliesEngaged = 0;
    uint64 callForHelpGroupSkipped = 0;
    uint64 callForHelpNoAllies = 0;
    uint64 retreatAttempts = 0;
    uint64 retreatAllies = 0;
    uint64 retreatFallbacks = 0;
    uint64 retreatStuck = 0;
    uint64 retreatCompleted = 0;
    uint64 protectAllyAttempts = 0;
    uint64 protectAllyHeals = 0;
    uint64 protectAllyIntercepts = 0;
    uint64 protectAllyNoTarget = 0;
    uint64 repositionAttempts = 0;
    uint64 actionsRetreat = 0;
    uint64 actionsProtectAlly = 0;
    uint64 actionsCallForHelp = 0;
    uint64 actionsReposition = 0;
    uint64 decisionsNoAction = 0;
    uint64 decisionTimeNs = 0;
    uint64 decisionTimeSamples = 0;
};

class ModCombatAIState
{
public:
    static ModCombatAIState& Instance();

    ModCombatAIContext* GetOrCreate(Creature* creature, bool eligible);
    ModCombatAIContext* Find(ObjectGuid guid);
    void Erase(ObjectGuid guid);
    void OnEnterCombat(Creature* creature, bool eligible);
    void OnExitCombat(Unit* unit);
    void OnDeath(Unit* unit);
    void OnRemoveFromWorld(Creature* creature);
    void ResetOnConfigReload();

    void BeginWorldTick(uint32 maxDecisions);
    [[nodiscard]] bool ConsumeDecisionBudget();

    [[nodiscard]] ModCombatAICounters const& Counters() const { return _counters; }
    ModCombatAICounters& Counters() { return _counters; }

private:
    ModCombatAIState() = default;

    std::unordered_map<ObjectGuid, ModCombatAIContext> _contexts;
    ModCombatAICounters _counters;
    uint32 _decisionBudgetRemaining = 0;
};
