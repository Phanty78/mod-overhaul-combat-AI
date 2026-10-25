# mod-combat-ai

**État : Phase 1 — squelette (observation seule, aucune tactique).**

Module AzerothCore pour WotLK 3.3.5a. Il ajoute une couche de décision de combat
externe aux créatures, sans remplacer leur IA native. La Phase 1 valide uniquement
l'intégration : chargement du module, configuration, politique d'éligibilité, hooks
de cycle de vie, limitation de charge et journalisation. Aucune action de combat
n'est exécutée.

Le module est **désactivé par défaut** (`CombatAI.Enable = 0`) et fonctionne en
**opt-in strict** (`CombatAI.OptInOnly = 1`). Avec la configuration livrée, aucune
créature n'est affectée.

Le rapport d'audit et les limitations détaillées sont dans
[`docs/phase-0-faisabilite.md`](docs/phase-0-faisabilite.md).

## Installation

1. Placer le module dans `modules/mod-combat-ai/`. Le dossier `src/` doit exister :
   c'est lui qui rend le module visible pour CMake.
2. Reconfigurer CMake, opération obligatoire. Les sources des modules sont
   collectées par glob au moment du configure ; un simple build ne détecte pas un
   module ou un fichier source ajouté après la dernière configuration. Aucun
   `CMakeLists.txt` de module n'est nécessaire.
3. Compiler la cible `worldserver` (voir la section Build).
4. Copier la configuration distribuée vers le fichier lu au runtime :

   ```sh
   cp modules/mod-combat-ai/conf/mod_combat_ai.conf.dist \
      env/dist/etc/modules/mod_combat_ai.conf
   ```

   Le chemin `env/dist/etc` correspond à l'installation standard de ce dépôt
   (`GetConfigPath()`). Le fichier `.dist` est installé par CMake sous
   `<conf>/modules/`, mais le runtime lit le fichier sans suffixe `.dist`.
5. Démarrer le serveur. Le chargement écrit dans `server.loading` :

   ```text
   mod-combat-ai: disabled (phase 1 skeleton, opt-in only)
   ```

6. Pour activer le module, éditer `env/dist/etc/modules/mod_combat_ai.conf` :

   ```ini
   CombatAI.Enable = 1
   CombatAI.OptInOnly = 1
   CombatAI.IncludeEntries = "1234,5678"
   ```

   Remplacer les identifiants par des entrées de créature réelles. Une entrée
   `CombatAI.IncludeMaps` peut aussi servir d'opt-in, mais elle couvre **toutes**
   les créatures éligibles de ces cartes : préférer `IncludeEntries` pour un
   déploiement progressif.

Aucune mise à jour SQL n'est requise en Phase 1.

## Configuration

Fichier : `env/dist/etc/modules/mod_combat_ai.conf` (`conf/mod_combat_ai.conf.dist`
comme modèle). Les 22 clés sont rechargées par `.reload config` via
`WorldScript::OnAfterConfigLoad`.

