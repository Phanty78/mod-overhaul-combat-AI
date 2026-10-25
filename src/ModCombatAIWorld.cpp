#include "ModCombatAIWorld.h"
#include "ModCombatAIConfig.h"
#include "ModCombatAIState.h"
#include "Log.h"

ModCombatAIWorldScript::ModCombatAIWorldScript() : WorldScript("ModCombatAIWorldScript", { WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_UPDATE })
{
}

void ModCombatAIWorldScript::OnAfterConfigLoad(bool reload)
{
    g_ModCombatAIConfig.Initialize(reload);
    ModCombatAIState::Instance().ResetOnConfigReload();

    LOG_INFO("server.loading", "mod-combat-ai: {} (phase 1 skeleton, opt-in only)", g_ModCombatAIConfig.Enabled() ? "enabled" : "disabled");
}

void ModCombatAIWorldScript::OnUpdate(uint32 diff)
{
    (void)diff;

    ModCombatAIState::Instance().BeginWorldTick(g_ModCombatAIConfig.MaxDecisionsPerTick());
}
