#include "ModCombatAICreature.h"
#include "ModCombatAIUnit.h"
#include "ModCombatAIWorld.h"

void Addmod_combat_aiScripts()
{
    new ModCombatAIWorldScript();
    new ModCombatAICreatureScript();
    new ModCombatAIUnitScript();
}
