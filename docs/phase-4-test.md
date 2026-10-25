# Phase 4 — protocole de test de fumée

## But

Vérifier de bout en bout qu'un soigneur éligible lance un sort de soin réel sur un
allié blessé et qu'un non-soigneur éligible se porte vers le point milieu entre
un soigneur allié en danger et son attaquant, en ajoutant une référence de combat
à zéro menace. Le test vérifie aussi que les compteurs restent cohérents avec une
action par branche, que les soins, interceptions et absences de cible sont
distingués, et que l'état observé ne montre pas de répétition sur la fenêtre
T+0/T+7 s.

## Environnement

- Serveur AzerothCore WotLK local (`worldserver`), carte 0, sans joueur connecté
  (0 joueur).
- Harnais temporaire **non versionné** : un `WorldScript` de test spawn les deux
  scénarios, configure les factions, la vie et le mode passif, ouvre le combat,
  puis lit les compteurs à T+0, T+3 s et T+7 s. Il est retiré du module après
  l'exécution ; aucun artefact de code de test ne reste dans le module.
- Les deux scénarios sont séparés de plus de 150 yards sur la carte 0 et
  n'interagissent pas.
- Les factions sont forcées après le spawn : `17` pour les créatures amies
  (soigneurs, allié, mélé) et `11` pour les ennemis.
- Les ennemis et les alliés sont passés en `REACT_PASSIVE`.
  `SetNoCallAssistance(true)` neutralise l'assistance native du core et le combat
  est ouvert par `Attack(ennemi, true)` pour éviter un déplacement avant la
  décision.

## Scénarios

### Scénario A — soin d'un allié blessé

Zone de spawn (map 0) : `-9233.27, 271.076, 72.9248`.

| Rôle | Entrée | Faction | Détail |
|---|---:|---:|---|
| Soigneur (initiateur) | 940 | 17 | Éligible et opt-in, en combat, porteur d'un sort de soin réel. |
| Allié blessé | 38 | 17 | Vie ramenée à environ 30 % (`29.069767` relevé). Cible du soin. |
| Ennemi | 68 | 11 | Passif, sert de cible de combat au soigneur. |

Le soigneur est mis en combat contre l'ennemi passif. L'allié blessé est à portée,
dans la même zone et en ligne de vue. Aucun sort n'est créé ni ajouté par le
harnais : la branche soigneur ne peut utiliser qu'un sort déjà présent dans
`m_spells`.

### Scénario B — interception d'un attaquant

Zone de spawn (map 0) : `-9390.82, 286.729, 62.639`.

| Rôle | Entrée | Faction | Détail |
|---|---:|---:|---|
| Mélé (initiateur) | 38 | 17 | Non-soigneur, éligible et opt-in, en combat. |
| Soigneur allié | 940 | 17 | Vie ramenée à 50 %, porteur d'un sort de soin réel, engagé. |
| Ennemi 1 | 68 | 11 | Passif. |
| Ennemi 2 | 68 | 11 | Passif. |

L'écart de spawn entre le mélé et le soigneur est de 10 yards ; la distance de
surface relevée par le module est de 7. Le soigneur est engagé (`IsEngaged()`)
pour que la branche non-soigneur le retienne ; sa victime si elle est non nulle,
à défaut la première cible vivante de sa liste de menace triée, sert d'attaquant.
Le mélé et le soigneur allié sont mis en combat par `Attack(ennemi, true)`. Les
deux ennemis sont présents dans la zone du scénario.

## Configuration appliquée

Les valeurs ont été fournies par les variables d'environnement
`AC_COMBAT_AI_*` correspondantes :

| Clé | Variable d'environnement | Valeur |
|---|---|---|
| `CombatAI.Enable` | `AC_COMBAT_AI_ENABLE` | `1` |
| `CombatAI.IncludeEntries` | `AC_COMBAT_AI_INCLUDE_ENTRIES` | `940,38` |
| `CombatAI.ProtectAlly.Enable` | `AC_COMBAT_AI_PROTECT_ALLY_ENABLE` | `1` |
| `CombatAI.CallForHelp.Enable` | `AC_COMBAT_AI_CALL_FOR_HELP_ENABLE` | `0` |
| `CombatAI.Retreat.Enable` | `AC_COMBAT_AI_RETREAT_ENABLE` | `0` |
| `CombatAI.DecisionIntervalMs` | `AC_COMBAT_AI_DECISION_INTERVAL_MS` | `500` |
| `CombatAI.DecisionJitterMs` | `AC_COMBAT_AI_DECISION_JITTER_MS` | `0` |
| `CombatAI.Debug` | `AC_COMBAT_AI_DEBUG` | `1` |

