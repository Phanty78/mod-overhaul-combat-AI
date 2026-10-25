# Changelog

## 1.0.0 - 2026-10-25

### Ajouté

- Instrumentation de mesure réservée à `CombatAI.Debug = 1` : compteurs
  `decisionTimeNs` (cumul en nanosecondes) et `decisionTimeSamples` (nombre
  d'échantillons), et moyenne `avgDecisionUs`
  (`decisionTimeNs / decisionTimeSamples / 1000`, division entière) ajoutée au
  résumé `scripts.ai`. Seul l'appel au moteur de décision est chronométré ; le
  hook, l'éligibilité et le budget ne le sont pas. Les scans internes sont inclus
  dans la mesure du moteur, sans mesure séparée. Aucun changement de comportement
  quand Debug est à `0`.
- `docs/benchmark.md` : but, environnement, protocole, journaux OFF/ON exacts,
  tableaux des compteurs et des durées de mise à jour, interprétation, matrice de
  non-régression, limites et checklist de déploiement progressif.

### Modifié

- Version `1.0.0` : la Phase 6 finalise le déploiement progressif, le benchmark
  reproductible et les tests de non-régression. Les quatre tactiques et le moteur
  de décision sont inchangés ; aucune clé de configuration n'est ajoutée, le
  total reste à 24.
- README : état Phase 6, sections « Déploiement progressif » et « Benchmark et
  non-régression », compteurs de temps documentés, limites et travaux futurs
  actualisés.
- Le résumé `scripts.ai` ajoute `avgDecisionUs` à la fin de la ligne existante.

### Notes

- Benchmark local du 2026-10-25 : carte 0, 0 joueur, 100 couples
  initiateur/ennemi (200 créatures) et 4 sondes d'éligibilité. Le harnais
  temporaire fait apparaître les créatures à T+20 s et la mesure couvre T+20 s à
  T+40 s, avec `RecordUpdateTimeDiffInterval = 10000` et
  `MinRecordUpdateTimeDiff = 0`.

  ```text
  # module OFF
  COMBATAI P6 SMOKE RESULT: decisions=0 skippedBudget=0 contexts=0 callAttempts=0 alliesEngaged=0 callActions=0 noAction=0 avgDecisionUs=0 | normal=true excluded=false scripted=false boss=false

  # module ON
  COMBATAI P6 SMOKE RESULT: decisions=1584 skippedBudget=436 contexts=636 callAttempts=101 alliesEngaged=0 callActions=0 noAction=1584 avgDecisionUs=26 | normal=true excluded=false scripted=false boss=false
  ```

- Durées de mise à jour après l'apparition des créatures : OFF moyenne 1-2 ms,
  médiane 1 ms, p95 3-7 ms, p99 4-8 ms, max 10-25 ms ; ON moyenne 1 ms, médiane
  1 ms, p95 2-3 ms, p99 3-5 ms, max 4-8 ms. Aucune régression mesurable des
  durées de mise à jour n'a été observée à cette échelle locale de 200 créatures ;
  ce constat ne définit pas de seuil de production.
- `avgDecisionUs = 26` est une mesure Debug du seul appel au moteur de décision ;
  ce n'est pas une latence ni un seuil de production.
- Les 100 initiateurs étaient déjà engagés : `CanAssistTo` a refusé les alliés
  engagés. Les `101` tentatives sont des scans et `alliesEngaged = 0` est
  attendu ; ce n'est pas un test du comportement d'appel à l'aide.
- Non-régression : entrée `38` éligible ; entrée `940` non éligible avec
  `ExcludeEntries = 940` ; entrée `349` non éligible (`ScriptName` non vide) ;
  entrée `1748` non éligible (`CREATURE_TYPE_FLAG_BOSS_MOB`) ; module désactivé,
  décisions, contextes et actions à `0`. Compilation des modules et de
  `worldserver` réussie, symbole loader présent, core intact.
- Le harnais de benchmark temporaire a été retiré ; aucun code de test ne reste
  dans le module.

## 0.5.0 - 2026-10-25

### Ajouté

- Moteur de décision scoré pour les créatures éligibles et opt-in. À chaque
  décision, les quatre tactiques sont évaluées comme candidats : fuite
  `100 + (seuil - vie) * 2` lorsque la vie est inférieure ou égale au seuil,
  protection `60`, appel à l'aide `40` et repositionnement `30`.
- `IndividualIntelligence` multiplie les scores de fuite et de repositionnement
  par `(0.5 + valeur)` ; `GroupIntelligence` multiplie ceux de protection et
  d'appel par `(0.5 + valeur)`. Chaque candidat reçoit une variation aléatoire
  bornée de ±10 %. Les candidats sont triés par score décroissant et la première
  action réussie consomme la décision ; une tentative échouée passe au candidat
  suivant. Si aucun candidat n'agit, `decisionsNoAction` est incrémenté.
- Une fuite en cours appelle le watchdog et consomme la décision sans compter de
  nouvelle action ; les autres tactiques ne sont pas tentées.
- Coordinateur par groupe : clé `mapId` + cellule de 40 yards, fenêtre de 5 s.
  Seuls l'appel à l'aide réussi et l'interception de protection réussie
  sont des actions coopératives, plafonnées à
  `1 + uint32(GroupIntelligence * 2)`. Les soins ne sont pas limités par le
  coordinateur ; la fuite et le repositionnement ne passent jamais par lui.
  L'appel est filtré dans le moteur ; l'interception se filtre dans
  `TryProtectAlly`.
- Nouvelle clé `CombatAI.Reposition.Enable` (bool, défaut `1`). Un lanceur dont un
  sort de `m_spells` atteint au moins 25 yards et dont la cible vivante est à
  moins de 5 yards recule de 15 yards à l'opposé de cette cible via `MovePoint`
  (identifiant `0x4D434152`), avec un cooldown de 5 s par créature. Le
  repositionnement est ignoré pendant un cast, une fuite suivie ou un générateur
  `POINT_MOTION_TYPE`, `FLEEING_MOTION_TYPE` ou `TIMED_FLEEING_MOTION_TYPE` ; il
  ne change ni cible ni menace.
- Nouveaux compteurs : `repositionAttempts`, `actionsRetreat`,
  `actionsProtectAlly`, `actionsCallForHelp`, `actionsReposition` et
  `decisionsNoAction`.
- Diagnostic `scripts.ai` de la tactique retenue avec son score sous
  `CombatAI.Debug`, et résumé `LOG_INFO` des compteurs au plus toutes les 60 s.
- Clé de groupe partagée extraite dans `ModCombatAIGroupKey.h/.cpp` ; le
  comportement du cooldown de groupe de l'appel à l'aide est inchangé.

### Modifié

- `CombatAI.IndividualIntelligence` et `CombatAI.GroupIntelligence` deviennent
  actives : elles pondèrent désormais les scores et le plafond coopératif.
- Le module compte **24 clés de configuration** ; `CombatAI.Reposition.Enable`
  s'ajoute aux 23 clés existantes.
- `.reload config` réinitialise aussi le coordinateur et l'horodatage du résumé
  de diagnostic ; les compteurs cumulés restent en mémoire.

### Notes

- Test de fumée local sur quatre scénarios (A fuite, B protection, C appel,
  D repositionnement) :

  ```text
  mod-combat-ai: enabled (call for help enabled)
  COMBATAI P5 SMOKE: spawned A=(830,831,832) B=(833,834,835) C=(853,854,855) D=(856,857)
  COMBATAI P5 SMOKE T+4s: retreatActions=2 protectActions=2 callActions=1 repositionActions=1 noAction=34 | A_distAlly=0 B_allyHealth=29.069767 C_allyEngaged=true D_distEnemy=15 D_type=0
  COMBATAI P5 SMOKE T+8s: retreatActions=2 protectActions=2 callActions=1 repositionActions=2 noAction=80 | A_distAlly=0 B_allyHealth=100 C_allyEngaged=true D_distEnemy=15 D_type=0
  ```

  Les quatre tactiques sont sélectionnées. A a rejoint son allié
  (`A_distAlly=0`), B est passé de `29.069767` % à `100` %, l'allié de C est
  engagé (`C_allyEngaged=true`) et D est passé de 3 à 15 yards avec `D_type=0`
  (`IDLE_MOTION_TYPE`). `retreatActions` et `protectActions` sont stables entre
  T+4 s et T+8 s ; `repositionActions` passe de 1 à 2. Les compteurs sont agrégés
  sur les quatre scénarios et ne permettent pas d'attribuer chaque action à un
  scénario. Le watchdog de fuite n'ajoute pas de nouvelle action pendant un
  déplacement en cours. Protocole et journal dans `docs/phase-5-test.md`.
- Les scores et la variation ±10 % sont des heuristiques de départ, pas des
  valeurs d'équilibrage validées. Les tests joueurs solo/duo/groupe, les tests de
  charge et les autres types de créatures ne sont pas couverts.
- Aucune modification du core, aucune mise à jour SQL. Aucun changement de cible
  ni de menace artificielle ; les soins ne sont pas limités par le coordinateur.

## 0.4.0 - 2026-10-25

### Ajouté

- Protection des alliés pour les créatures éligibles et opt-in, évaluée après la
  fuite et avant l'appel à l'aide. Au plus une action tactique par décision :
  `TryRetreat`, puis `TryProtectAlly`, puis `TryCallForHelp`.
- Branche soigneur : une créature dont `m_spells` contient un sort positif avec
  `SPELL_EFFECT_HEAL` scanne les alliés dans un rayon fixe de 20 yards (vivant,
  ami, différent du soigneur, même carte, même zone, à portée, ligne de vue, vie
  strictement inférieure à 80 %) et retient celui dont la vie est la plus basse.
  Le cast exige un cooldown de sort à 0, `CheckTarget` égal à `SPELL_CAST_OK`,
  la portée `GetMaxRange(true, créature)` (repli 40 yards) et la ligne de vue. Le
  soin est lancé par `CastSpell(allié, sort, false)` et arme un cooldown de
  1000 ms. `protectAllyAttempts` et `protectAllyHeals` sont incrémentés.
- Branche non-soigneur : recherche d'un allié soigneur ami, vivant, engagé, dans
  un rayon de 20 yards, même carte et même zone, avec ligne de vue et vie
  inférieure ou égale à 60 %. L'attaquant est la victime du soigneur, à défaut la
  première cible vivante et en ligne de `GetSortedThreatList()`. Il doit être
  vivant, hostile à l'initiateur et à portée de 20 yards. L'initiateur reçoit un
  `MovePoint` vers le point milieu soigneur/attaquant (identifiant unique
  `0x4D434150`) puis `EngageWithTarget(attaquant)`, sans `AttackStart` ni
  changement de victime. Cooldown de 5000 ms ; `protectAllyAttempts` et
  `protectAllyIntercepts` sont incrémentés.
- `CombatAI.ProtectAlly.Enable` devient actif : à `1`, les deux branches sont
  disponibles ; à `0`, aucune protection n'est exécutée. Le rayon (20 yards),
  les seuils (80 %, 60 %) et les cooldowns (1 s, 5 s) sont des constantes de
  code, pas des clés. Le module conserve ses 23 clés de configuration.
- Nouveaux compteurs : `protectAllyAttempts`, `protectAllyHeals`,
  `protectAllyIntercepts` et `protectAllyNoTarget`.
- Diagnostics `scripts.ai` sous `CombatAI.Debug` : soin d'un allié avec le sort
  retenu et interception d'un attaquant pour un soigneur en danger.
- Aucun appel direct à `AddThreat`, `ModifyThreat` ou `ScaleThreat` ; aucun
  changement de cible ; aucun sort inventé, appris ou ajouté ; pas de
  `GetCreatureAI`.

### Notes

- Test de fumée local sur deux scénarios (soin d'un allié à 29,069767 % et
  interception d'un mélé vers un soigneur à 50 %) : `protectAllyAttempts=2`
  (`protectAllyHeals=1`, `protectAllyIntercepts=1`), allié remonté à 100 % à
  T+7 s, distance de surface mélé-soigneur 7 → 0, `meleeType=0`
  (`IDLE_MOTION_TYPE`), aucune erreur. `protectAllyNoTarget` passe de 6 à 34
  entre T+3 s et T+7 s pour les créatures éligibles sans candidat ; c'est
  attendu. SmartAI peut aussi soigner : `protectAllyHeals` prouve le cast du
  module, mais la remontée à 100 % n'est pas attribuée exclusivement au module.
  Les tests joueurs solo/duo/groupe, les immunités, les contrôles de foule, les
  compositions de menace, plusieurs soigneurs simultanés, les obstacles, les
  durées de plusieurs minutes et la charge ne sont pas validés ; les constantes
  sont fixées dans le code. Protocole et journal dans `docs/phase-4-test.md`.
- Aucune modification du core, aucune mise à jour SQL. Aucune clé de
  configuration ajoutée ou supprimée.

## 0.3.0 - 2026-10-25

### Ajouté

- Tactique de fuite bornée pour les créatures éligibles et opt-in : déclenchement
  lorsque la vie est inférieure ou égale à `CombatAI.Retreat.HealthThresholdPct`
  (défaut 20), que le cooldown de créature est échu, qu'une cible vivante existe
  (victime ou cible de menace) et que la créature n'est pas déjà en
  `UNIT_STATE_FLEEING`.
- Scan d'un allié destination dans le rayon `CombatAI.Retreat.Radius` : ami,
  vivant, hors evade, même carte, même zone, à portée et ligne de vue vers
  l'initiateur ; l'allié le plus proche est retenu. L'allié n'a pas besoin d'être
  éligible au module ni opt-in.
- Déplacement vers l'allié via `MotionMaster::MovePoint` avec l'identifiant
  unique `0x4D434149`, pour éviter les événements de point de SmartAI. Aucun
  `MoveSeekAssistance`, aucun `SetCombatMovement` et aucun
  `UNIT_STATE_NO_COMBAT_MOVEMENT`.
- Repli `MoveFleeing(ennemi, 3000)` borné à 3 secondes lorsqu'aucun allié
  qualifié n'est trouvé.
- Watchdog du déplacement : suivi des générateurs `POINT_MOTION_TYPE`,
  `FLEEING_MOTION_TYPE` et `TIMED_FLEEING_MOTION_TYPE` (actif et
  `MOTION_SLOT_CONTROLLED`) ; progression de plus d'un yard rafraîchissant le
  minuteur ; après 3 secondes sans progression, `MovementExpired()` et compteur
  `retreatStuck` ; générateur terminé ou remplacé, compteur `retreatCompleted`.
- Cooldown de fuite par créature (`CombatAI.Retreat.CooldownMs`, défaut 60000),
  armé à chaque tentative réelle.
- Priorité de la fuite sur l'appel à l'aide et au plus une action tactique par
  décision : `TryRetreat` d'abord, `TryCallForHelp` seulement si la fuite n'a pas
  consommé la décision.
- Nouveaux compteurs : `retreatAttempts`, `retreatAllies`, `retreatFallbacks`,
  `retreatStuck` et `retreatCompleted`.
- Nouvelle clé `CombatAI.Retreat.Radius` (float, défaut 15, doit être `> 0`). Le
  module compte 23 clés de configuration.
- Diagnostics `scripts.ai` sous `CombatAI.Debug` : destination alliée retenue,
  absence d'allié avec repli, expiration sans progression.

### Notes

- Test de fumée local à trois créatures temporaires : la créature à 10 % de vie
  rejoint l'allié (distance 11 → 0), le point se termine (`completed=1`) avec une
  seule tentative (`attempts=1`), sans blocage (`stuck=0`) ni repli
  (`fallbacks=0`). `activeType=8` correspond à `POINT_MOTION_TYPE`, puis
  `activeType=0` à `IDLE_MOTION_TYPE`. Les tests joueurs solo/duo/groupe, les
  obstacles, le leash et l'evade, ainsi que les durées de plusieurs minutes et la
  charge restent à valider ; le repli `MoveFleeing` n'a pas été exercé. Protocole
  et résultat dans `docs/phase-3-test.md`.
- L'IA native peut changer de cible et remplacer le point de déplacement ; le
  watchdog traite alors la disparition du générateur comme une fin de fuite.
- Aucune modification du core, aucune mise à jour SQL. Les 22 clés existantes
  restent inchangées et `CombatAI.Retreat.Radius` s'ajoute.

## 0.2.0 - 2026-10-25

### Ajouté

- Tactique d'appel à l'aide bornée pour les créatures éligibles et opt-in :
  scan local dans `CombatAI.CallForHelp.Radius`, contrôles natifs
  `CanAssistTo(initiateur, ennemi, false)`, même zone, distance et ligne de vue,
  filtre de phase par `CreatureWorker`, plafond
  `CombatAI.CallForHelp.MaxAllies`.
- Cooldown par créature (`CombatAI.CallForHelp.CooldownMs`, stocké dans
  `callForHelpCooldownEnd`) et cooldown local de groupe indexé par `mapId` et
  cellule de 40 yards, enregistré uniquement après l'engagement effectif d'au
  moins un allié.
- Engagement des alliés via `EngageWithTarget`, encadré par
  `SetNoCallForHelp(true)` puis `SetNoCallForHelp(false)` pour empêcher l'appel
  natif récursif ; le cooldown d'appel de l'allié est renseigné s'il possède déjà
  un contexte, pour éviter les cascades.
- Les alliés ne sont pas soumis à l'éligibilité ni à l'opt-in du module ; seul
  l'initiateur l'est.
- Nouveaux compteurs de diagnostic : `callForHelpAttempts` (un par scan réel),
  `callForHelpAlliesEngaged` (un par allié engagé), `callForHelpGroupSkipped`
  (blocage par le cooldown de groupe) et `callForHelpNoAllies` (scan sans allié
  qualifié).
- Diagnostics `scripts.ai` sous `CombatAI.Debug` : absence d'allié dans le rayon
  et résumé des alliés engagés.
- `.reload config` remet à zéro les cooldowns de groupe, en plus des contextes et
  du budget du tick.

### Notes

- Test de fumée local à trois créatures temporaires : l'allié initialement non
  engagé est engagé par l'appel, et aucune seconde tentative n'est observée sur
  cinq secondes (cooldowns créature et groupe). Les tests joueurs solo/duo/groupe,
  le plafond multi-alliés et la charge restent à valider.
- Une seule action tactique au maximum par décision ; le budget
  `CombatAI.MaxDecisionsPerTick` continue de s'appliquer.
- Aucune autre tactique : fuite, protection d'allié, repositionnement et moteur
  de décision restent à implémenter.
- Aucune modification du core, aucune mise à jour SQL. Les 22 clés de
  configuration existantes sont inchangées.

## 0.1.0 - 2026-10-25

### Ajouté

- Squelette du module `mod-combat-ai`, avec le point d'entrée
  `Addmod_combat_aiScripts()` et la détection automatique des sources par CMake.
- Configuration `CombatAI.*` à 22 clés via `ConfigValueCache`, fichier
  `conf/mod_combat_ai.conf.dist`, valeurs par défaut sûres et rechargement via
  `.reload config`.
- Politique d'éligibilité : humanoïdes, monde ouvert, `ScriptName` vide,
  exclusions des pets, créatures charmées, civiles, en evade, non attaquables,
  boss mondiaux et boss de donjon.
- Activation opt-in par `IncludeEntries` et `IncludeMaps`, avec priorité des
  exclusions `ExcludeEntries` et `ExcludeMaps`.
- Hooks `AllCreatureScript`, `UnitScript` et `WorldScript` pour le cycle de vie du
  contexte de combat, la limitation par intervalle et jitter, et le budget global
  `MaxDecisionsPerTick`.
- Journalisation de l'état du module dans `server.loading` et diagnostics
  `scripts.ai` activables par `CombatAI.Debug`.
- Compteurs de diagnostic en mémoire : décisions évaluées, créatures non
  éligibles, décisions différées par le budget et contextes créés.

### Notes

- Version d'observation seule : aucune tactique de combat n'est exécutée.
- `GetCreatureAI` n'est pas implémenté ; SmartAI et les IA natives ne sont pas
  remplacées.
- Aucune modification du core, aucune mise à jour SQL.
- Module désactivé par défaut (`CombatAI.Enable = 0`) et opt-in strict
  (`CombatAI.OptInOnly = 1`).
