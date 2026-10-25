# Phase 6 — benchmark et non-régression

## But

Mesurer le coût du module sur un worldserver local, vérifier que le module
désactivé ne fait aucun travail et que les filtres d'éligibilité répondent comme
prévu. Le benchmark compare une passe OFF (module désactivé) et une passe ON
(module activé), sur le même build, le même harnais et la même charge.

Ce document rapporte uniquement les mesures de la campagne du 2026-10-25. Il ne
définit ni seuil de production ni objectif de performance.

## Environnement

- AzerothCore WotLK 3.3.5a, `worldserver` local.
- Carte 0, aucun joueur connecté (0 joueur).
- Compilation des modules et de `worldserver` réussie ; symbole loader
  `Addmod_combat_aiScripts()` présent ; core non modifié.
- Harnais temporaire **non versionné** : il fait apparaître les créatures, ouvre
  les combats et lit les compteurs. Il est retiré après la campagne ; aucun code
  de test ne reste dans le module.
- Date de la campagne : 2026-10-25.

## Protocole

1. Démarrer `worldserver` avec le module désactivé (`CombatAI.Enable = 0`).
2. Le harnais fait apparaître, à T+20 s :
   - 100 couples initiateur/ennemi, soit 200 créatures ;
   - 4 sondes d'éligibilité (normale, exclue, scriptée, boss).
3. La fenêtre de mesure couvre T+20 s à T+40 s.
4. Les durées de mise à jour sont enregistrées avec les valeurs suivantes de
   `worldserver.conf` :

   ```ini
   RecordUpdateTimeDiffInterval = 10000
   MinRecordUpdateTimeDiff = 0
   ```

5. Relever la ligne de résultat et les synthèses de mise à jour de la passe OFF.
6. Rejouer la même séquence avec le module activé (`CombatAI.Enable = 1`) et
   `CombatAI.Debug = 1`, puis relever les mêmes éléments.
7. Retirer le harnais et vérifier qu'aucun code de test ne subsiste.

Les deux passes utilisent le même build et le même harnais. La passe ON active
l'instrumentation de temps ; en OFF, le module retourne immédiatement et aucun
compteur n'est alimenté.

## Configuration

Clés du module utilisées par la campagne :

| Clé | Passe OFF | Passe ON |
|---|---|---|
| `CombatAI.Enable` | `0` | `1` |
| `CombatAI.ExcludeEntries` | `940` | `940` |
| `CombatAI.Debug` | sans effet | `1` |

Les initiateurs et les quatre sondes sont soumis à la configuration d'éligibilité
du harnais. `CombatAI.ExcludeEntries = 940` retire la sonde exclue. Les valeurs
des autres clés ne sont pas détaillées ici ; la campagne ne rapporte aucune
variation de seuil, de rayon, de cooldown, de poids d'intelligence ou de cadence.

`349` et `1748` restent refusées par le filtre `ScriptName` pour l'une et par le
drapeau boss pour l'autre.

## Résultats

Les lignes `COMBATAI P6 SMOKE RESULT` sont produites par le harnais de mesure ;
le module émet de son côté, sous `CombatAI.Debug = 1`, le résumé
`mod-combat-ai diagnostics` dans `scripts.ai`. Les deux passes rapportent les
mêmes quatre champs d'éligibilité.

### Module désactivé (OFF)

```text
# module OFF
COMBATAI P6 SMOKE RESULT: decisions=0 skippedBudget=0 contexts=0 callAttempts=0 alliesEngaged=0 callActions=0 noAction=0 avgDecisionUs=0 | normal=true excluded=false scripted=false boss=false
```

Synthèses des durées de mise à jour après l'apparition des créatures :

| Statistique | Valeur |
|---|---:|
| Moyenne | 1–2 ms |
| Médiane | 1 ms |
| p95 | 3–7 ms |
| p99 | 4–8 ms |
| Max | 10–25 ms |

### Module activé (ON)

```text
# module ON
COMBATAI P6 SMOKE RESULT: decisions=1584 skippedBudget=436 contexts=636 callAttempts=101 alliesEngaged=0 callActions=0 noAction=1584 avgDecisionUs=26 | normal=true excluded=false scripted=false boss=false
```

Synthèses des durées de mise à jour après l'apparition des créatures :

| Statistique | Valeur |
|---|---:|
| Moyenne | 1 ms |
| Médiane | 1 ms |
| p95 | 2–3 ms |
| p99 | 3–5 ms |
| Max | 4–8 ms |

### Compteurs

Les libellés de la ligne de résultat correspondent aux compteurs ou valeurs
internes suivants :

