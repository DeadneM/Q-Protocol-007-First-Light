# Fil rouge Q Protocol — reprise après rejet A36

État au 4 novembre 2026, après la dernière instruction du mainteneur. Document de continuité pour ce projet/chat ; lire avec PROJECT_STATE.md avant toute nouvelle modification.

## Décision actuelle, prioritaire

**Ne rien restaurer pour l'instant, même jeu fermé.** Le mainteneur veut poursuivre la réparation, pas revenir en arrière sur son installation ce soir. A36 reste installée et REJETÉE. Aucun rollback n'a été exécuté. La release A36 a été conservée en brouillon ; retrait vérifié par le workflow `41906266428` et par le véritable moteur d'updater (A36 n'est plus proposée depuis build 34 beta). Les sauvegardes et traces privées restent disponibles. Pas de nouveau lancement, installation, build de gameplay ou promotion main pendant cette remise à plat.

L'objectif n'est pas d'empiler A37 sur un comportement A36 réputé acquis : comprendre les différences depuis la référence où le lobby fonctionnait, conserver les comportements utiles et corriger leur point de rupture. La reprise des changements de code est pour la séance suivante, avec ce fil rouge disponible.

## Référence et étalage des commits

Référence de comparaison : **v0.9.7, `8722000d859eb9785a8906cae82adc72d35ae1a2`**, acceptée après A28/A29. `main` contient ensuite le commit documentaire `62f75211256c498562a1727a966c27d551dccfb9`. Le signalement historique « fonctionne seulement dans le lobby, pas en mission Tac Sim » est consigné dans `26e8284`, après les précisions `0562194`. Cette preuve porte sur le fonctionnement/reconnaissance rapportés, pas sur une validation de toutes les armes ou de tous les modes. Ne pas réécrire le passé en disant que Tac Sim était validé universellement.

| Étape | Commit de code/livraison | Changement réel | Statut et implication |
|---|---|---|---|
| A26/A27 intégrées | `06245ba` | Queue DX12 autoritative et scan d'adresses x64 | Acquis à préserver ; incident GPU distinct toujours ouvert |
| A28 | `580e57c` | Remap individuel EXE novembre et gardes natifs | Acceptée ensuite avec A29 ; RIDs non remappés à cette étape |
| A29 | `17d70a4`, compilation corrigée `e04b7de` | Menu/updater, paquet deux binaires | Acceptée par le mainteneur |
| v0.9.7 | `c5948c1` puis `8722000` | Promotion autorisée, beta OFF par défaut | Base acceptée avant signalement Tac Sim ; référence lobby |
| A31 | `7cd008c`, publication updater `33f3dde` | Raisons/diagnostic de résolution ; primitives conservées | Observation, pas réparation. `b39b851` est une variante locale contenant des traces privées, pas la base publique à reprendre |
| A32 | `1d04050` | Resolver de secours pour handle vide, helper appelé avec l'ID global +30 | Échec Tac Sim ; code non retenu comme acquis |
| Audit/identités A33 | `43b18a3`, code `5e2e721` | Lookup loadout par propriétaire +34/+1AC au lieu du runtime ID +30/+1A8 | Différence d'identités prouvée ; détection A33 rejetée par le mainteneur |
| A34 | `13a843a`, suivi `b125d95` | Nouveau lecteur de personnage, session/epoch et mailboxes ; disponibilité liée au callback gameplay | Certaines actions fonctionnent en Tac Sim, silenced absent et AUTO/menu fragiles. Pas acceptée globalement |
| A35 | WIP local `236fcdb`, reprise `453cfaa`, mort `54d3fdd`/`5731636`, build `a73f0a0` | Worker producteur d'observation ; hook revalide et exécute ; mort confirmée séparée | Compilée/contrôlée, jamais validée seule en jeu. Ne pas conclure que son observer est validé parce qu'A36 compile |
| Double audit et RIDs | `55853e9`, `66c91ec` | Deux passes comparées ; 31 identités présentes et hashes recalculés | Contrôles, pas nouvelle fonctionnalité |
| A36 | logique `124fa75`, build `97cdf00`, publication `648d810` | Nouveau gate de livraison, lecture d'inventaire, ramassage conditionnel, terminalisation des échecs | **REJETÉE** après régression sévère. Ne pas garder le gate/délai comme acquis |
| Installation puis retrait | installation suivie `6299113`, retrait `01883f5`/`34cdbd9` | Paire A36 installée sur demande, puis release retirée en brouillon | Installation physique inchangée après rejet, sur nouvelle instruction du mainteneur |

