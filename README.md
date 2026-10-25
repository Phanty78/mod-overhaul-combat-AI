# mod-combat-ai

**État : Phase 2 — appel à l'aide borné (version 0.2.0). Le squelette de la
Phase 1 reste la fondation.**

Module AzerothCore pour WotLK 3.3.5a. Il ajoute une couche de décision de combat
externe aux créatures, sans remplacer leur IA native. La Phase 1 a livré
l'intégration : chargement du module, configuration, politique d'éligibilité, hooks
de cycle de vie, limitation de charge et journalisation. La Phase 2 ajoute une
seule tactique, l'appel à l'aide borné : une créature éligible et opt-in peut faire
engager un nombre limité d'alliés proches, avec un cooldown par créature et un
cooldown local de groupe. Aucune autre tactique n'est exécutée.

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
   mod-combat-ai: disabled (call for help enabled)
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

Aucune mise à jour SQL n'est requise en Phase 2.

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
| `CombatAI.IndividualIntelligence` | float | `0.5` | Poids de décision individuelle, entre `0.0` et `1.0`. Stocké pour les phases ultérieures ; sans effet en Phase 2. |
| `CombatAI.GroupIntelligence` | float | `0.5` | Poids de coordination de groupe, entre `0.0` et `1.0`. Stocké pour les phases ultérieures ; sans effet en Phase 2. |
| `CombatAI.CallForHelp.Enable` | bool | `1` | Active la tactique d'appel à l'aide de la Phase 2. À `0`, aucune tentative d'appel n'est faite. |
| `CombatAI.CallForHelp.MaxAllies` | uint32 | `2` | Plafond d'alliés engagés par appel, entre `0` et `10`. À `0`, la tactique est court-circuitée : aucun scan et aucun allié engagé. |
| `CombatAI.CallForHelp.Radius` | float | `15` | Rayon de recherche des alliés, en mètres. Doit être `> 0`. Utilisé avec les contrôles de zone, de distance et de ligne de vue. |
| `CombatAI.CallForHelp.CooldownMs` | uint32 | `30000` | Cooldown par créature entre deux appels, en ms (`0` = pas de délai). |
| `CombatAI.Retreat.Enable` | bool | `1` | Option réservée à la Phase 3 ; aucune fuite n'est déclenchée en Phase 2. |
| `CombatAI.Retreat.HealthThresholdPct` | uint32 | `20` | Seuil de vie en pourcentage, entre `1` et `100`. Stocké pour la Phase 3 ; sans effet en Phase 2. |
| `CombatAI.Retreat.CooldownMs` | uint32 | `60000` | Cooldown de retrait en ms, stocké pour la Phase 3 ; sans effet en Phase 2. |
| `CombatAI.ProtectAlly.Enable` | bool | `1` | Option réservée à la Phase 4 ; aucune protection n'est exécutée en Phase 2. |
| `CombatAI.MaxDecisionsPerTick` | uint32 | `64` | Budget global de décisions par tick du monde. Doit être `> 0`. Les décisions au-delà du budget sont différées. |
| `CombatAI.Debug` | bool | `0` | Si `1`, journalise dans `scripts.ai` l'entrée en combat éligible, chaque décision évaluée et le résultat des tentatives d'appel à l'aide. |

Les listes CSV ignorent les espaces autour des valeurs et les jetons vides, non
numériques ou hors plage. Une valeur invalide pour une clé validée produit un
`LOG_ERROR` dans `server.loading` et la valeur par défaut est utilisée.

> Les valeurs livrées sont des **points de départ pour les tests**, pas des
> valeurs d'équilibrage validées. En particulier, `OptInOnly = 1` avec des listes
> d'inclusion vides signifie zéro créature affectée, et une inclusion par carte
> est beaucoup plus large qu'une inclusion par entrée.

## Comportement du module

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
  logique.
- `OnAfterConfigLoad` relit la configuration. Un `.reload config` applique les
  nouvelles valeurs, vide les contextes en mémoire et remet à zéro les cooldowns
  de groupe ; contextes et cooldowns sont recréés au fil des combats et des ticks
  suivants.
- `OnUpdate` réinitialise le budget global `MaxDecisionsPerTick` pour le tick et
  élague périodiquement les cooldowns de groupe expirés.

