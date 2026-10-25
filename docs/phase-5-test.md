# Phase 5 — protocole de test de fumée

## But

Vérifier de bout en bout que le moteur de décision sélectionne les quatre
tactiques sur quatre scénarios indépendants : fuite (A), protection des alliés
(B), appel à l'aide (C) et repositionnement (D). Le test vérifie aussi que les
compteurs de sélection restent cohérents entre deux relevés, que le watchdog
d'une fuite en cours ne crée pas de nouvelle action, que le repositionnement
termine son déplacement et que les journaux ne contiennent pas d'erreur.

## Environnement

- Serveur AzerothCore WotLK local (`worldserver`), carte 0, sans joueur connecté
  (0 joueur).
- Harnais temporaire **non versionné** : un `WorldScript` de test spawn les
  quatre scénarios, force les factions, la vie, les états de réaction et ouvre le
  combat, puis lit les compteurs à T+4 s et T+8 s. Il est retiré du module après
  l'exécution ; aucun artefact de code de test ne reste dans le module.
- Les quatre zones sont séparées de plus de 150 yards sur la carte 0 et
  n'interagissent pas.
- Les factions sont forcées après le spawn : `17` pour les créatures amies et
  `11` pour les ennemis.
- Les initiateurs reçoivent `SetNoCallAssistance(true)` pour neutraliser
  l'assistance native du core, et le combat est ouvert par `Attack(ennemi, true)`.
  Les créatures non initiatrices sont passives, sauf l'allié de C, configuré
  agressif pour que l'appel puisse l'engager.
- Les entrées `38` et `940` sont opt-in. Les alliés de A, B et C appartiennent
  donc à des entrées éligibles ; lorsqu'ils sont engagés, ils peuvent produire
  leurs propres décisions. Les compteurs sont agrégés sur les quatre scénarios.
  L'ennemi `68` n'est pas opt-in.

## Scénarios

### Scénario A — fuite

Zone de spawn (map 0) : `-9233.27, 271.076, 72.9248`.

| Rôle | Entrée | Faction | Position (map 0) | Détail |
|---|---:|---:|---|---|
| Initiateur (fuite) | 38 (Defias Thug) | 17 | `-9233.27, 271.076, 72.9248` | Vie ramenée à environ 10 %. |
| Allié destination | 38 (Defias Thug) | 17 | `-9247.27, 271.076, 72.9248` | À 14 yards à l'ouest, passif. |
| Ennemi | 68 (Stormwind City Guard) | 11 | `-9228.27, 271.076, 72.9248` | À 5 yards à l'est, passif. |

L'initiateur est mis en combat contre l'ennemi passif. Sa vie est à 10 %, sous le
seuil de fuite configuré à 50 %. L'allié est dans le rayon de fuite, dans la même
zone et en ligne de vue.

### Scénario B — protection des alliés

Zone de spawn (map 0) : `-9390.82, 286.729, 62.639`.

| Rôle | Entrée | Faction | Position (map 0) | Détail |
|---|---:|---:|---|---|
| Soigneur (initiateur) | 940 (Kurzen Medicine Man) | 17 | `-9390.82, 286.729, 62.639` | Porteur d'un sort de soin réel. |
| Allié blessé | 38 (Defias Thug) | 17 | `-9387.82, 286.729, 62.639` | À 3 yards (`x + 3`), vie à environ 30 %. |
| Ennemi | 68 (Stormwind City Guard) | 11 | `-9384.82, 286.729, 62.639` | À 6 yards (`x + 6`), passif. |

Le soigneur est mis en combat contre l'ennemi passif. L'allié blessé est dans le
rayon de protection, dans la même zone et en ligne de vue. Aucun sort n'est créé
ni ajouté par le harnais : la branche soigneur ne peut utiliser qu'un sort déjà
présent dans `m_spells`.

### Scénario C — appel à l'aide

Zone de spawn (map 0) : `-9483.68, 547.621, 53.2887`.

| Rôle | Entrée | Faction | Position (map 0) | Détail |
|---|---:|---:|---|---|
| Mélé (initiateur) | 38 (Defias Thug) | 17 | `-9483.68, 547.621, 53.2887` | En combat, opt-in. |
| Allié appelé | 38 (Defias Thug) | 17 | `-9497.68, 547.621, 53.2887` | À 14 yards (`x - 14`), agressif. |
| Ennemi | 68 (Stormwind City Guard) | 11 | `-9478.68, 547.621, 53.2887` | À 5 yards (`x + 5`), passif. |