Les parents Git ont été vérifiés : **A31 (`7cd008c`), A32 (`1d04050`) et A34 (`13a843a`) ont toutes trois `62f7521` comme parent direct.** A34 ne cumule donc pas le resolver A32/A33. A33 appartient à la ligne A32 ; A35/A36 poursuivent A34. Ne pas tirer une causalité d'une simple succession de numéros.

[Empreintes des fichiers et parents exacts par version](audits/20261104/COMMIT_LINEAGE.json). La table distingue commits source, corrections CI, publication et documentation ; seuls les premiers changent le gameplay.

## Ce que le mod doit faire

1. Vérifier le jeu pris en charge et les préimages avant de toucher au code natif. Pas de contournement des gardes.
2. Identifier le personnage local actuel et une disponibilité pertinente. Un loadout existant, un ID non nul ou un menu affiché ne suffisent pas à prouver que Bond est contrôlable.
3. Les actions manuelles utilisent ce contexte actuel et le même chemin natif que leur équivalent AUTO.
4. **AUTO : une fois à l'arrivée en mission et après mort/rechargement ; aucune répétition causée par ouverture/fermeture du menu ou pause.** Règle explicitement confirmée par le mainteneur. Les reloads conservant l'identité sans mort observée ne sont pas encore distingués de façon fiable.
5. Préserver la réactivité et les armes déjà fonctionnelles. Une observation expérimentale de réception ne doit pas bloquer toute la file ni empêcher les munitions sans preuve que ce nouveau gate fonctionne.
6. Garder séparés « soumis », « traité par le spawner » et « reçu dans l'inventaire ». Ne pas appeler COMPLETE une réception non prouvée ; ne pas transformer non plus un lecteur non validé en condition bloquante de toute l'expérience.

## Fonctions : but, responsabilité et limites

| Fonction/ensemble | But réel | Exécution et ce qu'elle ne prouve pas |
|---|---|---|
| Proxy DXGI, bootstrap, ValidateTargetExecutable/NativeMap | Charger ASI/DXGI système et refuser un EXE incompatible | Initialisation ; compilation/PE corrects ne prouvent pas gameplay |
| ResolveNativeLoadout/ResolvePlayer v0.9.7 | Obtenir loadout typé et runtime ID, autoriser READY | Worker ; fonctionne dans les contextes rapportés du lobby, retourne vide dans le Tac Sim défaillant |
| ReadNativePlayer A34+, ObserveWorkerPlayer A35 | Lire personnage local, référence/génération, propriétaire et prédicats vivant/séquence | Lecture ; « vivant et hors séquence » reste un contrat partiel de contrôle réel |
| PlayableSession | Mémoriser session, epoch, phases AUTO | Politique de cycle de vie ; aucune autorité pour inventer mission/reload à partir d'un simple false |
| QueueWeapon/StartLoadoutPackage | Recevoir l'intention, capturer le profil, séquencer QPistol/OneHanded/TwoHanded | Worker ; F3, F4 et catalogue doivent partager la même mécanique, pas trois moteurs divergents |
| BuildGraphIndex/ResolveGraph/ValidateGraph | Associer RID à ItemEntry/Spawner trouvé en mémoire | Worker ; présence, type et RID ne prouvent ni cible ni livraison. Premier spawner par RID/paire de source restent à examiner |
| BeginWeaponRequest, GameplayMailbox | Transmettre une commande cohérente ; empêcher écrasement et réponse périmée | Worker → hook ; un ticket valide ne prouve pas l'effet dans l'inventaire |
| QpGameplayTick/ExecuteSpawn | Revalider le contexte et appeler le spawn natif | Callback gameplay uniquement ; observer le retour au repos signifie fin de traitement, pas réception |
| ProcessWeaponQueue/Complete/Fail | Gérer temps, résultat, cooldown et passage au rôle suivant | Worker ; c'est ici qu'A36 a imposé un gate non opérationnel et introduit les retards |
| QueueAmmoProfile/PublishAmmoProfileGameplayThread | Publier les réserves additives par classe via entrée native | Worker → hook, verrou natif ; ne pas confondre publication et mesure de réserve |
| AUTO dans WorkerThread | Appliquer profil armes puis munitions selon un événement de session défini | Politique commune ; A36 marque Weapons Failed et empêche alors AUTO Ammo, régression visible |
| F4 | Choisir une variante puis passer par QueueWeapon | Pas un moteur Q-Pistol distinct. L'alternance avance actuellement à l'enqueue, pas à la réception ; défaut séparé |
| OverlayPump/WndProc/rendu | Affichage, configuration, intention utilisateur et propriété des entrées | UI ; filtrage des raccourcis quand le menu capte les touches intentionnel, AUTO n'est pas désactivé par une condition « menu visible » |
| LoadConfig/Save INI | Respecter préférences et raccourcis existants | Ne pas réinitialiser l'INI pour masquer une panne ; résultat des écritures et snapshot complet du profil restent à améliorer séparément |
| Updater | Découvrir paquet compatible, préserver INI et remplacer après sortie | Canal/paquet contrôlés distincts de fonctionnalité native ; beta OFF par défaut, stable séparée |

