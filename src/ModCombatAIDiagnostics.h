#pragma once

#include "Common.h"
#include "ModCombatAITactic.h"

class ModCombatAIDiagnostics
{
public:
    static ModCombatAIDiagnostics& Instance();

    void RecordSelected(ModCombatAITactic tactic);
    void RecordNoAction();
    void LogSummary(uint32 now, bool enabled);
    void Reset();

private:
    uint32 _lastSummaryMs = 0;
};

extern ModCombatAIDiagnostics g_ModCombatAIDiagnostics;