| Clé | Type | Défaut | Rôle et contrainte |
|---|---|---:|---|
| `CombatAI.Enable` | bool | `0` | Interrupteur principal. À `0`, chaque hook retourne immédiatement et l'IA native reste inchangée. |
| `CombatAI.HumanoidsOnly` | bool | `1` | Si `1`, seules les créatures de type humanoïde (`CREATURE_TYPE_HUMANOID`) sont éligibles. |
| `CombatAI.OpenWorldOnly` | bool | `1` | Si `1`, seules les cartes ouvertes (`Map::IsWorldMap()`) sont éligibles ; instances, raids, BG et arènes sont exclus. |
| `CombatAI.OptInOnly` | bool | `1` | Si `1`, la créature doit être autorisée par `IncludeEntries` ou `IncludeMaps`. |
| `CombatAI.IncludeEntries` | string (CSV) | `""` | Entrées de créature autorisées, séparées par des virgules. Vide avec `OptInOnly = 1` : aucune créature affectée. |
| `CombatAI.ExcludeEntries` | string (CSV) | `""` | Entrées toujours refusées. L'exclusion est prioritaire sur l'inclusion. |
| `CombatAI.IncludeMaps` | string (CSV) | `""` | Cartes (`mapId`) autorisées. En mode opt-in, une carte incluse autorise toutes ses créatures éligibles. Sans effet restrictif si `OptInOnly = 0`. |
| `CombatAI.ExcludeMaps` | string (CSV) | `""` | Cartes toujours refusées. L'exclusion est prioritaire sur l'inclusion. |
| `CombatAI.DecisionIntervalMs` | uint32 | `1000` | Intervalle minimal entre deux décisions d'une même créature, en ms. Doit être `> 0`. |
| `CombatAI.DecisionJitterMs` | uint32 | `250` | Jitter aléatoire ajouté à l'intervalle, en ms (`0` = aucun jitter), pour lisser la charge. |
| `CombatAI.IndividualIntelligence` | float | `0.5` | Poids de décision individuelle, entre `0.0` et `1.0`. Stocké pour les phases suivantes ; sans effet en Phase 1. |
| `CombatAI.GroupIntelligence` | float | `0.5` | Poids de coordination de groupe, entre `0.0` et `1.0`. Stocké pour les phases suivantes ; sans effet en Phase 1. |
| `CombatAI.CallForHelp.Enable` | bool | `1` | Option réservée à la Phase 2 ; aucune assistance n'est appelée en Phase 1. |
| `CombatAI.CallForHelp.MaxAllies` | uint32 | `2` | Plafond d'alliés, entre `0` et `10`. Stocké ; sans effet en Phase 1. |
| `CombatAI.CallForHelp.Radius` | float | `15` | Rayon de recherche en mètres. Doit être `> 0`. Stocké ; sans effet en Phase 1. |
| `CombatAI.CallForHelp.CooldownMs` | uint32 | `30000` | Cooldown d'appel à l'aide en ms. Stocké ; sans effet en Phase 1. |
| `CombatAI.Retreat.Enable` | bool | `1` | Option réservée à la Phase 3 ; aucune fuite n'est déclenchée en Phase 1. |
| `CombatAI.Retreat.HealthThresholdPct` | uint32 | `20` | Seuil de vie en pourcentage, entre `1` et `100`. Stocké ; sans effet en Phase 1. |
| `CombatAI.Retreat.CooldownMs` | uint32 | `60000` | Cooldown de retrait en ms. Stocké ; sans effet en Phase 1. |
| `CombatAI.ProtectAlly.Enable` | bool | `1` | Option réservée à la Phase 4 ; aucune protection n'est exécutée en Phase 1. |
| `CombatAI.MaxDecisionsPerTick` | uint32 | `64` | Budget global de décisions par tick du monde. Doit être `> 0`. Les décisions au-delà du budget sont différées. |
| `CombatAI.Debug` | bool | `0` | Si `1`, journalise dans `scripts.ai` l'entrée en combat éligible et chaque décision évaluée. |

Les listes CSV ignorent les espaces autour des valeurs et les jetons vides, non
numériques ou hors plage. Une valeur invalide pour une clé validée produit un
`LOG_ERROR` dans `server.loading` et la valeur par défaut est utilisée.

> Les valeurs livrées sont des **points de départ pour les tests**, pas des
> valeurs d'équilibrage validées. En particulier, `OptInOnly = 1` avec des listes
> d'inclusion vides signifie zéro créature affectée, et une inclusion par carte
> est beaucoup plus large qu'une inclusion par entrée.

## Comportement de la Phase 1

### Hooks enregistrés

Le symbole loader `Addmod_combat_aiScripts()` enregistre trois scripts :