Les problèmes DX12, durée de vie des ressources GPU, concurrence UI et updater du double audit restent des sujets séparés. Ne pas les invoquer comme explication du silencieux absent sans mesure causale.

## Ce qui est établi, et ce qui ne l'est pas

- **Établi dans les fichiers** : 31/31 RIDs publics et installés existent comme TEMP, correspondent aux chemins nommés et à leur hash recalculé. Aucun changement de catalogue entre A26/A27, v0.9.7 et la ligne actuelle. `AssassinRifle` correspond à exotic_harpoon : nom à traiter avec migration, pas changement arbitraire de RID.
- **Établi par le joueur avant A36** : F3 en Tac Sim donne les deux autres armes du profil mais pas le silencieux ; F4 seul donne le Q-Pistol normal, pas le silencieux ; d'autres armes manquent aussi. Ne pas demander au joueur de tester tout le catalogue pour commencer le travail.
- **Lecture seule avant A36** : références de sortie conservées et valides pour normal/silencieux et témoins, mais celle du silencieux absente de la liste du joueur ; composants ZFirearmCharacterEntity et IItem identifiés. Ces références venaient de capacité historique du vecteur : elles ne constituent pas un accusé attribué à une requête propre.
- **Établi après A36** : logs du lobby avec PLAYER PLAYABLE, puis timeout de confirmation sur chacun des trois rôles AUTO, AUTO PACKAGE FAILED et absence de phase Ammo ; F3 occupé pendant des attentes successives et mêmes timeouts manuels. Ce cas n'est pas un simple NOT READY.
- **Non démontré** : le ramassage natif A36 répare un objet. Le log rejeté ne contient pas sa ligne « Created item absent ... native pickup queued ». L'échec se situe avant une publication de ramassage observée. Ne pas modifier le mode/événement au hasard pour compenser cette absence.
- **Hypothèse principale à départager** : résultat créé trop bref pour le lecteur de lot logique, ou autre échec de sa chaîne de lecture. Le code A36 attend malgré cet état inconnu puis le transforme en échec. La capture manquée est plausible, pas une cause prouvée par instrumentation dans ce log.

## Reprise demain : ordre de travail

1. Relire ce document et les instructions actuelles. **Aucun rollback/installation sans nouvelle instruction.** A36 rejetée reste physiquement présente à la demande du mainteneur.
2. Confronter les fonctions réellement modifiées à v0.9.7, puis A34 → A35 → A36. Les empreintes/Git montrent que UI, hook ASM, config/RIDs et moteur updater ne sont pas la même famille de changements que disponibilité/queue/livraison.
3. Isoler au niveau du code les changements rejetés d'A36 : gate de confirmation, attentes par rôle et blocage d'AUTO Ammo. Ne pas effectuer de rollback physique ni livrer une nouvelle build sous prétexte de faire cette comparaison.
4. Établir un mécanisme d'observation du résultat attaché à la bonne requête, sans adopter un ancien objet ni rendre les armes fonctionnelles dépendantes d'une lecture encore non validée. Définir la cible du spawner et la responsabilité du rattachement avant d'ajouter un autre appel natif.
5. Définir une correction cohérente et un résultat observable, conserver les protections utiles (commandes immuables, générations, réponse périmée refusée), puis seulement construire une candidate ciblée avec rollback disponible et essai court histoire/Tac Sim. Ne pas refaire une grande réécriture de disponibilité pour masquer une panne de livraison déjà PLAYER PLAYABLE.

Les tests de compilation/modèles sont nécessaires mais ne constituent pas l'acceptation. Seul le résultat concret contrôlé avec le mainteneur permet de marquer VALIDÉ ; un diagnostic reste un diagnostic.
