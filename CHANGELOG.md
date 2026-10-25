# Changelog

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
