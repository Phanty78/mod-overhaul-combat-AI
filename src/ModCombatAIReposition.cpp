#include "ModCombatAIReposition.h"
#include "ModCombatAIConfig.h"
#include "ModCombatAIState.h"
#include "Creature.h"
#include "MotionMaster.h"
#include "ThreatManager.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float RepositionTooCloseDistance = 5.0f;
    constexpr float RepositionDesiredDistance = 15.0f;
    constexpr float RepositionRangedSpellMinRange = 25.0f;
    constexpr uint32 RepositionCooldownMs = 5000;
    constexpr uint32 RepositionMovePointId = 0x4D434152;

    bool HasRangedSpell(Creature* creature)
    {
        for (uint32 spellId : creature->m_spells)
        {
            if (!spellId)
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!spellInfo)
                continue;

            float const maxRange = std::max(spellInfo->GetMaxRange(true, creature),
                spellInfo->GetMaxRange(false, creature));

            if (maxRange >= RepositionRangedSpellMinRange)
                return true;
        }

        return false;
    }

    bool IsRepositionBlockingMovement(Creature* creature)
    {
        auto const isBlockingType = [](MovementGeneratorType type)
        {
            return type == POINT_MOTION_TYPE || type == FLEEING_MOTION_TYPE || type == TIMED_FLEEING_MOTION_TYPE;
        };

        MotionMaster* motion = creature->GetMotionMaster();
        return isBlockingType(motion->GetCurrentMovementGeneratorType())
            || isBlockingType(motion->GetMotionSlotType(MOTION_SLOT_CONTROLLED));
    }
}

bool mod_combat_ai::TryReposition(Creature* creature, ModCombatAIContext& context, uint32 now)
{
    if (!creature || !g_ModCombatAIConfig.RepositionEnabled())
        return false;

    if (int32(context.repositionCooldownEnd - now) > 0)
        return false;

    if (creature->HasUnitState(UNIT_STATE_CASTING))
        return false;

    if (context.moduleMoving || IsRepositionBlockingMovement(creature))
        return false;

    Unit* victim = creature->GetVictim();
    if (!victim)
        victim = creature->GetThreatMgr().GetCurrentVictim();
    if (!victim)
        victim = creature->GetThreatMgr().GetAnyTarget();
    if (!victim || !victim->IsAlive())
        return false;

    float const distance = creature->GetDistance(victim);
    if (distance >= RepositionTooCloseDistance)
        return false;

    if (!HasRangedSpell(creature))
        return false;

    float dx = creature->GetPositionX() - victim->GetPositionX();
    float dy = creature->GetPositionY() - victim->GetPositionY();
    float length = std::sqrt(dx * dx + dy * dy);

    if (length < 0.1f)
    {
        dx = 1.0f;
        dy = 0.0f;
        length = 1.0f;
    }

    float const destX = creature->GetPositionX() + (dx / length) * RepositionDesiredDistance;
    float const destY = creature->GetPositionY() + (dy / length) * RepositionDesiredDistance;
    float const destZ = creature->GetPositionZ();

    creature->GetMotionMaster()->MovePoint(RepositionMovePointId, destX, destY, destZ);
    context.repositionCooldownEnd = now + RepositionCooldownMs;
    ++ModCombatAIState::Instance().Counters().repositionAttempts;

    if (g_ModCombatAIConfig.Debug())
    {
        LOG_DEBUG("scripts.ai", "mod-combat-ai: reposition entry {} guid {} backing away from victim entry {} guid {} at distance {:.2f} toward ({:.2f}, {:.2f}, {:.2f})",
            creature->GetEntry(), creature->GetGUID().GetCounter(),
            victim->GetEntry(), victim->GetGUID().GetCounter(), distance,
            destX, destY, destZ);
    }

    return true;
}