Le mélé est mis en combat contre l'ennemi passif. L'allié est à 14 yards, dans le
rayon d'appel, dans la même zone, en ligne de vue et agressif ; il peut donc être
engagé par `EngageWithTarget`.

### Scénario D — repositionnement

Zone de spawn (map 0) : `-9559.39, 217.842, 56.78`.

| Rôle | Entrée | Faction | Position (map 0) | Détail |
|---|---:|---:|---|---|
| Lanceur (initiateur) | 940 (Kurzen Medicine Man) | 17 | `-9559.39, 217.842, 56.78` | Possède un sort dont la portée atteint au moins 25 yards. |
| Ennemi | 68 (Stormwind City Guard) | 11 | `-9556.39, 217.842, 56.78` | À 3 yards (`x + 3`), passif. |

Le lanceur est mis en combat contre l'ennemi passif. La distance initiale entre
les deux créatures est de 3 yards, sous le seuil de déclenchement de 5 yards. La
vérification de portée porte sur tout sort de `m_spells` dont la portée maximale
atteint 25 yards ; un sort de soin à portée suffit, le module n'exige pas un sort
offensif.

## Configuration appliquée

Les valeurs ont été fournies par les variables d'environnement
`AC_COMBAT_AI_*` correspondantes :

| Clé | Valeur |
|---|---|
| `CombatAI.Enable` | `1` |
| `CombatAI.HumanoidsOnly` | `1` |
| `CombatAI.OpenWorldOnly` | `1` |
| `CombatAI.OptInOnly` | `1` |
| `CombatAI.IncludeEntries` | `"38,940"` |
| `CombatAI.DecisionIntervalMs` | `500` |
| `CombatAI.DecisionJitterMs` | `0` |
| `CombatAI.IndividualIntelligence` | `0.5` |
| `CombatAI.GroupIntelligence` | `0.5` |
| `CombatAI.CallForHelp.Enable` | `1` |
| `CombatAI.Retreat.Enable` | `1` |
| `CombatAI.Retreat.HealthThresholdPct` | `50` |
| `CombatAI.ProtectAlly.Enable` | `1` |
| `CombatAI.Reposition.Enable` | `1` |
| `CombatAI.MaxDecisionsPerTick` | `64` |
| `CombatAI.Debug` | `1` |

Les autres clés conservent leurs valeurs par défaut : notamment
`CombatAI.CallForHelp.MaxAllies = 2`, `CombatAI.CallForHelp.Radius = 15`,
`CombatAI.CallForHelp.CooldownMs = 30000`, `CombatAI.Retreat.Radius = 15` et
`CombatAI.Retreat.CooldownMs = 60000`. Les constantes de protection (20 yards,
80 %, 60 %, 1 s, 5 s) et de repositionnement (5 yards, 15 yards, 25 yards, 5 s)
ne sont pas configurables et n'ont pas été modifiées. Le seuil de fuite est
configuré à 50 % ; la vie de l'initiateur de A est ramenée à environ 10 %.

## Résultat observé — 2026-10-25

```text
mod-combat-ai: enabled (call for help enabled)
COMBATAI P5 SMOKE: spawned A=(830,831,832) B=(833,834,835) C=(853,854,855) D=(856,857)
COMBATAI P5 SMOKE T+4s: retreatActions=2 protectActions=2 callActions=1 repositionActions=1 noAction=34 | A_distAlly=0 B_allyHealth=29.069767 C_allyEngaged=true D_distEnemy=15 D_type=0
COMBATAI P5 SMOKE T+8s: retreatActions=2 protectActions=2 callActions=1 repositionActions=2 noAction=80 | A_distAlly=0 B_allyHealth=100 C_allyEngaged=true D_distEnemy=15 D_type=0
```

## Interprétation

- La ligne de chargement `enabled (call for help enabled)` reflète la
  configuration : module actif et appel à l'aide activé. Elle ne liste pas les
  trois autres tactiques ; `Retreat.Enable`, `ProtectAlly.Enable` et
  `Reposition.Enable` sont actifs via leurs clés respectives.
