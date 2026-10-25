# mod-combat-ai

**État : Phase 4 — protection des alliés (version 0.4.0). Les phases 1 à 3
restent la fondation.**

Module AzerothCore pour WotLK 3.3.5a. Il ajoute une couche de décision de combat
externe aux créatures, sans remplacer leur IA native. La Phase 1 a livré
l'intégration : chargement du module, configuration, politique d'éligibilité, hooks
de cycle de vie, limitation de charge et journalisation. La Phase 2 ajoute une
première tactique, l'appel à l'aide borné : une créature éligible et opt-in peut
faire engager un nombre limité d'alliés proches, avec un cooldown par créature et
un cooldown local de groupe. La Phase 3 ajoute la fuite bornée : une créature
éligible et opt-in dont la vie est inférieure ou égale au seuil rejoint l'allié le
plus proche par un déplacement natif, ou fuit brièvement à défaut d'allié. La
Phase 4 ajoute la protection des alliés : un soigneur éligible soigne un allié
blessé avec un sort de soin réellement présent dans sa liste de sorts, et un
combattant non soigneur se porte vers le point milieu entre un soigneur allié en
danger et son attaquant, en ajoutant une référence de combat à zéro menace. Au
plus une action tactique est exécutée par décision.

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

   Le premier jeton reflète `CombatAI.Enable`, le second
   `CombatAI.CallForHelp.Enable`. Avec `CombatAI.Enable = 1` et
   `CombatAI.CallForHelp.Enable = 0`, la ligne devient
   `mod-combat-ai: enabled (call for help disabled)`.

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

Aucune mise à jour SQL n'est requise en Phase 4.

## Configuration

Fichier : `env/dist/etc/modules/mod_combat_ai.conf` (`conf/mod_combat_ai.conf.dist`
comme modèle). Les 23 clés sont rechargées par `.reload config` via
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
| `CombatAI.IndividualIntelligence` | float | `0.5` | Poids de décision individuelle, entre `0.0` et `1.0`. Stocké pour les phases ultérieures ; sans effet en Phase 4. |
| `CombatAI.GroupIntelligence` | float | `0.5` | Poids de coordination de groupe, entre `0.0` et `1.0`. Stocké pour les phases ultérieures ; sans effet en Phase 4. |
| `CombatAI.CallForHelp.Enable` | bool | `1` | Active la tactique d'appel à l'aide de la Phase 2. À `0`, aucune tentative d'appel n'est faite. |
| `CombatAI.CallForHelp.MaxAllies` | uint32 | `2` | Plafond d'alliés engagés par appel, entre `0` et `10`. À `0`, la tactique est court-circuitée : aucun scan et aucun allié engagé. |
| `CombatAI.CallForHelp.Radius` | float | `15` | Rayon de recherche des alliés, en mètres. Doit être `> 0`. Utilisé avec les contrôles de zone, de distance et de ligne de vue. |
| `CombatAI.CallForHelp.CooldownMs` | uint32 | `30000` | Cooldown par créature entre deux appels, en ms (`0` = pas de délai). |
| `CombatAI.Retreat.Enable` | bool | `1` | Active la tactique de fuite de la Phase 3. À `0`, aucune fuite n'est déclenchée et le suivi d'un déplacement de fuite en cours est abandonné au tick suivant. |
| `CombatAI.Retreat.HealthThresholdPct` | uint32 | `20` | Seuil de vie en pourcentage, entre `1` et `100`. La fuite peut se déclencher lorsque la vie de la créature est inférieure ou égale à ce seuil. |
| `CombatAI.Retreat.CooldownMs` | uint32 | `60000` | Cooldown par créature entre deux tentatives de fuite, en ms (`0` = pas de délai). |
| `CombatAI.Retreat.Radius` | float | `15` | Rayon de recherche de l'allié destination, en mètres. Doit être `> 0`. L'allié le plus proche dans ce rayon, visible et dans la même zone, sert de destination. |
| `CombatAI.ProtectAlly.Enable` | bool | `1` | Active la protection des alliés de la Phase 4. À `1`, un soigneur peut soigner un allié blessé et un non-soigneur peut intercepter l'attaquant d'un soigneur en danger. À `0`, aucune des deux branches n'est exécutée. Les seuils et cooldowns de ces branches sont des constantes de code, pas des clés. |
| `CombatAI.MaxDecisionsPerTick` | uint32 | `64` | Budget global de décisions par tick du monde. Doit être `> 0`. Les décisions au-delà du budget sont différées. |
| `CombatAI.Debug` | bool | `0` | Si `1`, journalise dans `scripts.ai` l'entrée en combat éligible, chaque décision évaluée, le résultat des tentatives d'appel à l'aide, les diagnostics de fuite et les soins/interceptions de la Phase 4. |

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
Le budget s'applique avant la tactique : une décision différée ne tente aucune
tactique, fuite, protection ou appel à l'aide.