| Script | Type | Hooks utilisés |
|---|---|---|
| `ModCombatAIWorldScript` | `WorldScript` | `OnAfterConfigLoad`, `OnUpdate` |
| `ModCombatAICreatureScript` | `AllCreatureScript` | `OnAllCreatureUpdate`, `OnCreatureAddWorld`, `OnCreatureRemoveWorld` |
| `ModCombatAIUnitScript` | `UnitScript` | `OnUnitEnterCombat`, `OnUnitExitCombat`, `OnUnitEnterEvadeMode`, `OnUnitDeath` |

- `OnAllCreatureUpdate` est le tick principal. Il est appelé après la mise à jour
  native de la créature et ne remplace aucun comportement.
- `OnUnitEnterCombat` calcule l'éligibilité et crée le contexte de combat.
- `OnUnitExitCombat`, `OnUnitEnterEvadeMode`, `OnUnitDeath` et
  `OnCreatureRemoveWorld` détruisent le contexte. Le dernier hook sert de filet de
  sécurité au despawn.
- `OnCreatureAddWorld` est enregistré comme point d'ancrage du cycle de vie, sans
  logique en Phase 1.
- `OnAfterConfigLoad` relit la configuration. Un `.reload config` applique les
  nouvelles valeurs et vide les contextes en mémoire ; ils sont recréés au fil
  des combats et des ticks suivants.
- `OnUpdate` réinitialise le budget global `MaxDecisionsPerTick` pour le tick.

### Filtrage et limitation de charge

Pour chaque créature, le module vérifie dans l'ordre : module activé, créature en
monde, vivante et engagée (`IsEngaged()`), puis l'éligibilité. L'éligibilité est
calculée à l'entrée en combat et mise en cache dans le contexte associé au GUID.
Une créature non éligible conserve intégralement son IA native.

Une décision n'est évaluée que si l'intervalle `DecisionIntervalMs` (plus un jitter
aléatoire dans `[0, DecisionJitterMs]`) est écoulé. Le budget
`MaxDecisionsPerTick` est partagé par toutes les créatures pour un tick du monde :
au-delà, la décision est comptée comme différée et retentée à l'intervalle suivant.

### Journalisation et diagnostics

- Au chargement et à chaque rechargement de configuration :
  `LOG_INFO("server.loading", ...)`, avec l'état activé/désactivé.
- Avec `CombatAI.Debug = 1` : `LOG_DEBUG("scripts.ai", ...)` à l'entrée en combat
  d'une créature éligible et à chaque décision évaluée.
- Compteurs en mémoire : `decisionsEvaluated`, `skippedNotEligible`,
  `skippedBudget`, `contextsCreated`. La Phase 1 n'expose ni commande console ni
  export périodique de ces compteurs.

### Ce qui n'est pas fait en Phase 1

Aucune tactique n'est implémentée. Le module ne change pas de cible, ne lance
aucun sort, ne déplace pas la créature, n'ajoute pas de menace et n'appelle pas
`CallForHelp`, `MoveFleeing`, `MovePoint` ni `EngageWithTarget`. Les clés
`CallForHelp.*`, `Retreat.*`, `ProtectAlly.Enable`, `IndividualIntelligence` et
`GroupIntelligence` sont lues et conservées pour les phases suivantes.

## Éligibilité et exclusions

Une créature n'est éligible que si **toutes** les conditions suivantes sont
remplies :

- elle existe, est en monde, vivante et engagée en combat ;
- elle n'est ni pet, ni charmée, ni en mode evade, ni civile ;
- elle ne porte pas `UNIT_FLAG_NON_ATTACKABLE` ou `UNIT_FLAG_NOT_SELECTABLE` ;
- son template de créature existe ;
- si `HumanoidsOnly = 1`, son type est `CREATURE_TYPE_HUMANOID` ;
- son `ScriptName` est vide ;
- elle n'est ni boss mondial (`isWorldBoss()`), ni boss de donjon
  (`IsDungeonBoss()`) ;
- si `OpenWorldOnly = 1`, sa carte est une carte ouverte (`IsWorldMap()`) ;
- son entrée n'est pas dans `ExcludeEntries` et sa carte n'est pas dans
  `ExcludeMaps` (les exclusions sont prioritaires) ;
