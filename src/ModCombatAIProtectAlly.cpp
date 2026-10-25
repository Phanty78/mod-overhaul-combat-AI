#include "ModCombatAIProtectAlly.h"
#include "ModCombatAIConfig.h"
#include "ModCombatAIState.h"
#include "CellImpl.h"
#include "Creature.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "MotionMaster.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "ThreatManager.h"
#include "Unit.h"

namespace
{
    constexpr float ProtectAllyRadius = 20.0f;
    constexpr float ProtectHealThresholdPct = 80.0f;
    constexpr float ProtectHealerDangerThresholdPct = 60.0f;
    constexpr uint32 ProtectHealCooldownMs = 1000;
    constexpr uint32 ProtectInterceptCooldownMs = 5000;
    constexpr uint32 ProtectMovePointId = 0x4D434150;

    bool HasHealSpell(Creature* creature)
    {
        for (uint32 spellId : creature->m_spells)
        {
            if (!spellId)
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!spellInfo)
                continue;

            if (spellInfo->HasEffect(SPELL_EFFECT_HEAL) && spellInfo->IsPositive())
                return true;
        }

        return false;
    }

    Unit* GetTopThreatTarget(Creature* creature)
    {
        for (ThreatReference const* reference : creature->GetThreatMgr().GetSortedThreatList())
        {
            if (!reference || reference->IsOffline())
                continue;

            Unit* victim = reference->GetVictim();
            if (victim && victim->IsAlive())
                return victim;
        }

        return creature->GetVictim();
    }

    bool IsRetreatMovementActive(Creature* creature, ModCombatAIContext const& context)
    {
        if (context.moduleMoving)
            return true;

        auto const isRetreatType = [](MovementGeneratorType type)
        {
            return type == POINT_MOTION_TYPE || type == FLEEING_MOTION_TYPE || type == TIMED_FLEEING_MOTION_TYPE;
        };

        MotionMaster* motion = creature->GetMotionMaster();
        return isRetreatType(motion->GetCurrentMovementGeneratorType())
            || isRetreatType(motion->GetMotionSlotType(MOTION_SLOT_CONTROLLED));
    }

    class WoundedAllyWorker
    {
    public:
        WoundedAllyWorker(Creature* healer, float range)
            : _healer(healer), _range(range)
        {
        }

        void operator()(Creature* ally)
        {
            if (!ally || ally == _healer)
                return;

            if (!ally->IsAlive())
                return;

            if (!ally->IsFriendlyTo(_healer))
                return;

            if (ally->GetMap() != _healer->GetMap())
                return;

            if (ally->GetZoneId() != _healer->GetZoneId())
                return;

            if (!ally->IsWithinDistInMap(_healer, _range))
                return;

            if (!ally->IsWithinLOSInMap(_healer))
                return;

            float const healthPct = ally->GetHealthPct();
            if (healthPct >= ProtectHealThresholdPct)
                return;

            if (!_best || healthPct < _bestHealthPct)
            {
                _best = ally;
                _bestHealthPct = healthPct;
            }
        }

        [[nodiscard]] Creature* Best() const { return _best; }

    private:
        Creature* _healer;
        float _range;
        Creature* _best = nullptr;
        float _bestHealthPct = 0.0f;
    };

    class HealerInDangerWorker
    {
    public:
        HealerInDangerWorker(Creature* protector, float range)
            : _protector(protector), _range(range)
        {
        }

        void operator()(Creature* ally)
        {
            if (!ally || ally == _protector)
                return;

            if (!ally->IsAlive())
                return;

            if (!ally->IsFriendlyTo(_protector))
                return;

            if (ally->GetMap() != _protector->GetMap())
                return;

            if (ally->GetZoneId() != _protector->GetZoneId())
                return;

            if (!ally->IsWithinDistInMap(_protector, _range))
                return;

            if (!ally->IsWithinLOSInMap(_protector))
                return;

            if (!HasHealSpell(ally))
                return;

            if (!ally->IsEngaged())
                return;

            float const healthPct = ally->GetHealthPct();
            if (healthPct > ProtectHealerDangerThresholdPct)
                return;

            if (!_best || healthPct < _bestHealthPct)
            {
                _best = ally;
                _bestHealthPct = healthPct;
            }
        }

        [[nodiscard]] Creature* Best() const { return _best; }

    private:
        Creature* _protector;
        float _range;
        Creature* _best = nullptr;
        float _bestHealthPct = 0.0f;
    };
}

