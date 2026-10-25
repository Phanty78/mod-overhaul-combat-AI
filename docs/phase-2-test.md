# Phase 2 — protocole de test de fumée

## But

Vérifier de bout en bout qu'un initiateur éligible déclenche l'appel à l'aide
borné, qu'un allié non engagé est sélectionné et engagé, et qu'aucune seconde
tentative n'est émise pendant le cooldown.

## Environnement

- Serveur AzerothCore WotLK compilé depuis le HEAD du module, carte 0, données
  DBC/maps/VMaps/MMaps complètes.
- Base de test ou serveur local sans joueur connecté.
- Harnais temporaire **non versionné** : un `WorldScript` de test spawn trois
  créatures après 20 s et lit les compteurs 5 s plus tard. Il est retiré du module
  après l'exécution.

## Scénario

| Rôle | Entrée | Faction | Position (map 0) |
|---|---:|---:|---|
| Initiateur | 38 (Defias Thug) | 17 | `-9233.27, 271.076, 72.9248` |
| Allié | 38 (Defias Thug) | 17 | `-9247.27, 271.076, 72.9248` |
| Ennemi | 68 (Stormwind City Guard) | 11 | `-9228.27, 271.076, 72.9248` |

- Les factions sont forcées après le spawn pour garantir allié amical, ennemi
  hostile.
- `SetNoCallAssistance(true)` sur l'initiateur neutralise l'assistance native du
  core ; l'ennemi est passé en `REACT_PASSIVE` et le combat est ouvert par
  `Attack(ennemi, true)` pour éviter un déplacement avant la décision.
- L'allié est initialement non engagé ; il est à 14 mètres de l'initiateur et
  19 mètres de l'ennemi, dans une zone ouverte avec ligne de vue.

## Configuration appliquée

```ini
CombatAI.Enable = 1
CombatAI.HumanoidsOnly = 1
CombatAI.OpenWorldOnly = 1
CombatAI.OptInOnly = 1
CombatAI.IncludeEntries = 38
CombatAI.DecisionIntervalMs = 500
CombatAI.DecisionJitterMs = 0
CombatAI.CallForHelp.Enable = 1
CombatAI.CallForHelp.MaxAllies = 2
CombatAI.CallForHelp.Radius = 15
CombatAI.CallForHelp.CooldownMs = 30000
CombatAI.MaxDecisionsPerTick = 64
CombatAI.Debug = 1
```

Les autres clés conservent leurs valeurs par défaut. Sur le serveur local dont le
`CONF_DIR` compilé n'est pas inscriptible, les valeurs ont été fournies par les
variables d'environnement `AC_COMBAT_AI_*` correspondantes.

## Résultat observé — 2026-10-25

```text
mod-combat-ai: enabled (call for help enabled)
COMBATAI SMOKE: spawned caller=829 ally=830 enemy=831 allyInitiallyEngaged=false
COMBATAI SMOKE RESULT: callerEngaged=true allyEngaged=true allyTargetsEnemy=true
  attempts=1 engaged=1 noAllies=0 groupSkipped=0
```

Interprétation :

- l'allié n'était pas engagé au spawn ;
- il est engagé sur l'ennemi après la décision du module ;
- `engaged=1` confirme la sélection et l'engagement par la tactique ;
- `attempts=1` après 5 s confirme que le cooldown de créature (30 s) et/ou le
  cooldown de groupe bloque une nouvelle tentative ;
- aucune erreur ni assertion dans le journal du serveur.

## Non couvert par ce test

- Combats avec joueurs en solo, duo et groupe de cinq à six.
- Plafond `MaxAllies` avec plus de deux alliés qualifiés.
- Comportement sur plusieurs minutes, charge avec des dizaines/centaines de
  créatures, et interactions réelles avec mod-hunts/mod-living-world.
- Obstacles complexes, changements de phase et cas de leash/evade en conditions
  réelles.