- si `OptInOnly = 1`, son entrée figure dans `IncludeEntries` ou sa carte figure
  dans `IncludeMaps`.

Conséquences pratiques :

- Un `ScriptName` non vide exclut les IA de script enregistrées ; les créatures
  SmartAI ordinaires ont un `ScriptName` vide et restent donc éligibles si elles
  sont explicitement opt-in.
- `OpenWorldOnly = 0` autorise aussi les instances, raids, BG et arènes, sous
  réserve des autres filtres ; ce réglage doit être réservé aux tests.
- `OptInOnly = 0` élargit le module à toutes les créatures qui passent les autres
  filtres. Ce n'est pas le mode recommandé en Phase 1.

## Compatibilité

- **Pas de `GetCreatureAI`.** Ce hook retourne la première IA non nulle et
  remplacerait SmartAI ou les scripts existants. Le module utilise uniquement des
  hooks additifs : l'IA native continue de tourner et peut ignorer ou écraser de
  futurs ordres.
- **SmartAI n'est pas remplacé.** Ses actions natives d'appel à l'aide ou de fuite
  restent actives ; la Phase 1 n'en ajoute aucune.
- **mod-hunts.** Ses proies d'élite et ses summons peuvent avoir un `ScriptName`
  vide, et certains summons n'ont pas d'`AIName` : l'exclusion par `ScriptName` ne
  suffit donc pas. Ce module écrit lui-même mouvement et engagement. Tant que
  l'opt-in reste limité à des entrées précises, ses créatures restent hors
  périmètre ; toute inclusion par carte ou tout passage à `OptInOnly = 0` exige
  des exclusions explicites par entrée ou par carte.
- **mod-living-world.** Ses `TempSummon` peuvent aussi avoir un `ScriptName` vide.
  Les exclure explicitement par entrée ou par carte avant d'élargir l'opt-in.
- **autobalance et playerbots** ne sont pas présents dans ce checkout. Leur
  compatibilité devra être retestée s'ils sont ajoutés.
- **Collision de noms.** `CombatAI` existe comme classe du core et comme `AIName`.
  Le module utilise le préfixe `ModCombatAI*`, le symbole
  `Addmod_combat_aiScripts()` et conserve les clés de configuration `CombatAI.*`.
- Aucun patch du core n'est nécessaire ; aucune modification des autres modules.

## Limites de la Phase 1

- Le module est en observation seule : aucune tactique, aucun ordre natif, aucun
  changement de comportement de combat.
- Les clés des tactiques futures sont présentes mais inertes.
- Les contrôles de ligne de vue et de phase ne sont pas utilisés en Phase 1 ; leur
  sémantique exacte reste à valider avant la Phase 2.
- L'éligibilité est mise en cache à l'entrée en combat. Un changement de drapeaux
  ou de type en cours de combat n'est pas réévalué avant la destruction du contexte
  (fin de combat, evade, mort, despawn ou rechargement de configuration).
- Les compteurs sont en mémoire uniquement. Un `.reload config` ne les remet pas
  à zéro : seuls les contextes et le budget du tick sont réinitialisés.
- Aucun chemin de code ne modifie les statistiques, les sorts, la menace ou le
  mouvement des créatures.

## Build

Depuis la racine du dépôt :

```sh
cmake -S . -B var/build
cmake --build var/build --target worldserver -j
```

La détection du module et de ses sources se fait pendant la configuration CMake.
Après l'ajout du module ou d'un nouveau fichier `.cpp`, relancer la commande
`cmake -S . -B var/build` avant de compiler. Si un arbre `var/build` existant
contient un cache périmé ou un `ModulesLoader.cpp` sans le module, relancer la
configuration ; au besoin, repartir d'un dossier de build propre.

Le loader généré doit référencer le symbole `Addmod_combat_aiScripts()`, dérivé du
nom de dossier `mod-combat-ai` par la machinerie des modules AzerothCore.