bool mod_combat_ai::TryProtectAlly(Creature* creature, ModCombatAIContext& context, uint32 now)
{
    if (!creature || !g_ModCombatAIConfig.ProtectAllyEnabled())
        return false;

    if (int32(context.protectAllyCooldownEnd - now) > 0)
        return false;

    if (creature->HasUnitState(UNIT_STATE_CASTING))
        return false;

    ModCombatAIState& state = ModCombatAIState::Instance();

    if (HasHealSpell(creature))
    {
        WoundedAllyWorker worker(creature, ProtectAllyRadius);
        Acore::CreatureWorker<WoundedAllyWorker> gridWorker(creature, worker);
        Cell::VisitObjects(creature, gridWorker, ProtectAllyRadius);

        Creature* ally = worker.Best();
        if (!ally)
        {
            ++state.Counters().protectAllyNoTarget;
            return false;
        }

        for (uint32 spellId : creature->m_spells)
        {
            if (!spellId)
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!spellInfo)
                continue;

            if (!spellInfo->HasEffect(SPELL_EFFECT_HEAL) || !spellInfo->IsPositive())
                continue;

            if (creature->GetSpellCooldown(spellId) != 0)
                continue;

            if (spellInfo->CheckTarget(creature, ally, false) != SPELL_CAST_OK)
                continue;

            float maxRange = spellInfo->GetMaxRange(true, creature);
            if (maxRange <= 0.0f)
                maxRange = 40.0f;

            if (!creature->IsWithinDistInMap(ally, maxRange))
                continue;

            if (!creature->IsWithinLOSInMap(ally))
                continue;

            creature->CastSpell(ally, spellId, false);
            ++state.Counters().protectAllyAttempts;
            ++state.Counters().protectAllyHeals;
            context.protectAllyCooldownEnd = now + ProtectHealCooldownMs;

            if (g_ModCombatAIConfig.Debug())
            {
                LOG_DEBUG("scripts.ai", "mod-combat-ai: protect entry {} guid {} healing ally entry {} guid {} with spell {}",
                    creature->GetEntry(), creature->GetGUID().GetCounter(),
                    ally->GetEntry(), ally->GetGUID().GetCounter(), spellId);
            }

            return true;
        }

        ++state.Counters().protectAllyNoTarget;
        return false;
    }

    if (IsRetreatMovementActive(creature, context))
        return false;

    HealerInDangerWorker worker(creature, ProtectAllyRadius);
    Acore::CreatureWorker<HealerInDangerWorker> gridWorker(creature, worker);
    Cell::VisitObjects(creature, gridWorker, ProtectAllyRadius);

    Creature* healer = worker.Best();
    if (!healer)
    {
        ++state.Counters().protectAllyNoTarget;
        return false;
    }

    Unit* attacker = healer->GetVictim();
    if (!attacker)
        attacker = GetTopThreatTarget(healer);

    if (!attacker || !attacker->IsAlive() || !creature->IsHostileTo(attacker)
        || !attacker->IsWithinDistInMap(creature, ProtectAllyRadius))
    {
        ++state.Counters().protectAllyNoTarget;
        return false;
    }

    if (creature->GetVictim() == attacker)
        return false;

    float const pointX = (healer->GetPositionX() + attacker->GetPositionX()) * 0.5f;
    float const pointY = (healer->GetPositionY() + attacker->GetPositionY()) * 0.5f;
    float const pointZ = (healer->GetPositionZ() + attacker->GetPositionZ()) * 0.5f;

    creature->GetMotionMaster()->MovePoint(ProtectMovePointId, pointX, pointY, pointZ);
    creature->EngageWithTarget(attacker);
    ++state.Counters().protectAllyAttempts;
    ++state.Counters().protectAllyIntercepts;
    context.protectAllyCooldownEnd = now + ProtectInterceptCooldownMs;

    if (g_ModCombatAIConfig.Debug())
    {
        LOG_DEBUG("scripts.ai", "mod-combat-ai: protect entry {} guid {} intercepting attacker entry {} guid {} for healer entry {} guid {}",
            creature->GetEntry(), creature->GetGUID().GetCounter(),
            attacker->GetEntry(), attacker->GetGUID().GetCounter(),
            healer->GetEntry(), healer->GetGUID().GetCounter());
    }

    return true;
}
