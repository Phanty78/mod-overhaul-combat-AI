#pragma once

#include "WorldScript.h"

class ModCombatAIWorldScript final : public WorldScript
{
public:
    ModCombatAIWorldScript();

    void OnAfterConfigLoad(bool reload) override;
    void OnUpdate(uint32 diff) override;
};
