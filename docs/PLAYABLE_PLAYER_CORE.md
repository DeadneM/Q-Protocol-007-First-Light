# A34 — joueur jouable, session et commandes communes

## Base et verdicts

Base de production : v0.9.7, commit `8722000d859eb9785a8906cae82adc72d35ae1a2`.
Branche de travail créée depuis main : `experiment/a34-playable-player-core`.
A31 est un diagnostic ; A32 et A33 sont rejetées comme corrections fonctionnelles.
Leurs fallbacks de loadout ne sont pas repris dans A34.

Objectif : un seul état du personnage local autorise AUTO et les commandes
manuelles, avec des demandes et résultats rattachés à la bonne session.
Réussite : le personnage contrôlable est reconnu en histoire et en Tac Sim,
AUTO s'applique une fois pour son incarnation et F2/F3 répondent aux demandes ;
chargement, mort, séquence et changement de personnage ne réutilisent aucune
demande de l'ancienne session. La réception des armes doit être constatée en jeu.

## Pourquoi A33 ne suffit pas

Le dernier log confirme l'ASI A33 installé. Il contient des packages AUTO et
manuels traités, plusieurs identités et des pertes de readiness. Le retour
utilisateur rejette son comportement. Un loadout persistant ne représente pas
le personnage contrôlé ; corriger son identifiant ne répare pas ce contrat.

## Chaîne native établie

EXE de novembre : SHA-256
`6e63df4d2891ea085a71ef9a6f4d24b9db52144879945e8cc1a2e8106fabd3f1`.

1. `ZCLGetLocalPlayerCharacter` (`0x015BD210`) lit l'instance à `0x05D1B188`.
2. Son implémentation native conserve à `+0x18` une référence de personnage
   avec index/génération, et à `+0x28` son composant typé. Le producteur est
   `0x015BCCD0`, avec mise à jour du binding par `0x015BCF80`.
3. La version humanoïde (`0x015D52A0`, instance `0x05D1B190`) donne la même
   référence dans les relevés contrôlables. Le composant est bien de type
   `ZHumanoidCharacterEntity` dans ces relevés histoire et Tac Sim.
4. `ZCLGetPlayerIDFromCharacter` établit le champ propriétaire `component+0x200`.
   Il correspond au `+0x1AC` du `ZPlayerSpawnerEntity` courant ; son `+0x1A8`
   fournit l'identifiant des primitives natives d'armes/munitions.
5. `ZCLIsPlayerInSequence` recherche le propriétaire dans root `0x06452BF8`
   → `+0x108` → vecteur `+0x40/+0x48`. `ZCLIsHumanoidAlive` exclut les identités
   présentes dans root → `+0x08` → vecteur `+0x48/+0x50`.

Les captures ont été effectuées en lecture seule, sans injection ni écriture
dans le jeu. La chaîne personnage/propriétaire et ses générations sont observées
en histoire et Tac Sim ; la liste de séquences est aussi observée en histoire.
Le filtre de vie provient du code natif et de fixtures ; sa nouvelle lecture
n'a pas été capturée en jeu, le processus ayant été fermé entre-temps.
Les captures brutes et logs restent hors Git. Un nom de classe seul n'a pas
servi de preuve de fonctionnement : les producteurs, champs et lectures ont
été suivis et les valeurs ont été confrontées aux relevés.

## Nouveau fonctionnement

Le callback gameplay existant observe le personnage avec le masque TLS natif,
vérifie génération/type du getter, propriétaire non ambigu et stabilité des
structures. Les listes natives vie/séquence ferment l'autorisation. Les lectures
sont bornées et toute observation incohérente est refusée. Les préimages PE/code
existantes restent actives ; des gardes supplémentaires couvrent ce contrat.

Le worker lit une copie cohérente de cet état. Une observation vieille de plus
de 500 ms ne peut pas autoriser de commande. Le loadout est facultatif : il sert
uniquement d'ancrage au scanner de graphes et ne produit plus READY.

