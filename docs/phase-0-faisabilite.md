# Phase 0 — Faisabilité du module mod-combat-ai

- Dépôt : `azerothcore-wotlk`, branche WotLK 3.3.5a.
- Branche / HEAD : `master` / `011a0fef485ac399585ae85e81c424936a6fed71`.
- Audit du 2026-10-25, en lecture seule : aucun build, aucune modification du noyau.
- Périmètre : ce rapport uniquement ; `modules/mod-combat-ai/` ne contient pas de `src/`.

## 1. Résumé exécutif

**Verdict : faisable sans aucune modification du noyau.** L'intégration repose sur un enrichissement externe : `AllCreatureScript::OnAllCreatureUpdate` (`src/server/game/Scripting/ScriptDefines/AllCreatureScript.h:30`), appelé en fin de `Creature::Update` (`src/server/game/Entities/Creature/Creature.cpp:970`), complété par `OnCreatureAddWorld` / `OnCreatureRemoveWorld` (`AllCreatureScript.h:43,50`) et par les transitions `UnitScript` (`ScriptDefines/UnitScript.h:108-111`).

Le module n'implémente pas `GetCreatureAI` : ce hook retourne la première IA non nulle (`src/server/game/Scripting/ScriptMgrMacros.h:38-53`) et remplacerait SmartAI ou les scripts existants. Il n'ajoute aucune statistique ni menace artificielle, et n'appelle pas `CallForHelp` (sans plafond ni cooldown, `Creature.cpp:2544,2559`) : il exécute au plus une action native par décision, après un scan borné réutilisant les prédicats natifs (`src/server/game/Grids/Notifiers/GridNotifiers.h:1182-1195`).

La protection par défaut (`OptInOnly = 1` avec `IncludeEntries` vide) garantit zéro créature affectée tant qu'aucune entrée n'est listée. Les limitations L1–L10 sont documentées avec alternatives ; tout point non prouvé est marqué « à vérifier en Phase 2/3 ».

**Phase 1 prête à démarrer après validation de l'architecture.** Aucun prérequis core ne manque ; il faudra reconfigurer CMake après création de `src/`, car les sources des modules sont globbées au configure.

Aucune dépendance réseau, IA générative ou service payant n'est introduite : tous les calculs restent locaux au serveur.

## 2. Environnement audité

- Dépôt `azerothcore-wotlk`, branche `master`, HEAD `011a0fef485ac399585ae85e81c424936a6fed71` (2026-10-25). HEAD non taggé ; l'audit porte sur ce HEAD exact.
- Six modules installés, clones git indépendants, sans `.gitmodules` ni `CMakeLists.txt` par module :

| module | remote | HEAD court | symbole loader |
|---|---|---|---|
| mod-ah-bot-plus | https://github.com/NathanHandley/mod-ah-bot-plus.git | f685832 (detached) | `Addmod_ah_bot_plusScripts()` (`src/ah_bot_loader.cpp:11`) |
| mod-challenge-modes | https://github.com/ZhengPeiRu21/mod-challenge-modes.git | 4c5444b (local-adaptations) | `Addmod_challenge_modesScripts()` (`src/ChallengeModes_loader.cpp:11`) |
| mod-hunger-thirst | https://github.com/Phanty78/mod-hunger-thirst.git | 9479227 (main) | `Addmod_hunger_thirstScripts()` (`src/mod_hunger_thirst_loader.cpp:13`) |
| mod-hunts | https://github.com/Hisha/mod-hunts | 417ef2d (main) | `Addmod_huntsScripts()` (`src/mod_hunts_loader.cpp:4`) |
| mod-living-world | https://github.com/Hisha/mod-living-world | 116926e (main) | `Addmod_living_worldScripts()` (`src/mod_living_world_loader.cpp:3`) |
| mod-weather-vibe | https://github.com/hermensbas/mod_weather_vibe | cb854eb (main) | `Addmod_weather_vibeScripts()` (`src/mod_weather_vibe.cpp:64`) |

