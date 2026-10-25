#include "ModCombatAIWorld.h"
#include "ModCombatAIConfig.h"
#include "ModCombatAICoordinator.h"
#include "ModCombatAIDiagnostics.h"
#include "ModCombatAIGroupCoordinator.h"
#include "ModCombatAIState.h"
#include "Log.h"
#include "Timer.h"

ModCombatAIWorldScript::ModCombatAIWorldScript() : WorldScript("ModCombatAIWorldScript", { WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_UPDATE })
{
}

void ModCombatAIWorldScript::OnAfterConfigLoad(bool reload)
{
    g_ModCombatAIConfig.Initialize(reload);
    ModCombatAIState::Instance().ResetOnConfigReload();
    g_ModCombatAICoordinator.Reset();
    g_ModCombatAIDiagnostics.Reset();
    g_ModCombatAIGroupCoordinator.Reset();

    LOG_INFO("server.loading", "mod-combat-ai: {} (call for help {})", g_ModCombatAIConfig.Enabled() ? "enabled" : "disabled",
        g_ModCombatAIConfig.CallForHelpEnabled() ? "enabled" : "disabled");
}

void ModCombatAIWorldScript::OnUpdate(uint32 diff)
{
    (void)diff;

    uint32 const now = getMSTime();

    ModCombatAIState::Instance().BeginWorldTick(g_ModCombatAIConfig.MaxDecisionsPerTick());
    g_ModCombatAICoordinator.Prune(now);
    g_ModCombatAIDiagnostics.LogSummary(now, g_ModCombatAIConfig.Debug());
    g_ModCombatAIGroupCoordinator.Prune(now, g_ModCombatAIConfig.CallForHelpCooldownMs());
}
