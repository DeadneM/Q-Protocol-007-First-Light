# TODO cumulatif — reprise du cœur joueur

- [x] Conserver v0.9.7/main comme base acceptée ; classer A33 REJETÉE.
- [x] Retrouver le personnage local et son propriétaire en histoire et Tac Sim.
- [x] Retirer la dépendance READY au loadout ; conserver celui-ci comme ancrage facultatif.
- [x] Observer sur le thread gameplay ; publier une copie cohérente et datée.
- [x] Identifier les sessions/incarnations ; retirer les demandes de l'ancienne disponibilité.
- [x] Remplacer les payloads/flags séparés par tickets et messages immuables.
- [x] Attendre un résultat munitions corrélé avant de terminer AUTO.
- [x] Garder les E/S disque hors de la section critique native des munitions.
- [x] Ajouter contrôles de portée et fixtures de session/lecture/reset/réponse tardive.
- [x] Compiler A34 et contrôler CI, paquet, manifeste et découverte réelle en bêta.
- [ ] Valider réception AUTO et F2/F3 en histoire puis Tac Sim.
- [ ] Vérifier chargement, mort/rechargement, séquence et retour lobby sans doublons.
- [ ] Confronter les éventuels blocages d'entrée propres aux missions à la définition de disponibilité.
- [ ] Établir une confirmation d'inventaire native ; un spawner au repos ne prouve pas la réception.
- [ ] Reprendre séparément les points DX12/durée de vie, defaults INI partiels et concurrence updater de l'audit.
- [ ] Promouvoir seulement après validation explicite de la candidate.
# 2026-11-04 — suite du double audit

- A35 source `a73f0a0` : build complète et contrôles CI réussis, ZIP contrôlé localement. Voir [état détaillé](A35_SESSION_REPAIR.md). A35 non installée/non validée, stable conservée.
- Vérifier ensuite en jeu la cadence indépendante du menu et le réarmement AUTO après mort ; reload sans changement d'identité reste ouvert.
- Q-Pistol silencieux : les graphes ciblés lus en Tac Sim sont cohérents, mais la livraison n'est pas prouvée. Attendre le résultat F3 réel et établir la preuve de possession ; ne pas changer arbitrairement le RID.
- Garder séparés les travaux transactionnels, UI/GPU et updater du double audit ; aucune promotion main.

## 2026-11-05 — A37

- [ ] Vérifier la cadence F3 et les munitions AUTO dans le lobby Tac Sim avec [A37](A37_WEAPON_CADENCE.md).
- [ ] Vérifier AUTO en mission, l'absence de répétition avec menu/pause et le réarmement après mort.
- [ ] Relier l'objet créé à sa requête et vérifier son rattachement pour le Q-Pistol silencieux, indépendamment de l'avancement de la file.

## 2026-11-05 — mise à jour du jeu

- [x] Sauvegarder et comparer les exécutables des builds 26252352 et 26430236.
- [x] Adapter les adresses et vérifier les gardes natifs A38 sur disque.
- [x] Construire/livrer A38 et vérifier sa découverte par l'updater sur le nouvel EXE.
- [ ] Valider les fonctions en histoire et Tac Sim sur le build 26430236.

## 2026-11-05 — après rejet A38

- [ ] Établir le destinataire et le chemin d'attachement des objets créés, à partir du comportement v0.9.7.
- [ ] Séparer version publique et compteur interne de build dans la prochaine candidate.
- [x] v0.9.8-beta.1 et build interne 39 affichés séparément dans la candidate.
- [x] Observer le job natif et comparer l'objet silencieux vivant à l'inventaire après un F3 contrôlé.
- [ ] Valider en jeu le rattachement natif corrigé, puis AUTO et les munitions.
- [ ] beta.2 : valider chargement menu fermé puis première ouverture du menu après le crash GPU beta.1.
- [x] Le mainteneur rapporte que beta.2 ne crashe plus au cours de son essai.
- [ ] beta.3 : vérifier six alternances F4 avec réception réelle du silencieux et absence de régression de démarrage.
- [x] Compiler, vérifier, publier et installer v0.9.8-beta.1 sur demande, en préservant l'INI et une sauvegarde de la paire précédente.
- [ ] Obtenir une validation fonctionnelle ciblée avant de reprendre les changements expérimentaux de session/détection.

## 2026-11-06 — acquis beta.3 et priorité AUTO

- [x] Le mainteneur valide le ramassage manuel instantané dans ses essais et rapporte un essai en histoire.
- [x] Relevé beta.3 : 13 demandes F4 alternées et deux paquets F3, toutes les réceptions manuelles confirmées dans l'inventaire. Voir [bilan et limites](V098_BETA3_INSTANT_PICKUP.md#2026-11-06--gameplay-checkpoint).
- [ ] Corriger AUTO à la première arrivée : le mainteneur a dû utiliser F3 ; les trois réceptions initiales sont non confirmées, dont deux objets devenus indisponibles avant le pickup. La deuxième session comporte trois réceptions AUTO confirmées.
- [ ] Établir la cause entre initialisation de scène et durée de vie/corrélation du résultat créé, sans délai arbitraire ni répétition aveugle d'un paquet.
- [ ] Vérifier ensuite AUTO après mort/rechargement, son absence de répétition pour menu/pause et les quantités de munitions.
- [ ] Conserver le chemin natif instantané, l'alternance F4 acquittée, le démarrage/menu beta.2 et le profil personnel pendant la correction ciblée ; promotion stable non autorisée par ce seul retour partiel.

## 2026-11-06 — correction AUTO beta.4

- [x] Observer en lecture seule le premier AUTO dans le lobby : les trois objets deviennent invalides sans rejoindre l'inventaire, résultat confirmé par le mainteneur.
- [x] Identifier la branche de finalisation native liée à l'index 0 des trois sources AUTO ; distinguer création, lancer physique et ramassage dans l'inventaire.
- [ ] Construire et contrôler [beta.4](V098_BETA4_AUTO_ARRIVAL.md), puis vérifier ses trois réceptions AUTO au premier lancement sans F3.
- [ ] Confirmer en jeu que la finalisation ordinaire demeure intacte pour les objets du jeu et que F3/F4 gardent leur livraison instantanée.
