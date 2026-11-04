# Q Protocol A36 — Native Delivery TEST

**EXPÉRIMENTAL — compilation et contrôles réussis, validation en jeu encore requise.**

Base : stable v0.9.7 préservée ; correction de session A35 puis livraison commune A36. Les 31 RIDs du catalogue et de l'INI installé ont été retrouvés dans les packages du jeu, rapprochés de Bond-Hashes et recalculés. Aucun RID n'a été remplacé au hasard.

A36 suit la référence créée pour sa propre requête et vérifie sa présence dans la liste native du joueur. Si l'objet est absent et tous les contrôles passent, elle publie une demande native de ramassage unique sur le hook gameplay. F3, F4, raccourcis, catalogue et AUTO utilisent ce même chemin. Une publication n'est pas une confirmation de réception ; COMPLETE nécessite désormais la présence de l'objet dans la liste du joueur.

Un résultat périmé/inconnu ou un timeout après action native ne relance pas aveuglément la création. Les rôles suivants restent traités ; le paquet finit en échec partiel si nécessaire. Les vérifications PE/préimages sont renforcées. A35 retire la dépendance de l'observation à la cadence du hook et distingue mort native confirmée et indisponibilité temporaire.

Préservés : main/v0.9.7, RIDs, raccourcis, préférences personnelles, INI existant, rendu/menu et updater avec beta OFF par défaut. ZIP : exactement dxgi.dll + QProtocol.asi à la racine. README et INI par défaut restent sur GitHub.

Objectif du test : réception réelle des armes du profil via F3, variantes Q-Pistol via F4, puis AUTO histoire/Tac Sim, menu ouvert/fermé, arrivée et mort/rechargement. Le contrôle des identités des 31 armes ne dépend pas d'une campagne manuelle du joueur. Un résultat créé trop brièvement pour être capturé peut rester non confirmé ; les copies fusionnées/remplacées par le jeu également. Un paquet partiel ne publie pas AUTO Ammo. Les reloads sans mort observée/changement d'identité et les défauts UI/GPU/updater du double audit restent ouverts.

Accès : activer Beta builds, Check now, puis Update on exit et relancer soi-même. La publication de cette beta n'installe rien et ne lance pas le jeu. Aucun succès de gameplay n'est revendiqué avant retour utilisateur.

Rollback jeu fermé : restaurer les deux binaires sauvegardés (ou le paquet précédent vérifié) et conserver QProtocol.ini. Le canal stable ne rétrograde pas automatiquement un numéro beta supérieur. Rapports détaillés : docs/A36_NATIVE_DELIVERY.md et docs/audits/20261104/WEAPON_IDENTITIES.md.