### Journalisation et diagnostics

- Au chargement et à chaque rechargement de configuration :
  `LOG_INFO("server.loading", ...)`, avec l'état du module et celui de
  `CombatAI.CallForHelp.Enable`.
- Avec `CombatAI.Debug = 1` : `LOG_DEBUG("scripts.ai", ...)` à l'entrée en combat
  d'une créature éligible et à chaque décision évaluée. La Phase 2 ajoute deux
  diagnostics d'appel : absence d'allié dans le rayon et résumé des alliés
  effectivement engagés. La Phase 3 ajoute trois diagnostics de fuite :
  destination alliée retenue, absence d'allié avec repli, et expiration du
  déplacement faute de progression. La Phase 4 ajoute deux diagnostics de
  protection : soin d'un allié avec le sort retenu, et interception d'un
  attaquant pour un soigneur en danger. Aucun journal n'est émis par allié.
- Compteurs en mémoire : `decisionsEvaluated`, `skippedNotEligible`,
  `skippedBudget`, `contextsCreated`, `callForHelpAttempts`,
  `callForHelpAlliesEngaged`, `callForHelpGroupSkipped`, `callForHelpNoAllies`,
  `retreatAttempts`, `retreatAllies`, `retreatFallbacks`, `retreatStuck`,
  `retreatCompleted`, `protectAllyAttempts`, `protectAllyHeals`,
  `protectAllyIntercepts` et `protectAllyNoTarget`. `callForHelpAttempts` compte
  les scans réellement exécutés, `callForHelpAlliesEngaged` les alliés engagés,
  `callForHelpGroupSkipped` les tentatives bloquées par le cooldown de groupe et
  `callForHelpNoAllies` les scans sans allié qualifié. Côté fuite,
  `retreatAttempts` compte les tentatives réellement exécutées, `retreatAllies`
  les fuites dirigées vers un allié, `retreatFallbacks` les replis `MoveFleeing`,
  `retreatStuck` les déplacements expirés après 3 s sans progression et
  `retreatCompleted` les déplacements terminés ou remplacés. Côté protection,
  `protectAllyAttempts` compte les actions réellement exécutées (un soin ou une
  interception), `protectAllyHeals` les soins lancés, `protectAllyIntercepts` les
  interceptions démarrées et `protectAllyNoTarget` les décisions arrivées au scan
  sans allié, soigneur ou attaquant qualifié, ou sans sort de soin lançable. Le
  module n'expose ni commande console ni export périodique de ces compteurs.

### Ce qui n'est pas fait en Phase 4

À l'issue de la Phase 4, le module implémente l'appel à l'aide, la fuite et la
protection des alliés. Le module ne change pas la cible de l'initiateur :
l'interception ajoute une référence de combat à zéro menace via
`EngageWithTarget`, sans `AttackStart` ni remplacement de la victime. Le soin ne
sélectionne que des sorts positifs `SPELL_EFFECT_HEAL` déjà présents dans
`m_spells` ; aucun sort n'est inventé, appris ou ajouté à la créature. La fuite
déplace la créature avec les générateurs natifs `MovePoint` (vers un allié) et
`MoveFleeing` (repli sans allié), et l'interception utilise `MovePoint` vers le
point milieu soigneur/attaquant, sans remplacer l'IA. Le repositionnement et le
moteur de décision fondé sur `IndividualIntelligence` et `GroupIntelligence`
restent des phases ultérieures. Les clés `IndividualIntelligence` et
`GroupIntelligence` sont lues et conservées pour ces phases.

## Phase 2 — appel à l'aide

La Phase 2 ajoute une première tactique. Au plus une action tactique est exécutée
par décision, et les tactiques des phases 3 et 4 sont prioritaires sur l'appel à
l'aide : l'ordre est fuite, protection des alliés, puis appel (voir les sections
Phase 3 et Phase 4).

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

## Phase 3 — fuite