Les autres clés conservent leurs valeurs par défaut : `HumanoidsOnly = 1`,
`OpenWorldOnly = 1`, `OptInOnly = 1`, `MaxDecisionsPerTick = 64`. L'appel à
l'aide et la fuite sont désactivés pour isoler la protection des alliés. Les
constantes de la Phase 4 (rayon de 20 yards, seuils de 80 % et 60 %, cooldowns de
1 s et 5 s) ne sont pas configurables et n'ont pas été modifiées.

## Résultat observé — 2026-10-25

```text
mod-combat-ai: enabled (call for help disabled)
COMBATAI P4 SMOKE: spawned healer=642 ally=643 enemy=644 melee=645 healerB=646 enemyB=647 enemyB2=648 allyHealth=29.069767 meleeDistHealer=7
COMBATAI P4 SMOKE T+3s: allyHealth=29.069767 meleeType=0 meleeDistHealer=0 attempts=2 heals=1 intercepts=1 noTarget=6
COMBATAI P4 SMOKE T+7s: allyHealth=100 meleeType=0 meleeDistHealer=0 attempts=2 heals=1 intercepts=1 noTarget=34
```

## Interprétation

- La ligne de chargement `enabled (call for help disabled)` reflète la
  configuration : module actif, appel à l'aide désactivé. La protection des
  alliés est active via `CombatAI.ProtectAlly.Enable = 1`, sans apparaître dans
  cette ligne.
- À T+0, le harnais a spawné le scénario A (soigneur `642`, allié `643`, ennemi
  `644`) et le scénario B (mélé `645`, soigneur `646`, ennemis `647` et `648`).
  `allyHealth=29.069767` correspond à l'allié ramené à environ 30 %.
  `meleeDistHealer=7` est la distance de surface pour un écart de spawn de
  10 yards.
- À T+3 s, `attempts=2` correspond à une action par branche : `heals=1` pour le
  soin et `intercepts=1` pour l'interception. `meleeDistHealer=0` indique que le
  mélé a rejoint la zone du soigneur. `meleeType=0` correspond à
  `IDLE_MOTION_TYPE` : aucun déplacement du module n'est actif au relevé.
- À T+3 s, `allyHealth` reste à `29.069767` alors que `heals=1` : le soin a été
  commandé et compté, mais son effet n'est pas encore visible dans l'échantillon.
  À T+7 s, l'allié est à `100`.
- À T+7 s, `attempts=2`, `heals=1` et `intercepts=1` restent inchangés. L'allié
  n'est plus sous le seuil de 80 %, donc aucun nouveau soin n'est retenu. Aucune
  nouvelle interception n'est comptée : la branche est soit encore sous son
  cooldown de 5 s, soit déjà en attaque sur l'attaquant retenu
  (`GetVictim() == attaquant`).
- `noTarget=6` à T+3 s puis `34` à T+7 s croît à chaque décision d'une créature
  éligible qui arrive au scan sans allié, soigneur ou attaquant qualifié, ou sans
  sort de soin lançable. C'est attendu : un soigneur ne peut pas se cibler
  lui-même et les créatures sans candidat ne produisent aucune action. Ce
  compteur n'indique pas une erreur.
- Le journal observé ne contient aucune erreur ni exception pendant le test.
- `protectAllyHeals=1` prouve qu'un `CastSpell` du module a été exécuté.
  SmartAI peut aussi lancer ses propres soins ; le test ne mesure pas la part
  exacte du soin du module dans la remontée de 29,069767 % à 100 %.

## Non couvert par ce test

- Les combats avec joueurs en solo, duo et groupe de cinq à six.
- Les immunités, les contrôles de foule et les compositions de menace complexes,
  notamment le repli sur `GetSortedThreatList()` lorsque le soigneur n'a pas de
  victime.
- Plusieurs soigneurs ou plusieurs alliés blessés dans le même rayon.
- Les obstacles et les ruptures de ligne de vue ; le test se déroule en ligne de
  vue dégagée.
- Les interactions réelles avec SmartAI, mod-hunts et mod-living-world, y compris
  un soin natif concurrent ou un changement de cible de l'IA native.
- Les durées de plusieurs minutes, la charge avec des dizaines ou des centaines
  de créatures, et les changements de phase ou de carte.
- La sensibilité des constantes : le rayon de 20 yards, les seuils de 80 % et
  60 %, et les cooldowns de 1 s et 5 s sont fixes et n'ont pas été variés.
- Le comportement lorsque `CombatAI.ProtectAlly.Enable = 0` ; le test ne couvre
  que la valeur `1`.
