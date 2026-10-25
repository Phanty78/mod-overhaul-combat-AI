#pragma once

#include "Common.h"
#include "Config.h"
#include "ConfigValueCache.h"
#include "Log.h"

#include <string_view>
#include <unordered_set>

enum class ModCombatAIConfigKey : uint32
{
    Enable,
    HumanoidsOnly,
    OpenWorldOnly,
    OptInOnly,
    IncludeEntries,
    ExcludeEntries,
    IncludeMaps,
    ExcludeMaps,
    DecisionIntervalMs,
    DecisionJitterMs,
    IndividualIntelligence,
    GroupIntelligence,
    CallForHelpEnable,
    CallForHelpMaxAllies,
    CallForHelpRadius,
    CallForHelpCooldownMs,
    RetreatEnable,
    RetreatHealthThresholdPct,
    RetreatCooldownMs,
    RetreatRadius,
    ProtectAllyEnable,
    RepositionEnable,
    MaxDecisionsPerTick,
    Debug,
    Count
};

class ModCombatAIConfig final : public ConfigValueCache<ModCombatAIConfigKey>
{
public:
    ModCombatAIConfig();

    void BuildConfigCache() override;

    [[nodiscard]] bool Enabled() const { return GetConfigValue<bool>(ModCombatAIConfigKey::Enable); }
    [[nodiscard]] bool HumanoidsOnly() const { return GetConfigValue<bool>(ModCombatAIConfigKey::HumanoidsOnly); }
    [[nodiscard]] bool OpenWorldOnly() const { return GetConfigValue<bool>(ModCombatAIConfigKey::OpenWorldOnly); }
    [[nodiscard]] bool OptInOnly() const { return GetConfigValue<bool>(ModCombatAIConfigKey::OptInOnly); }
    [[nodiscard]] uint32 DecisionIntervalMs() const { return GetConfigValue<uint32>(ModCombatAIConfigKey::DecisionIntervalMs); }
    [[nodiscard]] uint32 DecisionJitterMs() const { return GetConfigValue<uint32>(ModCombatAIConfigKey::DecisionJitterMs); }
    [[nodiscard]] float IndividualIntelligence() const { return GetConfigValue<float>(ModCombatAIConfigKey::IndividualIntelligence); }
    [[nodiscard]] float GroupIntelligence() const { return GetConfigValue<float>(ModCombatAIConfigKey::GroupIntelligence); }
    [[nodiscard]] bool CallForHelpEnabled() const { return GetConfigValue<bool>(ModCombatAIConfigKey::CallForHelpEnable); }
    [[nodiscard]] uint32 CallForHelpMaxAllies() const { return GetConfigValue<uint32>(ModCombatAIConfigKey::CallForHelpMaxAllies); }
    [[nodiscard]] float CallForHelpRadius() const { return GetConfigValue<float>(ModCombatAIConfigKey::CallForHelpRadius); }
    [[nodiscard]] uint32 CallForHelpCooldownMs() const { return GetConfigValue<uint32>(ModCombatAIConfigKey::CallForHelpCooldownMs); }
    [[nodiscard]] bool RetreatEnabled() const { return GetConfigValue<bool>(ModCombatAIConfigKey::RetreatEnable); }
    [[nodiscard]] uint32 RetreatHealthThresholdPct() const { return GetConfigValue<uint32>(ModCombatAIConfigKey::RetreatHealthThresholdPct); }
    [[nodiscard]] uint32 RetreatCooldownMs() const { return GetConfigValue<uint32>(ModCombatAIConfigKey::RetreatCooldownMs); }
    [[nodiscard]] float RetreatRadius() const { return GetConfigValue<float>(ModCombatAIConfigKey::RetreatRadius); }
    [[nodiscard]] bool ProtectAllyEnabled() const { return GetConfigValue<bool>(ModCombatAIConfigKey::ProtectAllyEnable); }
    [[nodiscard]] bool RepositionEnabled() const { return GetConfigValue<bool>(ModCombatAIConfigKey::RepositionEnable); }
    [[nodiscard]] uint32 MaxDecisionsPerTick() const { return GetConfigValue<uint32>(ModCombatAIConfigKey::MaxDecisionsPerTick); }
    [[nodiscard]] bool Debug() const { return GetConfigValue<bool>(ModCombatAIConfigKey::Debug); }

    [[nodiscard]] bool IsEntryIncluded(uint32 entry) const { return _includeEntries.contains(entry); }
    [[nodiscard]] bool IsEntryExcluded(uint32 entry) const { return _excludeEntries.contains(entry); }
    [[nodiscard]] bool IsMapIncluded(uint32 mapId) const { return _includeMaps.contains(mapId); }
    [[nodiscard]] bool IsMapExcluded(uint32 mapId) const { return _excludeMaps.contains(mapId); }

private:
    static std::unordered_set<uint32> ParseIdSet(std::string_view value);
    void RebuildIdSets();

    std::unordered_set<uint32> _includeEntries;
    std::unordered_set<uint32> _excludeEntries;
    std::unordered_set<uint32> _includeMaps;
    std::unordered_set<uint32> _excludeMaps;
};

extern ModCombatAIConfig g_ModCombatAIConfig;
