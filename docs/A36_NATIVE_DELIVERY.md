# A36 — livraison native commune (expérimental)

**REJETÉE par le mainteneur après essai réel.** Régression sévère : AUTO lobby Tac Sim absent, manuel retardé et silencieux toujours absent. Les logs confirment les timeouts de confirmation sur chaque rôle, le blocage F3 pendant le paquet et AUTO sans munitions. Les sections de conception et contrôles ci-dessous sont historiques ; elles ne décrivent pas une fonctionnalité validée. Retrait du canal beta et rollback A34 préparé, jeu fermé/confirmation requis. Ne pas conserver le nouveau gate comme acquis.

**Instruction ultérieure prioritaire : aucun rollback pour l'instant, même jeu fermé.** Le mainteneur demande une remise à plat des commits/fonctions puis correction ; voir [FIL_ROUGE_MOD.md](FIL_ROUGE_MOD.md). A36 reste physiquement installée sur sa demande, sans être considérée acquise. Retrait beta vérifié, pas de nouvelle build.

Base de source : A35 `a73f0a0`, compile et contrats contrôlés ; A35 n'est pas validée en jeu. Base stable conservée : v0.9.7. Contrôle des 31 RIDs : audit `66c91ec`, aucun remplacement d'identifiant. Branche `experiment/a36-native-delivery`. Pas d'installation automatique.

## Objectif

Ne plus confondre création traitée et arme reçue. Même pipeline pour F3, F4, raccourcis, catalogue et AUTO : capturer le résultat propre à la demande ; vérifier son rattachement au joueur courant ; utiliser le ramassage natif une seule fois s'il manque, puis confirmer le rattachement. Pas de cas spécial Q-Pistol et pas de valeur forcée de TLS/compatibilité.

## Changements

- Capture du lot logique créé par l'appel natif et de sa référence source. Une ancienne entrée conservée dans la capacité d'un vecteur vidé n'est jamais adoptée comme nouvelle réponse. Les clés/générations et la source doivent correspondre.
- Lecture bornée des tables de handles et de l'interface IItem, avec rapprochement de l'identifiant dans le registre natif ; distinction entre inventaire inconnu, objet absent et présent. Contrôles de stabilité des tables/listes, y compris modification des IDs en place.
- Ramassage via mailbox sur le hook gameplay, revérification du personnage, de l'objet et de l'absence avant publication. Entrée native `{playerId,itemId,mode=0}` (mode par défaut), sans répétition. La fin de publication n'est jamais une confirmation de livraison.
- COMPLETE seulement après présence de l'objet créé dans la liste native du joueur. Un timeout après entrée dans le natif ne relance pas la création. Un rôle non confirmé n'empêche pas les rôles suivants ; le paquet finit explicitement en échec partiel et AUTO ne rejoue pas le paquet.
- Une demande de ramassage encore en attente du hook peut attendre comme le spawn A35 ; le délai de confirmation démarre après sa réponse. Changement de session : annulation et refus des réponses périmées.
- Gardes de compatibilité supplémentaires sur les lectures et le callsite de ramassage. Les gardes A28/A34, RVAs acceptés, raccourcis, defaults/INI, rendu et updater sont conservés. Aucun nouveau hook ni appel natif sur le worker.

## Contrats natifs identifiés

Sur l'EXE SHA-256 `6e63df4d2891ea085a71ef9a6f4d24b9db52144879945e8cc1a2e8106fabd3f1` :

| Contrat | Source native |
|---|---|
| Validation handle, pages de 100 | `004EA2D0` |
| Résolution référence par ID | `01337F40`, manager global `06454008`, table `+90` |
| Cast d'interface | `007D6E80` |
| IItem et son ID | `015D7D0D`, descripteur `067924E0`, champ `+4A` |
| Objets rattachés par runtime player ID | `015D8E60`, root `06452BF8`, `+38`, table `+1A0` |
| Résultat créé et référence source | callback `016B5E40`, enregistrement de 56 octets, référence résultat `+8`, source `+18` |
| Ramassage | `015D7E56` : owner `06452BE0`, verrou `+238E0`, insertion `001157B0` dans `+10A68`, événement `F3` via `012A8470`, contexte `+109C8` |

Ce sont des identifications statiques et des comparaisons en lecture seule, pas la preuve qu'une publication de ramassage réussira dans tous les contextes.