- `var/build/` est un arbre de build périmé : `var/build/CMakeCache.txt:312` conserve `MODULES:STRING=static` et `:315` une entrée stale `MODULE_MOD-AUTOBALANCE:STRING=default` alors que le module n'est pas installé. Le loader généré `var/build/modules/gen_scriptloader/static/ModulesLoader.cpp:49-54` contient un bloc d'invocations vide (aucun module configuré). Une reconfiguration CMake sera nécessaire en Phase 1.
- Linkage statique dans cet environnement : `conf/dist/config.cmake:20` définit `MODULES=static` par défaut et le worldserver câble directement `AddModulesScripts` (`src/server/apps/worldserver/Main.cpp:272-273`). `DISABLED_AC_MODULES` ne gouverne que l'ancienne boucle `add_subdirectory` (`CMakeLists.txt:72-85`) ; les six modules n'ayant pas de `CMakeLists.txt`, cet interrupteur est inopérant. Le vrai levier est `-DMODULE_MOD-COMBAT-AI=disabled` ou `-DMODULES=none`.
- `modules/` est gitignoré (`.gitignore:8-13`) sauf `*.md`, `*.sh`, `CMakeLists.txt`, `*.h`, `*.cmake` à sa racine : les sources du futur module resteront invisibles au `git status` du core.
- Conventions de configuration : `modules/CMakeLists.txt:330-364` collecte les `conf/*.conf.dist`, `ConfigInstall.cmake:101` les installe sous `${CONF_DIR}/modules`, et le runtime lit `GetConfigPath() + "modules/"` (`Config.cpp:751-773`). La liste runtime est dérivée sans `.dist` (`modules/CMakeLists.txt:349-351`) : l'exploitation doit fournir le `.conf` correspondant.
- Aucun `CMakeLists.txt` ni `<module>/<module>.cmake` n'est requis ; un include optionnel `<module>/<module>.cmake` existe (`modules/CMakeLists.txt:301-303`). Le futur module suivra ce schéma et n'ajoutera aucun symbole loader en double.
- Conséquence pour la Phase 1 : tant que `src/` n'existe pas, le module est invisible pour CMake (`ConfigureModules.cmake:19-22,34-44`) ; après création, il faudra relancer CMake et vérifier le loader généré avant tout test activé/désactivé.

**Écarts corrigés lors de la re-vérification** (le plan initial était imprécis sur ces points) :

- `Main.cpp` est `src/server/apps/worldserver/Main.cpp:272-273` ; `src/server/game/Main.cpp` n'existe pas.
- `DoFleeToGetAssistance` est implémenté dans `Creature.cpp:1090-1118`, pas dans `PointMovementGenerator.cpp`. Les lignes `PointMovementGenerator.cpp:353-358` correspondent à `AssistanceMovementGenerator::Finalize`, qui appelle `CallAssistance()` à l'arrivée.
- `SetCombatMovement(false)` ne stoppe pas une chasse active : le commentaire `Creature.h:450-454` indique que l'appel ne fait rien en combat et n'affecte que `AttackStart`. Il n'est donc pas le mécanisme d'interruption de la fuite.
- `mod-hunts` n'est pas neutre au combat : il implémente deux `AllCreatureScript`, un `GetCreatureAI` pour les proies d'élite et écrit lui-même mouvement et engagement.

## 3. Mécanismes du core mobilisables

Toutes les ancres ci-dessous ont été relues au HEAD indiqué ; elles proviennent du code présent, pas d'une supposition d'API.