La Phase 3 ajoute la fuite bornée. Elle est évaluée en premier et consomme la
décision lorsqu'elle démarre ou lorsqu'un déplacement de fuite est encore actif.

### Déclenchement

- L'initiateur doit être éligible et opt-in, en combat et vivant.
- `CombatAI.Retreat.Enable = 1`.
- Sa vie doit être inférieure ou égale à `CombatAI.Retreat.HealthThresholdPct`.
- Le cooldown de créature (`retreatCooldownEnd`, alimenté par
  `CombatAI.Retreat.CooldownMs`) doit être échu.
- Une cible réelle vivante doit exister : victime (`GetVictim()`), à défaut
  cible de menace courante, à défaut n'importe quelle cible de menace.
- La créature ne doit pas déjà porter `UNIT_STATE_FLEEING`.
- Le budget `CombatAI.MaxDecisionsPerTick` s'applique en amont ; une décision
  refusée par le budget ne consomme pas de tentative de fuite.

### Destination alliée

- Scan local via `Cell::VisitObjects` autour de l'initiateur, dans
  `CombatAI.Retreat.Radius`.
- Chaque candidat doit être ami de l'initiateur (`IsFriendlyTo`), vivant, hors
  evade, sur la même carte, dans la même zone (`GetZoneId()`), à portée et voir
  l'initiateur (`IsWithinLOSInMap(initiateur)`). Le filtre `CreatureWorker`
  impose déjà la même phase.
- L'allié le plus proche (`GetDistance`) est retenu. **Il n'a pas besoin d'être
  éligible au module ni opt-in.**
- La destination est l'allié lui-même : `MotionMaster::MovePoint` aux coordonnées
  de l'allié, avec l'identifiant unique `0x4D434149`. Cet identifiant évite de
  collisionner avec les identifiants de points utilisés par les événements
  SmartAI.
- Aucun `MoveSeekAssistance`, aucun `SetCombatMovement` et aucun
  `UNIT_STATE_NO_COMBAT_MOVEMENT` : la fuite n'ajoute pas les effets de bord de
  `MoveSeekAssistance` (arrêt d'attaque, arrêt de sort, passage en
  `REACT_PASSIVE`) et ne fige pas le mouvement de combat.

### Repli sans allié

- Sans allié qualifié, la créature exécute `MoveFleeing(ennemi, 3000)` : fuite
  native bornée à 3 secondes.
- Le cooldown de créature est armé après l'ordre de mouvement, dans les deux
  branches.
- `retreatAttempts` est incrémenté pour chaque tentative réelle ; `retreatAllies`
  pour une destination alliée et `retreatFallbacks` pour le repli.

### Surveillance du déplacement

- `moduleMoving` marque une fuite active ; le contexte mémorise la position de
  référence et l'horodatage de progression.
- Le watchdog examine le générateur actif et l'emplacement
  `MOTION_SLOT_CONTROLLED` : `POINT_MOTION_TYPE`, `FLEEING_MOTION_TYPE` ou
  `TIMED_FLEEING_MOTION_TYPE` signalent une fuite en cours.
- Un déplacement de plus d'un yard par rapport à la dernière position de
  référence rafraîchit l'horodatage de progression.
- Après 3 secondes sans progression, `MovementExpired()` est appelée,
  `moduleMoving` retombe à `false` et `retreatStuck` est incrémenté.