### Filtrage et limitation de charge

Pour chaque créature, le module vérifie dans l'ordre : module activé, créature en
monde, vivante et engagée (`IsEngaged()`), puis l'éligibilité. L'éligibilité est
calculée à l'entrée en combat et mise en cache dans le contexte associé au GUID.
Une créature non éligible conserve intégralement son IA native.

Une décision n'est évaluée que si l'intervalle `DecisionIntervalMs` (plus un jitter
aléatoire dans `[0, DecisionJitterMs]`) est écoulé. Le budget
`MaxDecisionsPerTick` est partagé par toutes les créatures pour un tick du monde :
au-delà, la décision est comptée comme différée et retentée à l'intervalle suivant.
Le budget s'applique avant la tactique : une décision différée ne tente aucun appel
à l'aide.

### Journalisation et diagnostics

- Au chargement et à chaque rechargement de configuration :
  `LOG_INFO("server.loading", ...)`, avec l'état du module et celui de
  `CombatAI.CallForHelp.Enable`.
- Avec `CombatAI.Debug = 1` : `LOG_DEBUG("scripts.ai", ...)` à l'entrée en combat
  d'une créature éligible et à chaque décision évaluée. La Phase 2 ajoute deux
  diagnostics d'appel : absence d'allié dans le rayon et résumé des alliés
  effectivement engagés. Aucun journal n'est émis par allié.
- Compteurs en mémoire : `decisionsEvaluated`, `skippedNotEligible`,
  `skippedBudget`, `contextsCreated`, `callForHelpAttempts`,
  `callForHelpAlliesEngaged`, `callForHelpGroupSkipped`, `callForHelpNoAllies`.
  `callForHelpAttempts` compte les scans réellement exécutés,
  `callForHelpAlliesEngaged` les alliés engagés, `callForHelpGroupSkipped` les
  tentatives bloquées par le cooldown de groupe et `callForHelpNoAllies` les scans
  sans allié qualifié. Le module n'expose ni commande console ni export périodique
  de ces compteurs.

### Ce qui n'est pas fait en Phase 2

La Phase 2 n'implémente que l'appel à l'aide. Le module ne change pas la cible de
l'initiateur, ne lance aucun sort, ne déplace pas la créature et n'appelle ni
`MoveFleeing` ni `MovePoint`. Il n'ajoute de menace que par l'engagement natif des
alliés via `EngageWithTarget`. La fuite, la protection d'allié et le
repositionnement restent des phases ultérieures, tout comme le moteur de décision
fondé sur `IndividualIntelligence` et `GroupIntelligence`. Les clés `Retreat.*`,
`ProtectAlly.Enable`, `IndividualIntelligence` et `GroupIntelligence` sont lues et
conservées pour ces phases.

## Phase 2 — appel à l'aide

La Phase 2 ajoute une unique tactique. Au plus une action tactique est exécutée par
décision.

### Déclenchement

- L'initiateur doit être éligible pour le module et opt-in (`CombatAI.IncludeEntries`
  ou `CombatAI.IncludeMaps`), en combat, vivant et disposer d'une cible réelle :
  victime (`GetVictim()`) ou cible de menace. Sans cible, aucun appel n'est tenté.
- La tactique est court-circuitée si `CombatAI.CallForHelp.Enable = 0` ou si
  `CombatAI.CallForHelp.MaxAllies = 0` : aucun scan n'est lancé et aucun allié
  n'est engagé.
- Un cooldown de créature actif (`callForHelpCooldownEnd`, alimenté par
  `CombatAI.CallForHelp.CooldownMs`) ou un cooldown de groupe actif annule la
  tentative. Le cooldown de créature est propre à l'initiateur.
- Le budget `CombatAI.MaxDecisionsPerTick` s'applique en amont ; une décision
  refusée par le budget ne consomme pas de tentative d'appel.

### Sélection des alliés

- Scan local via `Cell::VisitObjects` autour de l'initiateur, dans
  `CombatAI.CallForHelp.Radius`.
- Chaque candidat doit passer `CanAssistTo(initiateur, ennemi, false)`, être sur
  la même carte et dans la même zone (`GetZoneId()`), à portée et voir l'ennemi
  (`IsWithinLOSInMap(ennemi)`). Le filtre `CreatureWorker` impose déjà la même
  phase.
