# Contrat d'identité natif : enquête après audit ASI

Date : 2026-11-03. Base acceptée v0.9.7 ; A32 non retenue comme correction.

## Écart établi dans A32

EXE étudié : SHA-256
`6e63df4d2891ea085a71ef9a6f4d24b9db52144879945e8cc1a2e8106fabd3f1`.

Les deux appels directs du moteur au helper `0x015805D0` n'utilisent pas le
champ transmis par A32 :

| Chemin | Chaîne / champ | Destination |
|---|---|---|
| Moteur, fonction `0x01581E40` | global `0x06452BF8` → `+0x10` → uint32 `+0x34` | ECX à l'appel `0x01581EC5` |
| Moteur, fonction `0x01580890` | même global → `+0x10` ; lecture `+0x30` et `+0x34`, ce dernier conservé dans EDI | ECX à l'appel `0x01581668` |
| A32 | même global → `+0x10` → uint32 `+0x30` | ECX du helper |

Le helper appelle en dernier recours `0x016BAEE0`. Celui-ci parcourt la liste
de références natives `[EXE+0x0671FFC0, EXE+0x0671FFC8)` et compare l'argument
à `objet+0x1AC`. Son résultat fournit le loadout typé à `objet+0x188`.

À distinguer : `0x016BAFD0` parcourt aussi cette liste mais retourne
`objet+0x1A8`, utilisé notamment par la publication native de munitions.
Il ne faut donc pas remplacer globalement le champ historique `+0x30` par
`+0x34` dans tout le mod. Les deux identités ont des consommateurs distincts.

Le resolver historique `0x0171D2E0` ignore le premier argument donné par l'ASI.
Il consulte le contexte global `0x06454120` / interface `0x06454130` et ses
références de génération ; passer une autre valeur que zéro à son premier
argument ne sélectionnerait pas un autre joueur.

## Comparaison réelle, en lecture seule

Outil : `tools/read_mission_context.py` (Python Windows 64 bits).

- Exige exactement un processus de jeu et le hash EXE attendu.
- Ouvre le processus avec VM_READ et QUERY_LIMITED_INFORMATION seulement.
- Aucun appel natif dans le jeu, injection, suspension, hook ou écriture mémoire.
- Lit les deux IDs, la liste native bornée, les types RTTI et références loadout.
- Relève les masques TLS des threads lisibles et les générations associées.
- Trois échantillons, relectures de stabilité ; aucune atomicité globale promise.
- Écrit uniquement un JSON privé hors du dépôt ; ne pas publier les captures brutes.

Première observation lobby : les deux champs sont différents ; `node+0x30`
correspond à `ZPlayerSpawnerEntity+0x1A8` et `node+0x34` à `+0x1AC`.
L'entrée possède une référence typée `ZKntPlayerLoadoutEntity` à `+0x188` ;
les références et racines sont stables sur les trois échantillons.
Les masques TLS lisibles observés sont identiques dans cette capture : cela
ne suffit pas à établir le contrat de tous les threads dans tous les modes.

La première tentative de lecture dans l'environnement isolé ne voyait pas le
processus externe. La lecture autorisée hors de cet environnement le voit et
fonctionne : une absence dans ce premier inventaire ne prouve pas que le jeu
était fermé. Aucune écriture dans le jeu n'a été nécessaire.

## Résultat de la comparaison Tac Sim

Mission contrôlable confirmée par l'utilisateur. Les trois échantillons montrent
les deux IDs stables et distincts, une entrée native `ZPlayerSpawnerEntity`
avec correspondance `node+0x34 == spawner+0x1AC`, et une référence
`ZKntPlayerLoadoutEntity` à `+0x188` avec génération concordante.
Les interfaces des deux contextes globaux ont changé depuis le lobby ; leurs
références/interfaces imbriquées utilisées par les premiers chemins sont vides.
Le loadout de l'entrée native conserve la même adresse entre ces deux captures :
c'est une référence exposée par le registre actuel, pas la preuve d'un inventaire
appartenant à l'acteur contrôlé ni d'une nouvelle génération de mission.

Conclusion : A32 passe la mauvaise identité à son chemin de dernier recours.
A33 corrige cet argument et préserve l'ID historique retourné aux munitions.
Les fixtures utilisent désormais deux IDs différents et un mock refusant le
mauvais ID ; elles couvrent aussi changement du second ID et sentinel invalide.

## Prochaine preuve requise

Tester A33 dans le jeu : READY, F3 et réception des armes, sans régression lobby.
Le constat de registre ne prouve pas cette réception. La logique de sessions et
le déclenchement AUTO restent à traiter après validation de la primitive manuelle.
