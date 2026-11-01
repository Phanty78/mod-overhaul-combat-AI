#include "ModCombatAIRetreat.h"
#include "ModCombatAIConfig.h"
#include "ModCombatAIState.h"
#include "Creature.h"
#include "MotionMaster.h"
#include "ThreatManager.h"
#include "Unit.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "Timer.h"

namespace
{
    constexpr uint32 RetreatStuckTimeoutMs = 3000;
    constexpr float RetreatProgressEpsilonSq = 1.0f;
    constexpr uint32 RetreatFallbackFleeMs = 3000;
    constexpr uint32 RetreatMovePointId = 0x4D434149;

    class ModCombatAIRetreatAllyWorker
    {
    public:
        ModCombatAIRetreatAllyWorker(Creature* caller, float range)
            : _caller(caller), _range(range)
        {
        }

        void operator()(Creature* ally)
        {
            if (!ally || ally == _caller)
                return;

            if (!ally->IsAlive() || ally->IsInEvadeMode())
                return;

            if (!ally->IsFriendlyTo(_caller))
                return;

            if (ally->GetMap() != _caller->GetMap())
                return;

            if (ally->GetZoneId() != _caller->GetZoneId())
                return;

            if (!ally->IsWithinDistInMap(_caller, _range))
                return;

            if (!ally->IsWithinLOSInMap(_caller))
                return;

            float const distance = ally->GetDistance(_caller);
            if (!_best || distance < _bestDistance)
            {
                _best = ally;
                _bestDistance = distance;
            }
        }

        [[nodiscard]] Creature* Best() const { return _best; }

    private:
        Creature* _caller;
        float _range;
        Creature* _best = nullptr;
        float _bestDistance = 0.0f;
    };

    // Watches an active retreat movement. Returns true because an ongoing retreat
    // keeps consuming the decision until its movement generator finishes or expires.
    bool UpdateWatchdog(Creature* creature, ModCombatAIContext& context, uint32 now)
    {
        MovementGeneratorType const activeType = creature->GetMotionMaster()->GetCurrentMovementGeneratorType();
        MovementGeneratorType const controlledType = creature->GetMotionMaster()->GetMotionSlotType(MOTION_SLOT_CONTROLLED);

        auto const isRetreatMovement = [](MovementGeneratorType type)
        {
            return type == POINT_MOTION_TYPE || type == FLEEING_MOTION_TYPE || type == TIMED_FLEEING_MOTION_TYPE;
        };

        if (!isRetreatMovement(activeType) && !isRetreatMovement(controlledType))
        {
            context.moduleMoving = false;
            ++ModCombatAIState::Instance().Counters().retreatCompleted;
            return true;
        }

        float const dx = creature->GetPositionX() - context.lastX;
        float const dy = creature->GetPositionY() - context.lastY;
        float const dz = creature->GetPositionZ() - context.lastZ;
        float const movedSq = dx * dx + dy * dy + dz * dz;

        if (movedSq > RetreatProgressEpsilonSq)
        {
            context.lastX = creature->GetPositionX();
            context.lastY = creature->GetPositionY();
            context.lastZ = creature->GetPositionZ();
            context.lastProgressMs = now;
        }

        if (getMSTimeDiff(context.lastProgressMs, now) > RetreatStuckTimeoutMs)
        {
            creature->GetMotionMaster()->MovementExpired();
            context.moduleMoving = false;
            ++ModCombatAIState::Instance().Counters().retreatStuck;

            if (g_ModCombatAIConfig.Debug())
            {
                LOG_DEBUG("scripts.ai", "mod-combat-ai: retreat entry {} guid {} made no progress, expiring movement",
                    creature->GetEntry(), creature->GetGUID().ToString().c_str());
            }
        }

        return true;
    }
}

bool mod_combat_ai::TryRetreat(Creature* creature, ModCombatAIContext& context, uint32 now)
{
    if (!creature || !g_ModCombatAIConfig.RetreatEnabled())
    {
        if (context.moduleMoving)
            context.moduleMoving = false;

        return false;
    }

    if (context.moduleMoving)
        return UpdateWatchdog(creature, context, now);

    if (int32(context.retreatCooldownEnd - now) > 0)
        return false;

    if (creature->HasUnitState(UNIT_STATE_FLEEING))
        return false;

    if (creature->GetHealthPct() > float(g_ModCombatAIConfig.RetreatHealthThresholdPct()))
        return false;

    Unit* enemy = creature->GetVictim();
    if (!enemy)
        enemy = creature->GetThreatMgr().GetCurrentVictim();
    if (!enemy)
        enemy = creature->GetThreatMgr().GetAnyTarget();
    if (!enemy || !enemy->IsAlive())
        return false;

    float const radius = g_ModCombatAIConfig.RetreatRadius();

    ModCombatAIRetreatAllyWorker worker(creature, radius);
    Acore::CreatureWorker<ModCombatAIRetreatAllyWorker> gridWorker(creature, worker);
    Cell::VisitObjects(creature, gridWorker, radius);

    ModCombatAIState& state = ModCombatAIState::Instance();
    ++state.Counters().retreatAttempts;

    if (Creature* ally = worker.Best())
    {
        creature->GetMotionMaster()->MovePoint(RetreatMovePointId,
            ally->GetPositionX(), ally->GetPositionY(), ally->GetPositionZ());
        ++state.Counters().retreatAllies;

        if (g_ModCombatAIConfig.Debug())
        {
            LOG_DEBUG("scripts.ai", "mod-combat-ai: retreat entry {} guid {} moving to ally entry {} guid {}",
                creature->GetEntry(), creature->GetGUID().ToString().c_str(),
                ally->GetEntry(), ally->GetGUID().ToString().c_str());
        }
    }
    else
    {
        creature->GetMotionMaster()->MoveFleeing(enemy, RetreatFallbackFleeMs);
        ++state.Counters().retreatFallbacks;

        if (g_ModCombatAIConfig.Debug())
        {
            LOG_DEBUG("scripts.ai", "mod-combat-ai: retreat entry {} guid {} has no ally within {} yards, fleeing for {} ms",
                creature->GetEntry(), creature->GetGUID().ToString().c_str(), radius, RetreatFallbackFleeMs);
        }
    }

    context.moduleMoving = true;
    context.moveStartMs = now;
    context.lastProgressMs = now;
    context.lastX = creature->GetPositionX();
    context.lastY = creature->GetPositionY();
    context.lastZ = creature->GetPositionZ();
    context.retreatCooldownEnd = now + g_ModCombatAIConfig.RetreatCooldownMs();

    return true;
}