- Les alliés retenus sont plafonnés par `CombatAI.CallForHelp.MaxAllies`.
- **Les alliés n'ont pas besoin d'être éligibles au module ni d'être opt-in.**
  Seul l'initiateur est soumis à l'éligibilité et à l'opt-in ; les alliés sont
  retenus par les contrôles natifs ci-dessus.

### Exécution et anti-récursion

- Chaque allié retenu est engagé via `EngageWithTarget(ennemi)`, encadré par
  `SetNoCallForHelp(true)` puis `SetNoCallForHelp(false)` pour empêcher l'appel
  natif récursif du core.
- Le cooldown d'appel de l'allié est renseigné s'il possède déjà un contexte,
  afin qu'il ne déclenche pas un nouvel appel en cascade.
- Le cooldown de groupe est local : clé `mapId` + cellule de 40 yards. Il n'est
  enregistré qu'après un engagement effectif d'au moins un allié ; un scan sans
  allié qualifié arme le cooldown de créature mais pas celui du groupe.
- Aucun sort, aucun changement de cible de l'initiateur et aucun déplacement ne
  sont effectués. L'appel se limite à l'engagement natif des alliés.

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
  filtres. Ce n'est pas le mode recommandé en Phase 2.
- Cette politique ne s'applique qu'à l'initiateur d'un appel à l'aide. Les alliés
  retenus par `CanAssistTo` ne sont pas soumis à l'éligibilité du module ni à
  l'opt-in (voir la section Phase 2).

## Compatibilité

- **Pas de `GetCreatureAI`.** Ce hook retourne la première IA non nulle et
  remplacerait SmartAI ou les scripts existants. Le module utilise uniquement des
  hooks additifs : l'IA native continue de tourner et peut ignorer ou écraser de
  futurs ordres.
- **SmartAI n'est pas remplacé.** Ses actions natives d'appel à l'aide ou de fuite
  restent actives ; la Phase 2 ajoute uniquement l'appel borné décrit ci-dessus,
  sans désactiver ces actions.
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

## Limites actuelles (Phase 2)

- Un test de fumée local à trois créatures temporaires a validé l'engagement
  d'un allié et l'absence de seconde tentative sur cinq secondes (cooldowns
  créature et groupe actifs). Les tests joueurs solo/duo/groupe, le plafond
  multi-alliés et la charge ne sont pas encore validés en conditions réelles.
  Le protocole et le résultat sont dans `docs/phase-2-test.md`.
- La Phase 2 n'exécute qu'une tactique. La fuite, la protection d'allié, le
  repositionnement et le moteur de décision restent à implémenter ; les clés
  correspondantes sont présentes mais inertes.
- La ligne de vue (`IsWithinLOSInMap()`) et le filtre de phase fourni par
  `CreatureWorker` sont utilisés et ont été exercés par le test de fumée ; les
  cas complexes (obstacles, phases multiples) restent à valider en jeu.
- Le cooldown de groupe est local et approximatif : il est indexé par `mapId` et
  par cellule de 40 yards, sans tenir compte de la composition réelle du groupe.
- Les alliés ne sont pas tenus d'être éligibles au module ni d'être opt-in ; seul
  l'initiateur l'est. Un allié peut donc être une créature que le module ne
  piloterait jamais par ailleurs.
- L'éligibilité est mise en cache à l'entrée en combat. Un changement de drapeaux
  ou de type en cours de combat n'est pas réévalué avant la destruction du contexte
  (fin de combat, evade, mort, despawn ou rechargement de configuration).
- Les compteurs sont en mémoire uniquement. Un `.reload config` ne les remet pas
  à zéro : seuls les contextes, les cooldowns de groupe et le budget du tick sont
  réinitialisés.
- Aucun chemin de code ne modifie les statistiques, les sorts ou le mouvement des
  créatures. La seule modification de menace provient de l'engagement natif des
  alliés par `EngageWithTarget`.
- L'IA native des alliés reste maîtresse de leur comportement : elle peut changer
  de cible ou ignorer l'engagement demandé.
- Un rayon `CallForHelp.Radius` élevé augmente le coût du scan effectué à chaque
  tentative d'appel.

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
