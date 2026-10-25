#include "ModCombatAIWorld.h"
#include "ModCombatAIConfig.h"
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
    g_ModCombatAIGroupCoordinator.Reset();

    LOG_INFO("server.loading", "mod-combat-ai: {} (call for help {})", g_ModCombatAIConfig.Enabled() ? "enabled" : "disabled",
        g_ModCombatAIConfig.CallForHelpEnabled() ? "enabled" : "disabled");
}

void ModCombatAIWorldScript::OnUpdate(uint32 diff)
{
    (void)diff;

    ModCombatAIState::Instance().BeginWorldTick(g_ModCombatAIConfig.MaxDecisionsPerTick());
    g_ModCombatAIGroupCoordinator.Prune(getMSTime(), g_ModCombatAIConfig.CallForHelpCooldownMs());
}
