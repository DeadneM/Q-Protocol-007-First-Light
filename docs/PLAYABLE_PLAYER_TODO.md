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

- [x] Contrôler les 31 RIDs dans les fichiers installés et recalculer leurs hashes depuis les chemins nommés ; aucun remap arbitraire.
- [x] Construire A36, contrôler les contrats d'inventaire et publier la beta/manifeste accessible depuis l'updater ; paquet public vérifié.
- [ ] Installer avec rollback/préservation INI, puis valider réception et AUTO histoire/Tac Sim ; A36 reste EXPÉRIMENTALE.
- [ ] Traiter les limites de résultat trop bref, de copie fusionnée/remplacée et d'AUTO Ammo après paquet partiel. Conserver ouverts reload sans changement d'identité et les autres constats du double audit.

- A35 source `a73f0a0` : build complète et contrôles CI réussis, ZIP contrôlé localement. Voir [état détaillé](A35_SESSION_REPAIR.md). A35 non installée/non validée, stable conservée.
- Vérifier ensuite en jeu la cadence indépendante du menu et le réarmement AUTO après mort ; reload sans changement d'identité reste ouvert.
- Q-Pistol silencieux : les graphes ciblés lus en Tac Sim sont cohérents, mais la livraison n'est pas prouvée. Attendre le résultat F3 réel et établir la preuve de possession ; ne pas changer arbitrairement le RID.
- Garder séparés les travaux transactionnels, UI/GPU et updater du double audit ; aucune promotion main.
