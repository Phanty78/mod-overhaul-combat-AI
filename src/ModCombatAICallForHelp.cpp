#include "ModCombatAICallForHelp.h"
#include "ModCombatAIConfig.h"
#include "ModCombatAIGroupCoordinator.h"
#include "ModCombatAIState.h"
#include "Creature.h"
#include "ThreatManager.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"

#include <algorithm>
#include <vector>

namespace
{
    class ModCombatAICallForHelpWorker
    {
    public:
        ModCombatAICallForHelpWorker(Creature* caller, Unit* enemy, float range, uint32 maxAllies)
            : _caller(caller), _enemy(enemy), _range(range), _maxAllies(maxAllies)
        {
        }

        void operator()(Creature* ally)
        {
            if (!ally || ally == _caller)
                return;

            if (_allies.size() >= _maxAllies)
                return;

            if (!ally->CanAssistTo(_caller, _enemy, false))
                return;

            if (!ally->IsWithinDistInMap(_caller, _range))
                return;

            if (!ally->IsWithinLOSInMap(_enemy))
                return;

            if (ally->GetZoneId() != _caller->GetZoneId())
                return;

            if (ally->GetMap() != _caller->GetMap())
                return;

            if (std::find(_allies.begin(), _allies.end(), ally) != _allies.end())
                return;

            _allies.push_back(ally);
        }

        [[nodiscard]] std::vector<Creature*> const& Allies() const { return _allies; }

    private:
        Creature* _caller;
        Unit* _enemy;
        float _range;
        uint32 _maxAllies;
        std::vector<Creature*> _allies;
    };
}

bool mod_combat_ai::TryCallForHelp(Creature* creature, ModCombatAIContext& context, uint32 now)
{
    if (!creature)
        return false;

    if (!g_ModCombatAIConfig.CallForHelpEnabled())
        return false;

    uint32 const maxAllies = g_ModCombatAIConfig.CallForHelpMaxAllies();
    if (maxAllies == 0)
        return false;

    if (int32(context.callForHelpCooldownEnd - now) > 0)
        return false;

    ModCombatAIState& state = ModCombatAIState::Instance();

    if (!g_ModCombatAIGroupCoordinator.CanCallForHelp(creature, now))
    {
        ++state.Counters().callForHelpGroupSkipped;
        return false;
    }

    Unit* enemy = creature->GetVictim();
    if (!enemy)
        enemy = creature->GetThreatMgr().GetCurrentVictim();
    if (!enemy)
        enemy = creature->GetThreatMgr().GetAnyTarget();
    if (!enemy || !enemy->IsAlive())
        return false;

    float const radius = g_ModCombatAIConfig.CallForHelpRadius();
    uint32 const cooldownMs = g_ModCombatAIConfig.CallForHelpCooldownMs();

    ModCombatAICallForHelpWorker worker(creature, enemy, radius, maxAllies);
    Acore::CreatureWorker<ModCombatAICallForHelpWorker> gridWorker(creature, worker);
    Cell::VisitObjects(creature, gridWorker, radius);

    context.callForHelpCooldownEnd = now + cooldownMs;
    ++state.Counters().callForHelpAttempts;

    if (worker.Allies().empty())
    {
        ++state.Counters().callForHelpNoAllies;

        if (g_ModCombatAIConfig.Debug())
        {
            LOG_DEBUG("scripts.ai", "mod-combat-ai: call for help entry {} found no allies within {} yards",
                creature->GetEntry(), radius);
        }

        return false;
    }

    uint32 engaged = 0;
    for (Creature* ally : worker.Allies())
    {
        ally->SetNoCallForHelp(true);
        ally->EngageWithTarget(enemy);
        ally->SetNoCallForHelp(false);

        if (ModCombatAIContext* allyContext = state.Find(ally->GetGUID()))
            allyContext->callForHelpCooldownEnd = now + cooldownMs;

        ++state.Counters().callForHelpAlliesEngaged;
        ++engaged;
    }

    g_ModCombatAIGroupCoordinator.OnCallForHelp(creature, now, cooldownMs);

    if (g_ModCombatAIConfig.Debug())
    {
        LOG_DEBUG("scripts.ai", "mod-combat-ai: call for help entry {} engaged {} all{} on {}",
            creature->GetEntry(), engaged, maxAllies, enemy->GetEntry());
    }

    return engaged > 0;
}
