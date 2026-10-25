#pragma once

#include "AllCreatureScript.h"

class Creature;

class ModCombatAICreatureScript final : public AllCreatureScript
{
public:
    ModCombatAICreatureScript();

    void OnAllCreatureUpdate(Creature* creature, uint32 diff) override;
    void OnCreatureAddWorld(Creature* creature) override;
    void OnCreatureRemoveWorld(Creature* creature) override;
};