`PlayableSession` suit l'identité du personnage, son propriétaire et son
incarnation native. Un changement produit une nouvelle session et réarme AUTO.
Un rafraîchissement d'équipement ne change pas cette identité. La perte de
disponibilité retire les tickets ; un retour du même personnage ne répète pas
un AUTO déjà traité. Un package partiellement interrompu est marqué en échec,
sans répétition aveugle susceptible de doubler armes ou munitions.

AUTO utilise les mêmes packages et primitives que F3/F2. Il passe par les
états attente → armes → munitions à soumettre → attente du résultat munitions
→ traité ou échec. Une mise en file occupée peut être retentée ; un résultat
de traitement appartient obligatoirement à la session qui l'a demandé.

Les deux boîtes de commandes copient leurs payloads sous verrou et portent
requestId/session/epoch/identité. Le consommateur reprend une copie exclusive,
revalide avant l'appel natif et rejette les résultats anciens. Un reset ne rend
pas réutilisable un stockage encore possédé par le consommateur. Un résultat
non acquitté ne peut pas être écrasé par une nouvelle demande manuelle.

Les appels natifs et les E/S disque ne se font pas sous les verrous des boîtes.
Les logs des munitions quittent la section critique du moteur et sont produits
par le worker après le résultat corrélé. Le hook ASM, le rendu, les entrées,
l'updater, les RID, délais/limites de spawn, F1 et le profil personnel sont conservés.

## Contrôles et limites

`verify_a34_scope.py` compare les fonctions primitives et fichiers protégés à
v0.9.7. `NativePlayerTests.cpp` couvre le contrat de lecture, génération,
propriétaire, absence d'équipement, séquence et vie sur mémoire synthétique.
`PlayableRuntimeTests.cpp` couvre session/AUTO, vieillissement, interruption,
payload immuable, réponse tardive, reset pendant consommation et écrasement
d'un acquittement. Les tests updater/menu/DXGI restent requis.

Ces contrôles ne prouvent ni la réception d'une arme ni la bonne couverture de
tous les états du jeu. Le spawn reste la primitive native acceptée : son retour
au repos n'est pas une lecture de l'inventaire. Des blocages d'entrée propres
à une mission peuvent aussi nécessiter une validation supplémentaire ; aucun
verrou du jeu n'est forcé. Les traces doivent décrire le traitement sans annoncer
une livraison non observée.

## Test de la candidate

ZIP : `dxgi.dll` et `QProtocol.asi` seuls à la racine ; INI personnel conservé.
Beta builds → Check now → Update on exit, puis redémarrage par l'utilisateur.
Vérifier histoire : disponible, AUTO reçu, F2/F3 réels. Passer lobby puis Tac Sim :
nouvelle session et mêmes fonctions. Vérifier ensuite mort/rechargement et retour
lobby sans multiplication des packages. Conserver les logs localement.
Statut avant ces essais : **EXPÉRIMENTAL**, jamais VALIDÉ par simple compilation.
Main/v0.9.7 restent la base de retour ; ne pas promouvoir sans validation utilisateur.

## Livraison contrôlée

- Source et tag : `13a843ae576d082dfaae2d5bb1909b6442dc7da2`.
- [CI Windows](https://github.com/DeadneM/Q-Protocol-007-First-Light/actions/runs/41728031516) réussie : 14 contrôles de lecture native, 38 de session/boîtes, 19 updater, menu/bootstrap réel, DXGI/dépendances et portée.
- [ZIP direct](https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/download/a34-playable-player-core-test/Q-Protocol_A34_PlayablePlayerCore_TEST.zip) : 590569 octets, SHA-256 `2a8d9fae0e376f05e52f22e51ffaf3699b16eebdd1b7bc33b2e903d2516bd8ec`.
- Manifeste public et tailles/hash des deux binaires vérifiés après téléchargement.
- Updater réel : A33 trouve A34 en bêta ; le canal stable l'exclut ; A34 ne se repropose pas.
- ASI installé toujours A33, hash inchangé depuis le début de la reprise ; aucune installation par l'agent.
- Main et la dernière release stable v0.9.7 vérifiés inchangés. **Validation en jeu encore requise.**
