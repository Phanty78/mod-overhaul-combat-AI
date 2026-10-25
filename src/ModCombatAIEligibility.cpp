#include "ModCombatAIEligibility.h"
#include "ModCombatAIConfig.h"
#include "Creature.h"
#include "Map.h"
#include "SharedDefines.h"
#include "UnitDefines.h"

ModCombatAIEligibility g_ModCombatAIEligibility;

bool ModCombatAIEligibility::IsEligible(Creature* creature) const
{
    // Only creatures that are present, alive and already fighting qualify.
    if (!creature || !creature->IsInWorld() || !creature->IsAlive() || !creature->IsEngaged())
        return false;

    // Player-controlled units already have their own AI.
    if (creature->IsPet() || creature->IsCharmed())
        return false;

    // Evading creatures are being reset by the core and must not be steered.
    if (creature->IsInEvadeMode())
        return false;

    // Civilians are never meant to fight.
    if (creature->IsCivilian())
        return false;

    if (creature->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE))
        return false;

    CreatureTemplate const* info = creature->GetCreatureTemplate();
    if (!info)
        return false;

    if (g_ModCombatAIConfig.HumanoidsOnly() && info->type != CREATURE_TYPE_HUMANOID)
        return false;

    // Registered script AI takes precedence over the module.
    if (!creature->GetScriptName().empty())
        return false;

    if (creature->isWorldBoss() || creature->IsDungeonBoss())
        return false;

    Map* map = creature->GetMap();
    if (!map)
        return false;

    if (g_ModCombatAIConfig.OpenWorldOnly() && !map->IsWorldMap())
        return false;

    uint32 const entry = creature->GetEntry();
    uint32 const mapId = creature->GetMapId();

    if (g_ModCombatAIConfig.IsEntryExcluded(entry) || g_ModCombatAIConfig.IsMapExcluded(mapId))
        return false;

    bool const included = g_ModCombatAIConfig.IsEntryIncluded(entry) || g_ModCombatAIConfig.IsMapIncluded(mapId);
    if (g_ModCombatAIConfig.OptInOnly() && !included)
        return false;

    return true;
}
