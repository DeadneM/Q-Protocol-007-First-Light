# Audit ASI — seconde passe : invariants et contre-exemples

Même source A34 que la première passe. Relecture depuis les effets vers leurs conditions d'autorisation, puis recherche de contre-exemples. Il s'agit d'une seconde passe du même auditeur, pas d'un audit indépendant. Les modèles ne lisent ni ne modifient le jeu.

## Vérifications croisées

1. **Disponibilité** : une observation âgée de 501 ms devient fausse alors que l'identité reste identique. Le modèle ancré dans le code reproduit cette possibilité. F01 confirmé au niveau logique ; les bascules du log sont compatibles avec ce mécanisme sans prouver chaque occurrence.
2. **AUTO** : le contrat confirmé par l'utilisateur est une application à l'arrivée en mission et après mort/rechargement, jamais à l'ouverture/fermeture du menu ou pause. L'observation booléenne ne porte pas la raison de l'indisponibilité. Les fixtures changeant manuellement l'incarnation ne démontrent pas que le moteur le fait à chaque mort/reload. F02 confirmé comme contrat incomplet ; il faut observer un événement natif fiable, pas assimiler chaque false à une nouvelle mission.
3. **Livraison** : remonter de COMPLETE mène à accepted/idle, pas à l'inventaire. F04 confirmé ; le retour Q-Pistol est un échec fonctionnel réel. L'absence de preuve de contexte dans le graphe (F03) reste une piste, pas une cause établie. Ne pas remplacer une incertitude par un autre RID au hasard.
4. **Exactement une demande** : après acceptation, un timeout ne signifie pas annulation native. Une nouvelle tentative peut dupliquer un effet tardif (F05). L'annulation d'un ticket protège sa réponse, pas l'effet déjà soumis au jeu.
5. **Origine GUI** : nouveau F13. La commande catalogue transmet un RID seul ; sa session est attribuée plus tard par le worker. Un clic avant transition peut donc devenir une demande dans la session suivante. Le ticket doit conserver la provenance dès l'intention, puis être rejeté si elle devient périmée.
6. **Cohérence du profil** : nouveau F14. Les armes sont copiées au départ ; les munitions AUTO utilisent ensuite g_autoAmmo courant. Un changement sauvegardé pendant l'opération peut mélanger deux profils. Capturer une seule révision complète par transaction.
7. **Concurrence UI** : g_uiLoaded est un bool non atomique lu/écrit par des threads différents sans verrou partagé à toutes les écritures (OverlayA19.cpp:186, 2593, 5165). F08 confirmé comme course de données. La visibilité atomique du menu ne protège pas une écriture ultérieure de ce bool.
8. **Durée de vie GPU** : le chemin font-upload peut sortir après ExecuteCommandLists puis échec de Signal/attente et libérer uploadBuffer/pTexture sans preuve de fin GPU (backend:583–623). F09 confirmé sur ce chemin d'erreur. Aucun lien démontré avec le timeout FrameFence observé après initialisation réussie.
9. **Falsification d'une suspicion** : indices frontend/backbuffer et backend/frameIndex différents ne suffisent pas à démontrer une réutilisation dangereuse. Le modèle teste 32 768 séquences, 2/3 buffers, 14 presents : aucun contre-exemple sous ses hypothèses (même taille de pool, fences monotones valides, arrêt sur erreur). Retirer cette suspicion des défauts confirmés ; ce n'est pas une preuve GPU générale.
10. **Updaters et cycle de vie** : F11/F12 persistent à la relecture des chemins d'exception. Ils doivent être corrigés séparément des armes. L'échec du pin, l'absence de réconciliation post-sortie et les résultats d'écriture ignorés demandent aussi des statuts explicites. Aucun de ces chemins n'explique à lui seul le Q-Pistol manquant.

## Bilan des moyens de preuve

`tools/audit_a34_models.py` contrôle ses points d'ancrage dans le commit figé et écrit AUDIT_MODELS.json. Il montre des possibilités logiques, pas des mesures de fréquence ni une exécution native. Les hashes de SOURCE_MANIFEST.json fixent le périmètre. Les index de fonctions peuvent contenir des opérateurs extraits imparfaitement ; ils ne remplacent pas l'analyse des contrats.

Aucune arme, réserve, transition de mort, mise à jour post-sortie ni stabilité GPU n'a été validée en jeu par cet audit. Aucun binaire, INI personnel, installation ou branche stable n'est modifié.
