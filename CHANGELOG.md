# Changelog

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