## Vérification attendue et limites

Fixtures : refus des générations périmées, handles libres/malformés, interface absente, inventaire illisible/changeant, ancienne capacité, source/tag modifiés et lots ambigus. Compilation complète et contrôles de périmètre/paquet obligatoires avant essai.

À valider par le joueur : réception réelle et absence de duplication avec F3/F4, AUTO à l'arrivée et après mort, histoire et Tac Sim ; menu ouvert/fermé. On ne demande pas une campagne manuelle sur les 31 armes pour établir les identités : ce contrôle a été fait dans les fichiers.

Limites connues : un résultat asynchrone trop bref pour être capturé pendant son lot logique reste non confirmé ; aucune adoption de mémoire historique pour masquer cette limite. Réception d'une copie déjà remplacée ou fusionnée par le jeu peut également rester non confirmée. Un paquet partiel échoue explicitement, et son AUTO Ammo n'est pas publié ; ne pas présenter ce cas comme succès complet. Les reloads sans mort observée ni changement d'identité restent ouverts, ainsi que les constats UI/GPU/updater du double audit. Ne pas annoncer le correctif validé avant l'essai réel.

## Livraison et rollback

ZIP candidat : uniquement dxgi.dll + QProtocol.asi à la racine, sans INI/README ; documentation ici. En l'absence de validation en jeu, aucune promotion main. Conserver A34/A35 et les deux binaires installés pour retour arrière ; préserver l'INI personnel. L'autorisation d'installation est distincte de cette compilation.

## Résultats contrôlés

Source figée `97cdf00f1ee4ebcf0004749104bb1a2cee46ef43`. [Compilation complète 41894755836](https://github.com/DeadneM/Q-Protocol-007-First-Light/actions/runs/41894755836), job `126801189202` : succès. 14 contrôles de lecture du joueur, 56 session/mailbox, 25 inventaire, 19 updater ; menu/bootstrap sur trois dimensions, proxy DXGI natif, dépendances, configuration et périmètre contrôlés. La première tentative `41894510452` a été arrêtée par un avertissement traité en erreur dans les fixtures, corrigé sans changer la logique de livraison.

[Publication 41895495682](https://github.com/DeadneM/Q-Protocol-007-First-Light/actions/runs/41895495682) : succès, sans reconstruction ; le paquet testé est republié à l'identique. [Prerelease A36](https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/tag/a36-native-delivery-test), tag pointant sur la source figée, stable/main inchangées. Le véritable moteur d'updater trouve A36 depuis build 34 avec beta activée, télécharge et contrôle le paquet public, ne le réoffre pas à build 36 et l'exclut du canal stable. Extraction dans le workspace uniquement : aucun lancement, installation ou test de sortie du jeu.

[ZIP direct](https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/download/a36-native-delivery-test/Q-Protocol_A36_NativeDelivery_TEST.zip), 598404 octets, SHA-256 `41cbf51c9945d01cbbe14a32f21bb7dc2bbebaabb87b497d72b7214c7b843796`.

- QProtocol.asi : 1037312 octets, SHA-256 `9a5d711aff3bef3b3534073f10e348528b74a76b4ee6c333aeb4799352b6a6b4`.
- dxgi.dll : 138240 octets, SHA-256 `9ec6c22c8c307f90e9e25c0b1e48e7a60fc7ca5b6c03bb3e0f9d0b5e58344340`.

ZIP téléchargé, CRC/racine exacte et PE AMD64 contrôlés localement ; empreintes identiques à la CI et au paquet public. A34 reste installée (ASI `88af6995…`). Statut **CONSTRUIT / CONTRÔLÉ / EXPÉRIMENTAL**, pas encore VALIDÉ par l'utilisateur. Essai ciblé et rollback prêt nécessaires avant de conclure.

## Installation demandée — 4 novembre 2026

A36 installée après confirmation explicite du mainteneur et contrôle jeu/updater fermés. Ancienne paire A34, INI personnel et logs sauvegardés et vérifiés localement avant remplacement. Les hashes des deux binaires installés correspondent exactement à la release A36 ; le hash de l'INI est identique avant/après. Aucun lancement du jeu ni validation en jeu effectués. Le relevé privé conserve le chemin de rollback ; la mention A34 ci-dessus décrit l'état avant cette installation.