- Si le générateur disparaît (fin, remplacement par l'IA native), `moduleMoving`
  retombe à `false` et `retreatCompleted` est incrémenté.
- Tant que `moduleMoving` est vrai, la fuite consomme la décision : aucune
  nouvelle fuite, aucune protection d'allié et aucun appel à l'aide ne sont
  tentés.

### Priorité et cooldown

- `TryRetreat` est évaluée avant `TryProtectAlly` puis `TryCallForHelp`.
  La protection et l'appel ne sont tentés que si la fuite n'a pas consommé la
  décision.
- Le cooldown de fuite est propre à la créature et n'est pas partagé entre
  créatures.
- `CombatAI.Retreat.Enable = 0` désactive la tactique ; le suivi d'un
  déplacement de fuite déjà en cours est abandonné (`moduleMoving = false`) au
  tick suivant, sans expiration forcée du générateur.

## Phase 4 — protection des alliés

La Phase 4 ajoute deux branches de protection, évaluées après la fuite et avant
l'appel à l'aide. Une seule action tactique est exécutée par décision : si la
fuite consomme la décision, la protection et l'appel ne sont pas tentés ; si la
protection agit, l'appel ne l'est pas. Le budget `CombatAI.MaxDecisionsPerTick`
s'applique en amont.

### Priorité et conditions communes

- Ordre d'évaluation : `TryRetreat`, puis `TryProtectAlly`, puis
  `TryCallForHelp`.
- L'initiateur doit être éligible et opt-in, en combat et vivant, et
  `CombatAI.ProtectAlly.Enable = 1`.
- Le cooldown de protection de la créature (`protectAllyCooldownEnd`) doit être
  échu. Il est propre à la créature et n'est pas partagé.
- La créature ne doit pas être en cours de cast (`UNIT_STATE_CASTING`). Ce
  contrôle est fait avant le choix de la branche.
- Les alliés soignés, le soigneur protégé et l'attaquant intercepté ne sont pas
  soumis à l'éligibilité du module ni à l'opt-in ; seul l'initiateur l'est.

### Branche soigneur

- Est considérée soigneur une créature dont la liste publique `m_spells`
  contient au moins un sort avec `SPELL_EFFECT_HEAL` et `IsPositive()`. Aucun
  autre sort n'est utilisé.
- Scan local via `Cell::VisitObjects` dans un rayon fixe de 20 yards. Chaque
  candidat doit être vivant, ami, différent du soigneur, sur la même carte et
  dans la même zone (`GetZoneId()`), à portée, visible
  (`IsWithinLOSInMap(soigneur)`) et avoir une vie strictement inférieure à 80 %.
  L'allié dont le pourcentage de vie est le plus bas est retenu.
- Le soin n'est lancé que si un sort de soin réel passe tous les contrôles :
  cooldown du sort à 0, `SpellInfo::CheckTarget(soigneur, allié, false)` égal à
  `SPELL_CAST_OK`, distance dans `GetMaxRange(true, soigneur)` (repli de 40 yards
  si la portée maximale est nulle ou négative) et ligne de vue vers l'allié.
- `CastSpell(allié, sort, false)` est exécuté sur l'allié. Le cooldown de
  protection est armé à 1000 ms. `protectAllyAttempts` et `protectAllyHeals`
  sont incrémentés.
- Aucun ordre de déplacement, aucun changement de cible et aucun appel explicite
  aux API de menace ne sont effectués par cette branche.

### Branche non-soigneur (interception)

- Réservée aux créatures sans sort de soin (`HasHealSpell` faux).
- Ignorée si un déplacement de fuite est actif (`moduleMoving` ou générateur
  `POINT_MOTION_TYPE`, `FLEEING_MOTION_TYPE` ou `TIMED_FLEEING_MOTION_TYPE`).
- Scan local de 20 yards : allié vivant, ami, différent de l'initiateur, même
  carte et même zone, à portée, visible, portant un sort de soin réel et engagé
  (`IsEngaged()`), avec une vie inférieure ou égale à 60 %. Le soigneur dont le
  pourcentage de vie est le plus bas est retenu.
- L'attaquant est la victime du soigneur (`GetVictim()`) si elle est non nulle ;
  sinon la première cible vivante et en ligne de la liste de menace triée
  (`GetSortedThreatList()`). Il doit être vivant, hostile à l'initiateur et à
  portée de 20 yards.
- Si l'initiateur attaque déjà cet attaquant (`GetVictim() == attaquant`),
  l'interception est ignorée sans incrémenter de compteur.
- Sinon, l'initiateur reçoit un `MovePoint` vers le point milieu
  soigneur/attaquant, avec l'identifiant unique `0x4D434150`, puis
  `EngageWithTarget(attaquant)`. Cet appel ajoute une référence de combat à zéro
  menace : le module n'appelle pas directement `AddThreat`, `ModifyThreat` ou
  `ScaleThreat`, et ne fait aucun `AttackStart` ni changement de victime.
- Le cooldown est armé à 5000 ms. `protectAllyAttempts` et
  `protectAllyIntercepts` sont incrémentés.

### Constantes

Ces valeurs sont fixées dans le code et ne sont pas des clés de configuration :

| Constante | Valeur | Rôle |
|---|---:|---|
| Rayon de protection | `20` yards | Scan des alliés blessés ou des soigneurs en danger. |
| Seuil de soin | `80` % | Un allié dont la vie est strictement inférieure à ce seuil peut être soigné. |
| Seuil de danger du soigneur | `60` % | Un soigneur allié dont la vie est inférieure ou égale à ce seuil peut être protégé. |
| Cooldown de soin | `1000` ms | Délai par créature après un soin lancé. |
| Cooldown d'interception | `5000` ms | Délai par créature après une interception démarrée. |
| Identifiant du point d'interception | `0x4D434150` | Évite les collisions avec les points de SmartAI. |

Le module conserve **23 clés de configuration** ; la Phase 4 n'ajoute aucune clé.

### Garanties

- **Sorts réels uniquement.** La branche soigneur ne peut lancer qu'un sort
  présent dans `m_spells` avec `SPELL_EFFECT_HEAL` positif. Aucun sort n'est
  inventé, appris ou ajouté.
- **Aucune menace artificielle.** Le module n'appelle pas directement
  `AddThreat`, `ModifyThreat` ou `ScaleThreat`. L'interception ajoute seulement
  une référence de combat à zéro menace via `EngageWithTarget`.
- **Aucun changement de cible.** Pas de `AttackStart` ni de remplacement de
  `GetVictim()` ; le soin ne change pas la cible de combat.
- **Seuls les initiateurs sont filtrés.** Les alliés soignés, le soigneur
  protégé et l'attaquant ne sont pas soumis à l'éligibilité du module ni à
  l'opt-in.

Le protocole et le résultat du test de fumée sont dans
[`docs/phase-4-test.md`](docs/phase-4-test.md).

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
  filtres. Ce n'est pas le mode recommandé en Phase 4.
- Cette politique ne s'applique qu'à l'initiateur d'une tactique. Les alliés
  retenus par `CanAssistTo` (appel à l'aide), comme destination de fuite, comme
  cible de soin ou comme soigneur protégé ne sont pas soumis à l'éligibilité du
  module ni à l'opt-in (voir les sections Phase 2, Phase 3 et Phase 4).

## Compatibilité

- **Pas de `GetCreatureAI`.** Ce hook retourne la première IA non nulle et
  remplacerait SmartAI ou les scripts existants. Le module utilise uniquement des
  hooks additifs : l'IA native continue de tourner et peut ignorer ou écraser de
  futurs ordres.
- **SmartAI n'est pas remplacé.** Ses actions natives d'appel à l'aide, de fuite
  ou de soin restent actives ; les phases 2 à 4 ajoutent l'appel borné, la fuite
  et la protection décrits ci-dessus, sans désactiver ces actions. Un soin natif
  de SmartAI peut donc s'ajouter à celui du module ; le compteur
  `protectAllyHeals` ne compte que les casts du module.
- **Mouvement natif.** La fuite utilise `MovePoint` et `MoveFleeing` du core,
  l'interception utilise `MovePoint`, et le module ne remplace pas l'IA :
  celle-ci peut terminer ou remplacer ces déplacements. Pour la fuite, le
  watchdog traite la disparition du générateur comme une fin de fuite.
- **Pas de menace artificielle.** L'appel à l'aide engage les alliés par le
  mécanisme natif ; l'interception appelle `EngageWithTarget`, qui ajoute une
  référence de combat à zéro menace. Le module n'appelle pas directement
  `AddThreat`, `ModifyThreat` ou `ScaleThreat`.
- **Pas de changement de cible.** L'interception ne fait pas `AttackStart` et ne
  remplace pas la victime de l'initiateur ; le soin ne change pas la cible de
  combat. Seuls les initiateurs éligibles et opt-in exécutent une action.
- **mod-hunts.** Ses proies d'élite et ses summons peuvent avoir un `ScriptName`
  vide, et certains summons n'ont pas d'`AIName` : l'exclusion par `ScriptName` ne
  suffit donc pas. Ce module tiers écrit lui-même mouvement et engagement. Tant
  que l'opt-in reste limité à des entrées précises, ses créatures restent hors
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

## Limites actuelles (Phase 4)

- Un test de fumée local à trois créatures temporaires a validé l'engagement
  d'un allié par l'appel à l'aide et l'absence de seconde tentative sur cinq
  secondes (cooldowns créature et groupe actifs). Les tests joueurs
  solo/duo/groupe, le plafond multi-alliés et la charge ne sont pas encore
  validés en conditions réelles. Le protocole et le résultat sont dans
  `docs/phase-2-test.md`.
- Un second test de fumée local a validé la fuite vers un allié : la créature
  rejoint l'allié (distance 11 → 0), le point se termine (`retreatCompleted = 1`)
  avec une seule tentative (`retreatAttempts = 1`) résolue par un allié
  (`retreatAllies = 1`) et sans blocage (`retreatStuck = 0`) ni repli
  (`retreatFallbacks = 0`). Les tests joueurs solo/duo/groupe, les obstacles, le
  leash et l'evade, ainsi que les durées de plusieurs minutes et la charge ne
  sont pas validés ; le repli `MoveFleeing` n'a pas été exercé. Le protocole et
  le résultat sont dans `docs/phase-3-test.md`.
- Un troisième test de fumée local a validé les deux branches de protection sur
  deux scénarios. Le soin a été lancé (`protectAllyHeals = 1`), l'allié est passé
  de 29,069767 % à 100 % entre T+3 s et T+7 s, et l'interception a ramené la
  distance de surface mélé-soigneur de 7 à 0 avec `meleeType = 0`
  (`IDLE_MOTION_TYPE`). Le total `protectAllyAttempts = 2` correspond à un soin
  et une interception, avec `protectAllyIntercepts = 1` et aucune erreur.
  `protectAllyNoTarget` croît pour les créatures éligibles sans candidat, ce qui
  est attendu. SmartAI peut aussi soigner : le compteur `protectAllyHeals` prouve
  le cast du module, mais la remontée à 100 % n'est pas attribuée exclusivement
  au module. Les tests joueurs solo/duo/groupe, les immunités, les contrôles de
  foule, les compositions de menace, plusieurs soigneurs simultanés, les
  obstacles, les durées de plusieurs minutes et la charge ne sont pas validés ;
  les constantes (20 yards, 80 %, 60 %, 1 s, 5 s) sont fixées dans le code. Le
  protocole et le journal sont dans `docs/phase-4-test.md`.
- La Phase 4 exécute trois tactiques. Le repositionnement et le moteur de
  décision restent à implémenter ; `IndividualIntelligence` et
  `GroupIntelligence` sont présentes mais inertes.
- La ligne de vue (`IsWithinLOSInMap()`) et le filtre de phase fourni par
  `CreatureWorker` sont utilisés et ont été exercés par les tests de fumée ; les
  cas complexes (obstacles, phases multiples) restent à valider en jeu.
- Le cooldown de groupe de l'appel à l'aide est local et approximatif : il est
  indexé par `mapId` et par cellule de 40 yards, sans tenir compte de la
  composition réelle du groupe.
- Les alliés ne sont pas tenus d'être éligibles au module ni d'être opt-in ; seul
  l'initiateur l'est. Un allié peut donc être une créature que le module ne
  piloterait jamais par ailleurs. Il en va de même du soigneur protégé et de
  l'allié soigné.
- L'éligibilité est mise en cache à l'entrée en combat. Un changement de drapeaux
  ou de type en cours de combat n'est pas réévalué avant la destruction du contexte
  (fin de combat, evade, mort, despawn ou rechargement de configuration).
- Les compteurs sont en mémoire uniquement. Un `.reload config` ne les remet pas
  à zéro : seuls les contextes, les cooldowns de groupe et le budget du tick sont
  réinitialisés.
- Aucun chemin de code ne modifie les statistiques ni la liste de sorts. La cible
  de l'initiateur n'est pas remplacée : le soin lance un sort sur un allié et
  l'interception ajoute une référence de combat à zéro menace. Les seules
  modifications de mouvement proviennent de `MovePoint` et `MoveFleeing`. Les
  seules modifications de menace demandées par le module proviennent des appels
  natifs à `EngageWithTarget` (engagement d'alliés de l'appel à l'aide, référence
  à zéro menace de l'interception) ; le soin peut produire la menace native
  associée au sort.
- L'IA native des alliés, du soigneur protégé et de l'attaquant reste maîtresse
  de leur comportement : elle peut changer de cible, lancer ses propres soins ou
  ignorer l'engagement demandé.
- Pendant une fuite, l'IA native de l'initiateur peut changer de cible ou
  remplacer le point de déplacement ; le watchdog détecte la disparition du
  générateur et libère `moduleMoving`.
- Un rayon `CallForHelp.Radius` ou `Retreat.Radius` élevé augmente le coût du
  scan effectué à chaque tentative. Le rayon de protection est fixe à 20 yards.

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
