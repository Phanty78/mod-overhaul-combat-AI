# Changelog

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
