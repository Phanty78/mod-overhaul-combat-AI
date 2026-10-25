# Phase 3 — protocole de test de fumée

## But

Vérifier de bout en bout qu'un initiateur éligible dont la vie est sous le seuil
déclenche la fuite bornée, sélectionne l'allié le plus proche comme destination,
exécute un `MovePoint` natif, puis libère l'état de déplacement lorsque le point
se termine. Le test vérifie aussi l'absence de repli, de blocage et de seconde
tentative pendant le cooldown.

## Environnement

- Serveur AzerothCore WotLK local (`worldserver`), carte 0, sans joueur
  connecté (0 joueur).
- Harnais temporaire **non versionné** : un `WorldScript` de test spawn trois
  créatures, les configure, puis lit l'état du déplacement et les compteurs à
  T+0, T+2 s et T+6 s. Il est retiré du module après l'exécution ; aucun
  artefact de code ne reste dans le module.

## Scénario

| Rôle | Entrée | Faction | Position (map 0) |
|---|---:|---:|---|
| Initiateur | 38 (Defias Thug) | 17 | `-9233.27, 271.076, 72.9248` |
| Allié | 38 (Defias Thug) | 17 | `-9247.27, 271.076, 72.9248` |
| Ennemi | 68 (Stormwind City Guard) | 11 | `-9228.27, 271.076, 72.9248` |

- Les factions sont forcées après le spawn pour garantir allié amical, ennemi
  hostile.
- La vie de l'initiateur est ramenée à environ 10 % (`healthPct=9.302325` dans le
  journal), sous le seuil de 50 % configuré.
- `SetNoCallAssistance(true)` sur l'initiateur neutralise l'assistance native du
  core ; l'ennemi et l'allié sont passés en `REACT_PASSIVE`, et le combat est
  ouvert par `Attack(ennemi, true)` pour éviter un déplacement avant la décision.
- L'allié est à 14 yards de l'initiateur, dans une zone ouverte avec ligne de
  vue. La distance initiale relevée par le module est de 11 : il s'agit de la
  distance de surface pour un écart de 14 yards entre les positions.

## Configuration appliquée

```ini
CombatAI.Enable = 1
CombatAI.HumanoidsOnly = 1
CombatAI.OpenWorldOnly = 1
CombatAI.OptInOnly = 1
CombatAI.IncludeEntries = 38
CombatAI.DecisionIntervalMs = 500
CombatAI.DecisionJitterMs = 0
CombatAI.CallForHelp.Enable = 0
CombatAI.Retreat.Enable = 1
CombatAI.Retreat.HealthThresholdPct = 50
CombatAI.Retreat.Radius = 15
CombatAI.Retreat.CooldownMs = 60000
CombatAI.MaxDecisionsPerTick = 64
CombatAI.Debug = 1
```

Les autres clés conservent leurs valeurs par défaut. Sur le serveur local dont le
`CONF_DIR` compilé n'est pas inscriptible, les valeurs ont été fournies par les
variables d'environnement `AC_COMBAT_AI_*` correspondantes.

## Résultat observé — 2026-10-25

```text
mod-combat-ai: enabled (call for help disabled)
COMBATAI P3 SMOKE: spawned caller=642 ally=643 enemy=644 healthPct=9.302325 distAlly=11
COMBATAI P3 SMOKE T+2s: callerHealthPct=9.302325 distAlly=3.2086048 moduleMoving=true activeType=8 controlledType=19 attempts=1 allies=1 fallbacks=0 stuck=0 completed=0
COMBATAI P3 SMOKE T+6s: callerHealthPct=9.302325 distAlly=0 moduleMoving=false activeType=0 controlledType=19 attempts=1 allies=1 fallbacks=0 stuck=0 completed=1
```

Interprétation :

- La ligne de chargement `enabled (call for help disabled)` reflète la
  configuration : module actif, appel à l'aide désactivé.
- À T+0, la vie est à 9,302325 %, sous le seuil de 50 %, et l'allié est à 11
  (distance de surface pour 14 yards).
- À T+2 s, `activeType=8` correspond à `POINT_MOTION_TYPE` : le `MovePoint` vers
  l'allié est actif. `moduleMoving=true`, la distance est tombée à 3,2086048 :
  la créature s'approche. `attempts=1` et `allies=1` confirment une seule
  tentative, résolue par la branche alliée ; `fallbacks=0`, `stuck=0` et
  `completed=0`.
- `controlledType=19` correspond à `NULL_MOTION_TYPE`, l'emplacement contrôlé
  restant vide : le déplacement utilise l'emplacement actif.
- À T+6 s, `distAlly=0` : l'allié est rejoint. `activeType=0` correspond à
  `IDLE_MOTION_TYPE`, le point est terminé. Le watchdog a remis
  `moduleMoving=false` et incrémenté `completed=1`.
- `attempts=1` reste inchangé à T+6 s : le cooldown de 60 s empêche une nouvelle
  tentative. Aucun blocage (`stuck=0`) ni repli (`fallbacks=0`) n'est observé.
- La vie reste à 9,302325 % pendant le test : l'ennemi passif n'inflige aucun
  dégât.

## Non couvert par ce test

- Le repli `MoveFleeing` sans allié : le test dispose d'un allié qualifié et
  `fallbacks=0`.
- Les obstacles, le leash et l'evade en conditions réelles, ainsi que les
  changements de phase.
- Les combats avec joueurs en solo, duo et groupe de cinq à six.
- Les tests de plusieurs minutes, la charge avec des dizaines/centaines de
  créatures et les interactions réelles avec mod-hunts/mod-living-world.
- Le remplacement du point par l'IA native lors d'un changement de cible ; le
  watchdog est conçu pour le traiter comme une fin de fuite, mais ce cas n'a pas
  été exercé ici.