| Libellé | Compteur ou valeur interne | OFF | ON |
|---|---|---:|---:|
| `decisions` | `decisionsEvaluated` | 0 | 1584 |
| `skippedBudget` | `skippedBudget` | 0 | 436 |
| `contexts` | `contextsCreated` | 0 | 636 |
| `callAttempts` | `callForHelpAttempts` | 0 | 101 |
| `alliesEngaged` | `callForHelpAlliesEngaged` | 0 | 0 |
| `callActions` | `actionsCallForHelp` | 0 | 0 |
| `noAction` | `decisionsNoAction` | 0 | 1584 |
| `avgDecisionUs` | `avgDecisionUs` (dérivé) | 0 | 26 |

`avgDecisionUs` est calculé à partir de `decisionTimeNs` et
`decisionTimeSamples` :

```text
avgDecisionUs = decisionTimeNs / decisionTimeSamples / 1000
```

La division est entière et vaut `0` sans échantillon. Les deux compteurs de temps
ne sont alimentés que si `CombatAI.Debug = 1`, et le résumé n'est émis que dans ce
cas.

## Interprétation

- **Module OFF.** Tous les compteurs sont à zéro : le module retourne
  immédiatement et ne crée aucun contexte, ne consomme aucun budget et n'exécute
  aucune action. Les durées de mise à jour OFF servent de référence.
- **Volume de décisions ON.** 1584 décisions ont été évaluées et 436 ont été
  différées par `CombatAI.MaxDecisionsPerTick`. 636 contextes ont été créés.
- **Appel à l'aide.** Les 100 initiateurs étaient déjà en combat. `CanAssistTo` a
  donc refusé les alliés eux-mêmes engagés : les 101 `callAttempts` sont des
  scans et `alliesEngaged = 0` est attendu. `callActions = 0` est cohérent. Ce
  zéro ne mesure pas le comportement d'appel à l'aide et ne prouve pas qu'un
  allié libre serait refusé. Les 101 scans sont exécutés dans `Decide` et entrent
  donc dans `avgDecisionUs` ; aucun coût de scan séparé n'est mesuré.
- **Absence d'action.** `noAction = 1584` égale `decisions` : dans ce scénario,
  aucun candidat n'a agi. Cela inclut les décisions dont les conditions internes
  ou les cooldowns empêchent l'action ; le benchmark ne cherche pas à distinguer
  ces causes.
- **Temps de décision.** `avgDecisionUs = 26` mesure uniquement l'appel au moteur
  de décision, avec `CombatAI.Debug = 1`. La mesure enveloppe `Decide()` et les
  appels `std::chrono` ne sont exécutés que sous Debug. Cette valeur n'est ni une
  latence de production ni un seuil à respecter.
- **Durées de mise à jour.** Les plages ON (moyenne 1 ms, médiane 1 ms, p95
  2–3 ms, p99 3–5 ms, max 4–8 ms) restent comparables ou inférieures aux plages
  OFF (moyenne 1–2 ms, médiane 1 ms, p95 3–7 ms, p99 4–8 ms, max 10–25 ms).
  Aucune régression mesurable des durées de mise à jour en régime stable n'a été
  observée à cette échelle locale de 200 créatures. Les durées ON incluent la
  journalisation `scripts.ai` et l'instrumentation de temps, actives sous Debug.
  Ce constat ne définit pas de seuil de production et ne doit pas être extrapolé
  à un serveur chargé.
- **Périmètre.** La campagne ne change aucune tactique, aucun score ni aucune
  clé. Le total reste à 24 clés de configuration.

## Non-régression

Les quatre champs `normal`, `excluded`, `scripted` et `boss` de la ligne de
résultat sont les résultats d'éligibilité des quatre sondes. `false` signifie
« non éligible ».

| Sonde | Entrée | Condition | Éligible attendu | Observé |
|---|---:|---|---|---|
| Normale | `38` | aucun filtre | `true` | `true` (`normal=true`) |
| Exclue | `940` | `CombatAI.ExcludeEntries = 940` | `false` | `false` (`excluded=false`) |
| Scriptée | `349` | `ScriptName` non vide | `false` | `false` (`scripted=false`) |
| Boss | `1748` | `CREATURE_TYPE_FLAG_BOSS_MOB` | `false` | `false` (`boss=false`) |

- L'entrée `940` est refusée par `CombatAI.ExcludeEntries = 940`. L'exclusion est
  prioritaire sur l'inclusion dans la politique d'éligibilité.
- Le filtre `ScriptName` écarte `349`, dont le `ScriptName` n'est pas vide.
- Le drapeau boss (`CREATURE_TYPE_FLAG_BOSS_MOB`) écarte `1748`.
- Module désactivé : décisions, contextes et actions restent à `0`.
- La compilation des modules et de `worldserver` réussit, le symbole loader est
  présent et le core n'est pas modifié.

Ces sondes vérifient l'éligibilité, pas le comportement tactique. Les scénarios
tactiques restent couverts par les tests de fumée des phases 2 à 5.