- À T+0, le harnais a spawné le scénario A (GUID `830`, `831`, `832`), le
  scénario B (`833`, `834`, `835`), le scénario C (`853`, `854`, `855`) et le
  scénario D (`856`, `857`).
- `A_distAlly=0` à T+4 s indique que l'initiateur de A a rejoint son allié : la
  fuite a été sélectionnée et son déplacement est terminé au relevé.
- `B_allyHealth` vaut encore `29.069767` à T+4 s, puis `100` à T+8 s : la vie de
  l'allié de B remonte entre les deux relevés, ce qui indique un soin. Le compteur
  agrégé `protectActions=2` prouve au moins une sélection de protection réussie
  du module ; SmartAI peut aussi lancer ses propres soins, donc la remontée à
  100 % n'est pas attribuée exclusivement au module.
- `C_allyEngaged=true` aux deux relevés indique que l'allié de C est engagé.
  `callActions=1` confirme une sélection réussie de l'appel à l'aide.
- `D_distEnemy=15` à T+4 s et à T+8 s, pour une distance initiale de 3 yards,
  indique que le lanceur de D a reculé jusqu'à la distance visée de 15 yards.
  `D_type=0` correspond à `IDLE_MOTION_TYPE` : aucun générateur de déplacement du
  module n'est actif au moment du relevé, le point est terminé.
- Les quatre tactiques ont donc été sélectionnées au moins une fois. Les
  compteurs sont agrégés sur les quatre scénarios et ne permettent pas
  d'attribuer chaque sélection à un scénario précis.
- `retreatActions=2`, `protectActions=2` et `callActions=1` sont stables entre
  T+4 s et T+8 s. Une fuite encore suivie par le watchdog ne produit pas de
  nouvelle sélection : le watchdog entretient le déplacement, il ne compte pas
  comme une action.
- `repositionActions` passe de `1` à `2` entre T+4 s et T+8 s, alors que D reste
  à 15 yards et que son générateur est `IDLE_MOTION_TYPE` aux deux relevés. Le
  journal agrégé ne permet pas d'identifier la créature à l'origine de la seconde
  sélection ni d'en déduire un mouvement supplémentaire de D au moment du
  relevé.
- `noAction` passe de `34` à `80`. Ce compteur s'incrémente pour les décisions
  arrivées au calcul des candidats et pour lesquelles aucun candidat n'a agi, par
  exemple à cause d'un cooldown, d'une condition interne non remplie ou de
  l'absence de cible qualifiée ; une fuite en cours est traitée avant ce calcul
  et n'incrémente pas ce compteur. Sa croissance est attendue avec les créatures
  opt-in et un intervalle de décision de 500 ms ; elle ne signale pas une erreur.
- Les lignes observées ne contiennent aucune erreur.

## Non couvert par ce test

- La sensibilité des scores : aucune variation de `IndividualIntelligence` ou de
  `GroupIntelligence` en dehors de `0.5`, aucun test à `0` ou `1`, et aucune
  mesure de l'effet de la variation aléatoire ±10 % sur l'ordre des candidats.
- La saturation du budget coopératif : plusieurs appels ou interceptions dans la
  même cellule de 40 yards et la même fenêtre de 5 s, y compris aux valeurs
  extrêmes de `GroupIntelligence`.
- L'exemption des soins vis-à-vis du plafond coopératif, avec plusieurs soigneurs
  ou plusieurs alliés blessés.
- Les combats avec joueurs en solo, duo et groupe de cinq à six.
- Les obstacles, les ruptures de ligne de vue, le leash, l'evade et les
  changements de phase ou de carte.
- Les bords de carte, les destinations de repositionnement sans chemin valide et
  les cas où la cible est exactement à 5 yards ou le sort exactement à 25 yards.
- Les interactions réelles avec SmartAI, mod-hunts et mod-living-world, y compris
  un soin natif concurrent, un changement de cible ou un remplacement du
  déplacement par l'IA native.
- Les durées de plusieurs minutes, la charge avec des dizaines ou des centaines
  de créatures, et les benchmarks.
- Les autres types de créatures que les humanoïdes, ainsi que les créatures avec
  `ScriptName`, les pets, les créatures charmées et les boss.
