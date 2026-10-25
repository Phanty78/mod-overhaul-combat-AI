#pragma once

#include "Common.h"

enum class ModCombatAITactic : uint8
{
    Retreat = 0,
    ProtectAlly = 1,
    CallForHelp = 2,
    Reposition = 3,
    Count = 4
};
