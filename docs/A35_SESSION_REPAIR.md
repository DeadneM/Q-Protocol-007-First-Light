# A35 — correction ciblée de session (travail expérimental)

Base : A34 `13a843a`, avec documentation de double audit `55853e9`. Stable v0.9.7 conservée ; A34 n'est pas une validation globale du gameplay. Le WIP `236fcdb` est repris explicitement, sans les anciennes expériences rejetées.

## Objectif et changements

Retirer la dépendance de l'observation à la cadence du hook. Le worker devient le seul producteur de sessions via des lectures protégées ; le hook ne publie plus de session et revérifie identité, fraîcheur et disponibilité avant l'appel natif et sa réponse. Aucun appel natif n'est déplacé sur le worker. Le masque TLS est lu et contrôlé sur chaque thread, sans valeur forcée de secours : un TLS indisponible ferme l'autorisation.

Une demande non encore déclenchée reste en attente du même ticket au lieu d'être abandonnée après deux secondes. Les transitions réelles annulent toujours les tickets ; aucun timeout après acceptation native n'est résolu par ce changement.

La présence confirmée du propriétaire dans la liste native not-alive arme un marqueur de mort. Le retour jouable crée une seule nouvelle session, même si l'identité native est conservée. Une nouvelle identité consomme aussi ce marqueur, sans double session. Une séquence, une lecture impossible ou la visibilité du menu ne constituent pas une mort. L'indisponibilité conserve le refus des actions ; une transaction partiellement appliquée n'est jamais rejouée aveuglément.

## Contrôles et limites

Fixtures ajoutées : absence prolongée du callback, dispatch retardé avec le même ticket, refus d'identité différente et d'observation périmée, séquence/lecture absente sans nouvel AUTO, mort puis même identité, remplacement après mort sans double session. Le lecteur natif distingue désormais séquence et preuve not-alive. Workflow dédié aux tests, sans publication de release.

**État : CONTRÔLÉ pour les deux exécutables de tests synthétiques.** CI [41832406109](https://github.com/DeadneM/Q-Protocol-007-First-Light/actions/runs/41832406109), source `57316365ba5a2d19a659fc6850ce2b23f68c9e09` : compilation MSVC et exécution des contrats natifs/session réussies, job `126582613475`. Aucun compilateur C++ local trouvé. Ce contrôle ne vaut ni compilation complète de l'ASI ni validation en jeu.

Restent ouverts : reload conservant l'identité sans mort observée, mort trop brève pour le polling, preuve exhaustive de contrôle effectif, possession du Q-Pistol, contexte des spawners, timeout après acceptation, GUI/session, profil complet AUTO, UI/GPU et updater. Ne pas prétendre que tout le contrat AUTO est déjà couvert : il manque un événement natif fiable pour les reloads non distinguables par ces observations.

## Critère de réussite et rollback

Tests synthétiques réussis puis compilation complète et test utilisateur histoire/Tac Sim : aucune bascule artificielle liée au menu ; aucun AUTO supplémentaire sur menu/pause ; un AUTO après arrivée et mort/reload, sans duplication. Q-Pistol à vérifier séparément, pas annoncé réparé. Ne pas installer avant une build complète vérifiée et autorisation ; conserver les deux binaires précédents et l'INI personnel. Pas de modification des gardes PE/préimages, des raccourcis, du canal beta par défaut ni de main. Le rollback de code est la branche d'audit avant reprise A35 ; le rollback installé reste le paquet précédemment conservé.