## Non couvert

- Les joueurs en solo, duo ou groupe.
- Les instances au runtime (donjons, raids, BG, arènes).
- Un `.reload config` au runtime : la relecture des 24 clés et la réinitialisation
  des contextes, cooldowns et du budget ne sont pas exercées ici.
- Les durées de plusieurs minutes et le coût CPU en conditions de production.
- Les interactions réelles avec mod-hunts et mod-living-world, y compris les
  proies d'élite, les summons dynamiques et leurs mouvements ou engagements.
- Une charge supérieure à 100 initiateurs/200 créatures.
- Le comportement des quatre tactiques, du coordinateur, des cooldowns, des
  seuils, des rayons et des poids d'intelligence.
- Les autres types de créatures que les humanoïdes.
- Les obstacles, la ligne de vue complexe, le leash, l'evade, les changements de
  phase ou de carte, et les cas limites de distance.
- Une mesure de latence de production : `avgDecisionUs` ne chronomètre que
  l'appel au moteur, sous Debug.

## Checklist de déploiement progressif

1. **Préparer.** Copier `conf/mod_combat_ai.conf.dist` vers
   `env/dist/etc/modules/mod_combat_ai.conf`. Aucune mise à jour SQL n'est
   nécessaire.
2. **Démarrer à froid.** Laisser `CombatAI.Enable = 0`, `CombatAI.OptInOnly = 1`,
   `CombatAI.HumanoidsOnly = 1` et `CombatAI.OpenWorldOnly = 1`. Renseigner
   `CombatAI.IncludeEntries` avec une liste courte d'entrées réelles ;
   `CombatAI.IncludeMaps` reste vide à ce stade.
3. **Activer les diagnostics.** Mettre `CombatAI.Debug = 1` pour obtenir les
   décisions détaillées et le résumé `scripts.ai` (dont `avgDecisionUs`) au plus
   toutes les 60 s.
4. **Activer le module.** Passer `CombatAI.Enable = 1`, puis vérifier la ligne de
   chargement `mod-combat-ai: enabled (...)`.
5. **Surveiller.** Suivre dans `scripts.ai` les décisions évaluées, la tactique
   retenue et son score, les diagnostics des tactiques et `skippedBudget`.
   Contrôler aussi les durées de mise à jour du worldserver.
6. **Élargir par paliers.** Ajouter des entrées à `CombatAI.IncludeEntries`, puis
   des cartes à `CombatAI.IncludeMaps`, un palier à la fois. `CombatAI.IncludeMaps`
   couvre toutes les créatures éligibles de la carte : préférer les entrées.
7. **Exclure les cas tiers.** Avant d'élargir, ajouter les proies de mod-hunts et
   les summons dynamiques de mod-living-world à `CombatAI.ExcludeEntries` ou
   `CombatAI.ExcludeMaps`. Leur `ScriptName` est vide : le filtre `ScriptName` ne
   les écarte pas. L'exclusion est prioritaire sur l'inclusion.
8. **Régler la cadence.** Ajuster `CombatAI.DecisionIntervalMs`,
   `CombatAI.DecisionJitterMs` et `CombatAI.MaxDecisionsPerTick` selon la charge.
   Un `skippedBudget` élevé indique que le budget diffère des décisions.
9. **Régler les tactiques.** Activer ou désactiver sélectivement
   `CombatAI.CallForHelp.Enable`, `CombatAI.Retreat.Enable`,
   `CombatAI.ProtectAlly.Enable` et `CombatAI.Reposition.Enable`. Les paramètres
   associés sont `CombatAI.CallForHelp.MaxAllies`,
   `CombatAI.CallForHelp.Radius`, `CombatAI.CallForHelp.CooldownMs`,
   `CombatAI.Retreat.HealthThresholdPct`, `CombatAI.Retreat.CooldownMs`,
   `CombatAI.Retreat.Radius`, `CombatAI.IndividualIntelligence` et
   `CombatAI.GroupIntelligence`. Les constantes de protection et de
   repositionnement sont fixées dans le code.
10. **Stabiliser.** Repasser `CombatAI.Debug = 0` après validation : les compteurs
    de temps ne sont plus alimentés et aucun résumé périodique n'est émis.
    Continuer à surveiller les durées de mise à jour du serveur.
11. **Recharger avec prudence.** `.reload config` relit les 24 clés et
    réinitialise contextes, cooldowns de groupe, coordinateur, horodatage du
    résumé et budget du tick ; les compteurs cumulés restent en mémoire. Ce
    rechargement n'a pas été testé au runtime pendant la Phase 6 : le valider sur
    un serveur de test avant la production.
12. **Retour arrière.** `CombatAI.Enable = 0` suffit à rendre la main à l'IA
    native ; le module ne remplace ni SmartAI, ni les scripts, ni les
    déplacements natifs.
