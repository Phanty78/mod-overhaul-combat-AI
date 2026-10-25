#include "ModCombatAIConfig.h"
#include "Tokenize.h"

#include <cctype>
#include <charconv>
#include <string>

namespace
{
std::string_view TrimAsciiWhitespace(std::string_view value)
{
    auto const isWhitespace = [](char character) { return std::isspace(static_cast<unsigned char>(character)) != 0; };

    while (!value.empty() && isWhitespace(value.front()))
        value.remove_prefix(1);

    while (!value.empty() && isWhitespace(value.back()))
        value.remove_suffix(1);

    return value;
}

bool IsUnitInterval(float const& value)
{
    return value >= 0.0f && value <= 1.0f;
}
}

ModCombatAIConfig g_ModCombatAIConfig;

ModCombatAIConfig::ModCombatAIConfig() : ConfigValueCache(ModCombatAIConfigKey::Count) { }

void ModCombatAIConfig::BuildConfigCache()
{
    SetConfigValue<bool>(ModCombatAIConfigKey::Enable, "CombatAI.Enable", false);
    SetConfigValue<bool>(ModCombatAIConfigKey::HumanoidsOnly, "CombatAI.HumanoidsOnly", true);
    SetConfigValue<bool>(ModCombatAIConfigKey::OpenWorldOnly, "CombatAI.OpenWorldOnly", true);
    SetConfigValue<bool>(ModCombatAIConfigKey::OptInOnly, "CombatAI.OptInOnly", true);
    SetConfigValue<std::string>(ModCombatAIConfigKey::IncludeEntries, "CombatAI.IncludeEntries", "");
    SetConfigValue<std::string>(ModCombatAIConfigKey::ExcludeEntries, "CombatAI.ExcludeEntries", "");
    SetConfigValue<std::string>(ModCombatAIConfigKey::IncludeMaps, "CombatAI.IncludeMaps", "");
    SetConfigValue<std::string>(ModCombatAIConfigKey::ExcludeMaps, "CombatAI.ExcludeMaps", "");
    SetConfigValue<uint32>(ModCombatAIConfigKey::DecisionIntervalMs, "CombatAI.DecisionIntervalMs", 1000,
        ConfigValueCache::Reloadable::Yes, [](uint32 const& value) { return value > 0; }, "> 0");
    SetConfigValue<uint32>(ModCombatAIConfigKey::DecisionJitterMs, "CombatAI.DecisionJitterMs", 250);
    SetConfigValue<float>(ModCombatAIConfigKey::IndividualIntelligence, "CombatAI.IndividualIntelligence", 0.5f,
        ConfigValueCache::Reloadable::Yes, IsUnitInterval, "0.0 - 1.0");
    SetConfigValue<float>(ModCombatAIConfigKey::GroupIntelligence, "CombatAI.GroupIntelligence", 0.5f,
        ConfigValueCache::Reloadable::Yes, IsUnitInterval, "0.0 - 1.0");
    SetConfigValue<bool>(ModCombatAIConfigKey::CallForHelpEnable, "CombatAI.CallForHelp.Enable", true);
    SetConfigValue<uint32>(ModCombatAIConfigKey::CallForHelpMaxAllies, "CombatAI.CallForHelp.MaxAllies", 2,
        ConfigValueCache::Reloadable::Yes, [](uint32 const& value) { return value <= 10; }, "<= 10");
    SetConfigValue<float>(ModCombatAIConfigKey::CallForHelpRadius, "CombatAI.CallForHelp.Radius", 15.0f,
        ConfigValueCache::Reloadable::Yes, [](float const& value) { return value > 0.0f; }, "> 0");
    SetConfigValue<uint32>(ModCombatAIConfigKey::CallForHelpCooldownMs, "CombatAI.CallForHelp.CooldownMs", 30000);
    SetConfigValue<bool>(ModCombatAIConfigKey::RetreatEnable, "CombatAI.Retreat.Enable", true);
    SetConfigValue<uint32>(ModCombatAIConfigKey::RetreatHealthThresholdPct, "CombatAI.Retreat.HealthThresholdPct", 20,
        ConfigValueCache::Reloadable::Yes, [](uint32 const& value) { return value >= 1 && value <= 100; }, "1 - 100");
    SetConfigValue<uint32>(ModCombatAIConfigKey::RetreatCooldownMs, "CombatAI.Retreat.CooldownMs", 60000);
    SetConfigValue<bool>(ModCombatAIConfigKey::ProtectAllyEnable, "CombatAI.ProtectAlly.Enable", true);
    SetConfigValue<uint32>(ModCombatAIConfigKey::MaxDecisionsPerTick, "CombatAI.MaxDecisionsPerTick", 64,
        ConfigValueCache::Reloadable::Yes, [](uint32 const& value) { return value > 0; }, "> 0");
    SetConfigValue<bool>(ModCombatAIConfigKey::Debug, "CombatAI.Debug", false);

    RebuildIdSets();
}

void ModCombatAIConfig::RebuildIdSets()
{
    _includeEntries = ParseIdSet(GetConfigValue<std::string>(ModCombatAIConfigKey::IncludeEntries));
    _excludeEntries = ParseIdSet(GetConfigValue<std::string>(ModCombatAIConfigKey::ExcludeEntries));
    _includeMaps = ParseIdSet(GetConfigValue<std::string>(ModCombatAIConfigKey::IncludeMaps));
    _excludeMaps = ParseIdSet(GetConfigValue<std::string>(ModCombatAIConfigKey::ExcludeMaps));
}

std::unordered_set<uint32> ModCombatAIConfig::ParseIdSet(std::string_view value)
{
    std::unordered_set<uint32> ids;

    for (std::string_view token : Acore::Tokenize(value, ',', false))
    {
        token = TrimAsciiWhitespace(token);
        if (token.empty())
            continue;

        uint32 id = 0;
        std::from_chars_result const parsed = std::from_chars(token.data(), token.data() + token.size(), id);
        if (parsed.ec != std::errc() || parsed.ptr != token.data() + token.size())
            continue;

        ids.insert(id);
    }

    return ids;
}