| symbole | chemin:ligne | usage prévu |
|---|---|---|
| `AllCreatureScript::OnAllCreatureUpdate` | `src/server/game/Scripting/ScriptDefines/AllCreatureScript.h:30` ; `src/server/game/Entities/Creature/Creature.cpp:970` | Tick externe post-IA, throttlé par le module ; point d'entrée principal. |
| `AllCreatureScript::OnCreatureAddWorld` | `AllCreatureScript.h:43` ; `Creature.cpp:338` | Initialisation du contexte à l'entrée dans le monde. |
| `AllCreatureScript::OnCreatureRemoveWorld` | `AllCreatureScript.h:50` ; `Creature.cpp:346` | Libération garantie de l'état au despawn. |
| `AllCreatureScript::GetCreatureAI` | `AllCreatureScript.h:101` ; `ScriptDefines/CreatureScript.cpp:156-171` ; `ScriptMgrMacros.h:38-53` | Écarté : premier non-nul, il remplacerait l'IA native. |
| `CreatureAIRegistry` / `CreatureAISelector::SelectAI` | `src/server/game/AI/CreatureAIRegistry.cpp:45,49` ; `CreatureAISelector.cpp:78-88` | Comprendre l'ordre script → AIName et la collision avec `"CombatAI"`. |
| `UnitScript::OnUnitEnterCombat` / `ExitCombat` / `Death` / `EnterEvadeMode` | `ScriptDefines/UnitScript.h:108-111` | Création, reset et destruction du contexte. |
| `WorldScript::OnAfterConfigLoad` | `ScriptDefines/WorldScript.h:53` ; `World/World.cpp:302` | Rechargement de configuration via `.reload config`. |
| `Creature::CallForHelp` | `Creature.h:272` ; `Creature.cpp:2544,2559` | Étudié mais non appelé : ni plafond d'alliés ni cooldown. |
| `Creature::CallAssistance` | `Creature.h:273` ; `Creature.cpp:2508` | Assistance native non retenue : elle n'est pas ciblée vers un allié choisi par le module. |
| `Creature::HasSearchedAssistance` | `Creature.h:277` | État natif d'assistance déjà recherchée ; lecture pour éviter les doublons. |
| `Unit::RespondsToCallForHelp` | `Unit.h:1047` | Distingue l'assistance de faction ; `CanAssistTo(false)` utilise `IsFriendlyTo` à la place. |
| `Creature::DoFleeToGetAssistance` | `Creature.h:271` ; `Creature.cpp:1090-1118` | Étudié mais non retenu : sans allié, applique `SetControlled(..., UNIT_STATE_FLEEING)`. |
| `Creature::CanAssistTo` | `Creature.h:278` ; `Creature.cpp:2567-2628` | Prédicat d'éligibilité d'un allié (react state, faction, détection de l'ennemi). |
| `Acore::CallOfHelpCreatureInRangeDo` | `GridNotifiers.h:1182-1195` | Modèle du scan borné : `CanAssistTo(..., false)`, distance, `IsWithinLOSInMap`, `SetNoCallForHelp`, `EngageWithTarget`. |
| `Creature::SetNoCallForHelp` | `Creature.h:274` | Anti-chaîne pendant le scan et l'engagement. |
| `Unit::EngageWithTarget` | `src/server/game/Entities/Unit/Unit.h:944` | Engagement de l'allié sur la cible réelle. |
| `Creature::IsEngaged` | `Creature.h:324` ; `Creature.cpp:2975` | Gate O(1) : la créature est réellement en combat. |
| `MotionMaster::MovePoint` | `src/server/game/Movement/MotionMaster.h:241-243` ; `MotionMaster.cpp:487-501` | Fuite vers un allié ; remplace le générateur du slot actif. |
| `UnitAI::AttackStart` | `src/server/game/AI/CoreAI/UnitAI.cpp:29-33` | Ré-émission native de `MoveChase` ; ne consulte pas `SetCombatMovement`. |
| `CreatureAI::UpdateVictim` | `src/server/game/AI/CreatureAI.cpp:350-381` | Ré-émet `AttackStart` seulement si la victime change. |
| `SmartAI::AttackStart` | `src/server/game/AI/SmartScripts/SmartAI.cpp:947-970` | Peut expirer un POINT et relancer `MoveChase` au changement de cible. |
| `MotionMaster::MoveFleeing` | `MotionMaster.h:240` ; `MotionMaster.cpp:901` | Repli local sans allié accessible (comportement à valider en Phase 3). |
| `MotionMaster::MoveSeekAssistance` | `MotionMaster.h:264` ; `MotionMaster.cpp:861-878` | Écarté : `AttackStop`, `CastStop`, `REACT_PASSIVE` avant le déplacement. |
| `MotionMaster::GetCurrentMovementGeneratorType` | `MotionMaster.h:273` ; `MotionMaster.cpp:1088-1095` | Watchdog : détection d'un mouvement remplacé ou terminé. |
| `ThreatManager::GetSortedThreatList` | `src/server/game/Combat/ThreatManager.h:131` | Lecture de la cible à menace maximale ; le module n'ajoute jamais de menace. |
| `ThreatManager::GetCurrentVictim` | `ThreatManager.h:111` | Sémantique native (fixate/taunt/hystérésis), pas un maximum brut. |
| `Creature::m_spells` / `m_CreatureSpellCooldowns` | `Creature.h:257-258` (publics) | Inventaire réel des sorts disponibles. |
| `Creature::HasSpell` / `HasSpellCooldown` / `GetSpellCooldown` | `Creature.h:168-174` | Filtrage des sorts et respect des cooldowns natifs. |
| `sSpellMgr->GetSpellInfo` / `SpellInfo::Effects` / `HasEffect` | `src/server/game/Spells/SpellMgr.h:745` ; `SpellInfo.h:417,434` | Détection d'un soin réel (`SPELL_EFFECT_HEAL`, cf. `Creature.cpp:2433`). |
| `Creature::GetCreatureType` / `CreatureTemplate::type` | `Unit.h:830` ; `Unit.cpp:11568` ; `CreatureData.h:218` ; `Globals/ObjectMgr.cpp:1151-1155` | Filtre humanoïde (`CREATURE_TYPE_HUMANOID = 7`, `src/server/shared/SharedDefines.h:2628`). |
| `Creature::isWorldBoss` / `IsDungeonBoss` | `Creature.h:124,132` ; `Creature.cpp:3368` | Exclusions boss mondiaux et boss de donjon. |
| `Creature::GetScriptName` / `GetAIName` / `GetScriptId` | `Creature.h:216-218` ; `Creature.cpp:3233-3264` | Exclusions des créatures scriptées ; lecture seule. |
| Validation d'`AIName` | `Globals/ObjectMgr.cpp:1080-1084` | Un AIName inconnu est vidé ; une créature sans IA scriptée retombe sur la factory. |
| `Map::IsWorldMap` / `IsDungeon` / `IsRaid` / `IsBattlegroundOrArena` | `src/server/game/Maps/Map.h:298-308` | Restriction au monde ouvert. |
| `IsWithinLOSInMap` / `GetPhaseMask` | `src/server/game/Entities/Object/Object.h:558,523` | Signatures présentes ; sémantique LOS/phase à vérifier en Phase 2. |
| `Creature::SetCombatMovement` | `Creature.h:455-456` ; `Creature.cpp:3987` | Usage limité ; n'arrête pas une chasse active (voir L2). |
| `ConfigValueCache` | `src/common/Configuration/ConfigValueCache.h:28-123` ; `modules/mod-hunts/src/HuntsModule.cpp:68-136` | Convention de configuration et de rechargement. |
| Symbole loader `Addmod_combat_aiScripts()` | `modules/CMakeLists.txt:150-166` | Point d'entrée des scripts, dérivé du nom de dossier (`-` → `_`). |
| Détection d'un module | `src/cmake/macros/ConfigureModules.cmake:19-22,34-44` | Sans sous-dossier `src/`, le module est invisible pour CMake. |
| Installation configuration | `modules/CMakeLists.txt:330-364` ; `ConfigInstall.cmake:101` ; `Config.cpp:751-773` | `conf/*.conf.dist` installé, lu depuis `GetConfigPath() + "modules/"`. |

## 4. Limitations L1–L10 et alternatives

Chaque limitation ci-dessous est accompagnée de l'alternative retenue ; aucune ne nécessite de patch du noyau.

- **L1 — Pas de hook « avant décision IA ».** Impossible d'intercepter la sélection de cible sans remplacer l'IA, et `GetCreatureAI` est non additif (`ScriptMgrMacros.h:38-53`). Alternative : tick externe `OnAllCreatureUpdate` et ordres natifs ; l'IA d'origine continue de tourner.
- **L2 — Conflit mouvement module / `MoveChase` natif.** `SetCombatMovement(false)` ne stoppe pas une chasse active (`Creature.h:450-454`) et `UnitAI::AttackStart` ne le consulte pas (`src/server/game/AI/CoreAI/UnitAI.cpp:29-33`). `MovePoint` mute le slot actif (`MotionMaster.cpp:487-501`) ; l'IA native ne ré-émet `MoveChase` qu'au changement de victime (`CreatureAI.cpp:350-381`) ; SmartAI peut écraser un POINT au changement de cible (`SmartAI.cpp:947-970`). Alternative : `MovePoint` + watchdog de position (aucune progression pendant N ms → abandon et repli natif). Ne pas toucher `UNIT_STATE_NO_COMBAT_MOVEMENT` (`UnitDefines.h:204`).
- **L3 — Pas de `MovementInform` côté module.** Alternative : polling borné via `GetCurrentMovementGeneratorType()` et la position, dans le tick déjà throttlé ; aucune requête de chemin supplémentaire.
- **L4 — `CallForHelp` sans plafond ni cooldown.** La garde native `m_alreadyCallForHelp` (`Creature.cpp:2559`) est positionnée sur chaque assistant et n'est que transitoire. Alternative : scan borné réutilisant `CanAssistTo(..., false)` + distance + `IsWithinLOSInMap` + `SetNoCallForHelp` + `EngageWithTarget` (`GridNotifiers.h:1182-1195`), avec cap `MaxAllies` et cooldowns par créature et par groupe local.
- **L5 — Pas de stockage d'état par créature côté core.** Alternative : map module `ObjectGuid → CombatContext`, durée de vie = combat, nettoyage sur `OnUnitDeath`, `OnUnitEnterEvadeMode`, `OnUnitExitCombat` et `OnCreatureRemoveWorld` en filet de sécurité.
- **L6 — Sorts réels uniquement.** `m_spells` et `m_CreatureSpellCooldowns` sont publics (`Creature.h:257-258`), mais il n'existe pas de `GetSpell(uint32)`. Alternative : `HasSpell`, `m_spells`, `HasSpellCooldown`, `GetSpellCooldown`, et validation du soin via `SpellInfo::Effects` (`SpellInfo.h:417`) / `SPELL_EFFECT_HEAL` (`Creature.cpp:2433`). Aucune capacité inventée.
- **L7 — Nom global déjà pris.** La classe core `CombatAI` (`src/server/game/AI/CoreAI/CombatAI.h:39`) et l'AIName `"CombatAI"` (`CreatureAIRegistry.cpp:45`) interdisent ce nom. Alternative : namespace `mod_combat_ai`, classes `ModCombatAI*` ; les clés de configuration sont des chaînes et restent `CombatAI.*`.
- **L8 — `DISABLED_AC_MODULES` inopérant ; module non détecté sans `src/`.** Alternative : variable cache `-DMODULE_MOD-COMBAT-AI=disabled` ou `-DMODULES=none` ; reconfiguration CMake obligatoire après création de `src/` ; l'arbre `var/build` actuel est périmé.
- **L9 — Créatures instanciées, scriptées ou boss.** Exclues par `Map::IsWorldMap()` (`Map.h:298-308`), `ScriptName` non vide (`Creature.h:217`), `isWorldBoss()` / `IsDungeonBoss()` (`Creature.h:124,132`). Précision : `ScriptName` non vide exclut les IA scriptées enregistrées (`CreatureAISelector.cpp:78-88`, `CreatureScript.cpp:156-171`), **pas** les créatures SmartAI ordinaires (AIName `SmartAI`, ScriptName vide). Le mode opt-in les protège par défaut.
- **L10 — LOS et phase.** Les signatures existent (`Object.h:558,523`), mais leur sémantique exacte reste **à vérifier en Phase 2**. `CanAssistTo` ne contrôle ni la distance, ni la LOS, ni la phase (`Creature.cpp:2567-2628`) : la distance et la LOS sont gérées par le notifier natif (`GridNotifiers.h:1182-1195`), la phase restant à valider en Phase 2.

## 5. Architecture proposée

**Fichiers de la Phase 1** (le dossier `src/` est obligatoire pour que CMake détecte le module, `ConfigureModules.cmake:19-22,34-44`) :

- `src/mod_combat_ai_loader.cpp` : `void Addmod_combat_aiScripts()` enregistre les objets de script.
- `src/ModCombatAIWorld.h/.cpp` : `WorldScript`, `ConfigValueCache` et `OnAfterConfigLoad`.
- `src/ModCombatAICreature.h/.cpp` : `AllCreatureScript`, `OnAllCreatureUpdate` et cycle de vie monde.
- `src/ModCombatAIUnit.h/.cpp` : `UnitScript`, transitions combat, mort et evade.
- `src/ModCombatAIConfig.h` : clés et valeurs par défaut.
- Phases 2 à 5 : `EligibilityPolicy`, `CombatContext`, `DecisionEngine`, `Tactics/CallForHelp`, `Tactics/Retreat`, `Tactics/ProtectAlly`, `Tactics/Reposition`, `Coordinator`, `Diagnostics`.

**Responsabilités des composants.**

- `EligibilityPolicy` : humanoïde, monde ouvert, exclusions ; résultat mis en cache par entrée et par carte.
- `CombatContext` : instantané borné (vie, rôle, alliés proches, menace, cooldowns), recalculé seulement à la décision.
- `DecisionEngine` : scores des quatre tactiques, poids et jitter, sans stratégie globale ni état permanent.
- `Coordinator` : quotas par groupe local pour éviter que tous les combattants changent de cible simultanément.
- `Diagnostics` : compteurs décisions/actions/échecs et logs désactivés par défaut.

**Flux de décision.** Une gate O(1) est évaluée en premier dans chaque hook : `CombatAI.Enable`, créature engagée, éligibilité mise en cache à l'engagement, intervalle `DecisionIntervalMs` et jitter `DecisionJitterMs`. Le contexte est ensuite borné : un seul scan d'alliés par décision, rayon limité, au plus `MaxAllies + 3` unités inspectées. `DecisionEngine` note au plus quatre tactiques (poids × `IndividualIntelligence`, facteur aléatoire modéré, budgets par groupe via `Coordinator`), puis le module exécute **au plus une action native** par décision. Les cooldowns sont suivis par créature et par groupe local.

Aucun pathfinding n'est exécuté par tick : `MovePoint` n'est demandé qu'à l'instant d'une tentative de fuite, au plus une requête par tentative. Le watchdog lit ensuite type de mouvement et position sans nouvelle requête. `CombatAI.Enable = 0` constitue l'interrupteur immédiat : le premier test de chaque hook retourne avant tout travail et le repli vers l'IA native est intégral.

**Non-objectifs explicites.** Le module n'implémente pas `GetCreatureAI`, n'appelle pas `CallForHelp`, n'appelle jamais `AddThreat` ni `ModifyThreatByPercent`, ne modifie ni statistiques ni sorts, et ne remplace aucune action SmartAI. Il ajoute des ordres natifs que l'IA d'origine peut ignorer ou écraser, d'où le watchdog et le repli natif.

**Configuration** — bloc de départ, 22 clés, valeurs du cahier §6 complétées par les listes d'exclusion et le bornage de charge §7. Ces valeurs sont des points de départ pour tests, pas des valeurs validées :

```
CombatAI.Enable = 0
CombatAI.HumanoidsOnly = 1
CombatAI.OpenWorldOnly = 1
CombatAI.OptInOnly = 1
CombatAI.IncludeEntries = ""
CombatAI.ExcludeEntries = ""
CombatAI.IncludeMaps = ""
CombatAI.ExcludeMaps = ""
CombatAI.DecisionIntervalMs = 1000
CombatAI.DecisionJitterMs = 250
CombatAI.IndividualIntelligence = 0.5
CombatAI.GroupIntelligence = 0.5
CombatAI.CallForHelp.Enable = 1
CombatAI.CallForHelp.MaxAllies = 2
CombatAI.CallForHelp.Radius = 15
CombatAI.CallForHelp.CooldownMs = 30000
CombatAI.Retreat.Enable = 1
CombatAI.Retreat.HealthThresholdPct = 20
CombatAI.Retreat.CooldownMs = 60000
CombatAI.ProtectAlly.Enable = 1
CombatAI.MaxDecisionsPerTick = 64
CombatAI.Debug = 0
```

**Cycle de vie et rechargement.** Le contexte est créé à `OnUnitEnterCombat`, réinitialisé à l'evade, détruit à la mort, à la fin de combat ou au despawn (`OnCreatureRemoveWorld` en filet). Un `.reload config` ne fait que relire les options via `OnAfterConfigLoad` : aucun état de combat n'est reconstruit, ce qui couvre les critères de reset après reload et redémarrage.

## 6. Compatibilité et conflits

- **SmartAI.** Nuance essentielle : le module n'implémente jamais `GetCreatureAI`. Une créature SmartAI ordinaire a un `ScriptName` vide et reste donc éligible si elle est explicitement opt-in ; son IA n'est pas remplacée. Ses actions natives `SMART_ACTION_CALL_FOR_HELP` (`SmartScriptMgr.h:581` ; `SmartScript.cpp:1358-1375`) et `SMART_ACTION_FLEE_FOR_ASSIST` (`SmartScript.cpp:1060-1073`) peuvent toutefois se superposer aux ordres du module : cooldowns et opt-in sont la parade.
- **mod-hunts.** Deux `AllCreatureScript` (`modules/mod-hunts/src/HuntScripts.cpp:533,655`), un `GetCreatureAI` limité aux proies d'élite (`modules/mod-hunts/src/HuntScripts.cpp:660-666`), des `TempSummon` sans `AIName` ni `ScriptName` (`modules/mod-hunts/src/HuntCreatureTemplateManager.cpp:388,397`) et des écritures directes de mouvement/engagement (`modules/mod-hunts/src/HuntManager.cpp:3098,3192,3205,3317,3554,3562` pour `MoveChase` ; `:3333,3467` pour `MovePoint` ; `:3074,3191,3204,3316` pour `AttackStart`). Ce module n'est pas neutre au combat : ses entrées doivent rester hors opt-in.
- **mod-living-world.** Spawn de `TempSummon` (`modules/mod-living-world/src/core/CreatureProvider.cpp:150`), templates matérialisés avec `AIName` et `ScriptName` vides (`modules/mod-living-world/src/core/LwCreatureTemplateManager.cpp:386,393`), déplacements `MoveSplinePath` (`modules/mod-living-world/src/core/AssaultManager.cpp:471` ; `modules/mod-living-world/src/core/MovementController.cpp:1312,1523,1609,1721`) et `AttackStart` (`modules/mod-living-world/src/core/AssaultManager.cpp:1048`). Ses créatures doivent aussi être exclues explicitement si l'opt-in est élargi.
- **Autres modules.** `mod-hunger-thirst` n'a qu'un `UnitScript` limité à `UNITHOOK_ON_AURA_APPLY` (`modules/mod-hunger-thirst/src/HungerThirst_Unit.cpp:19-32`) ; `mod-challenge-modes`, `mod-weather-vibe` et `mod-ah-bot-plus` n'exposent aucun hook créature/unité audité. Aucun module installé n'utilise `CallForHelp`, `ThreatManager` ou `SetCombatMovement`, et aucun n'implémente `OnAllCreatureUpdate`.
- **Additivité.** `AllCreatureScript` et `UnitScript` sont dispatchés à tous les scripts (`ScriptMgrMacros.h:56-65,72-74`) : le module s'ajoute sans masquer les autres. Le seul hook non additif est `GetCreatureAI`, volontairement non implémenté.
- **Collision de noms.** `CombatAI` existe comme classe core (`CombatAI.h:39`) et comme AIName (`CreatureAIRegistry.cpp:45`). Le module utilise le namespace `mod_combat_ai` et le préfixe `ModCombatAI*` ; aucun symbole `ModCombatAI` / `mod_combat_ai` n'existe aujourd'hui.
- **Modules absents.** autobalance et playerbots ne figurent pas dans ce checkout ; leur compatibilité devra être retestée s'ils sont ajoutés, sans présumer de leur présence (cahier §2).
- **Symbole loader.** `Addmod_combat_aiScripts()` est unique et dérivé du nom de dossier (`modules/CMakeLists.txt:150-166`) ; il n'entre en collision avec aucun des six loaders installés.
- **Protection par défaut.** `OptInOnly = 1` avec `IncludeEntries` vide signifie aucune créature affectée. Tout élargissement exige des exclusions explicites par entrée ou par carte pour les proies de mod-hunts et les summons de mod-living-world, qui n'ont pas de `ScriptName`.

## 7. Mapping des phases 1 à 6

Correspondance avec le cahier §8 ; les critères d'acceptation du cahier §9 sont cités pour chaque phase.

- **Phase 1 — Squelette.** Dépôt installable, `src/`, symbole loader `Addmod_combat_aiScripts()`, `conf/mod_combat_ai.conf.dist`, `ConfigValueCache` + `OnAfterConfigLoad`, `EligibilityPolicy`, activation opt-in, filtres et exclusions, journalisation. Compiler et démarrer avec le module activé puis désactivé.
  Critères §9 : compilation et chargement sans modification du core ; désactivé, comportement inchangé ; une créature non éligible ou exclue conserve son IA native.
- **Phase 2 — Appel à l'aide.** Scan borné avec `CanAssistTo(..., false)`, distance, `IsWithinLOSInMap`, `SetNoCallForHelp`, `EngageWithTarget` ; plafond `MaxAllies`, cooldowns créature et groupe, pas d'appel récursif. Les contrôles LOS/phase sont à vérifier en Phase 2 avant validation.
  Critères §9 : test sur un camp humanoïde dédié, renforts limités, absence d'aggro en chaîne, respect des limites d'aggro.
- **Phase 3 — Fuite.** Destination vers un allié accessible via `MovePoint` avec pathfinding natif, **pas** `MoveSeekAssistance` (dont les effets de bord sont `AttackStop`, `CastStop`, `REACT_PASSIVE`, `MotionMaster.cpp:861-878`) ; repli local via `MoveFleeing` ou comportement natif ; watchdog de position ; gestion obstacles, leash, evade, mort et reset. Le comportement exact du repli et du leash est à valider en Phase 3.
  Critères §9 : la fuite privilégie des alliés accessibles et ne bloque pas la créature sans chemin.
- **Phase 4 — Coopération.** `ProtectAlly` pour les alliés vulnérables, notamment les soigneurs, avec sorts réellement présents dans `m_spells`, filtrage `HasSpell`/cooldowns, détection de soin par `SpellInfo::Effects` / `SPELL_EFFECT_HEAL` ; lecture de la cible à menace maximale via `GetSortedThreatList()`, sans ajout artificiel de menace.
  Critères §9 : aucun sort inventé, respect de la menace, des immunités, des contrôles, des cooldowns et des scripts.
- **Phase 5 — Moteur de décision.** Scores pondérés des quatre tactiques, `IndividualIntelligence` / `GroupIntelligence`, intervalle et jitter, budgets du `Coordinator`, `MaxDecisionsPerTick`, diagnostics désactivés par défaut.
  Critères §9 : combats variés mais lisibles en solo, duo et groupe de cinq à six ; impact mesuré, sans seuil annoncé avant mesure.
- **Phase 6 — Généralisation.** Déploiement progressif par entrées et cartes (`IncludeEntries`, `IncludeMaps`, `ExcludeEntries`, `ExcludeMaps`), benchmarks reproductibles, tests de non-régression et interactions avec mod-hunts et mod-living-world. L'extension aux bêtes, morts-vivants et autres types reste une phase future distincte.
  Critères §9 : tests de charge avec des dizaines/centaines de créatures, documentation de l'impact réel, reset après mort, evade, désengagement, reload et redémarrage.

## 8. Checklist d'acceptation Phase 0

- [x] Module audité sans aucune modification du noyau.
- [x] Aucune API supposée sans vérification : les signatures sont relevées au HEAD ; les sémantiques non prouvées sont marquées « à vérifier en Phase 2/3 ».
- [x] Hooks et APIs réellement présents dans la version installée (`011a0fef4`).
- [x] Limitations L1–L10 documentées, chacune avec une alternative compatible.
- [x] Exclusions définies : `ScriptName` non vide, boss mondiaux et de donjon, instances, BG/arène, mode opt-in.
- [x] Rapport livré (`modules/mod-combat-ai/docs/phase-0-faisabilite.md`), sans code, sans build et sans modification du dépôt.
- [x] Aucune modification hors de ce rapport : `modules/mod-combat-ai/` ne contient que `docs/`, aucun `src/`, et le noyau est intact.
