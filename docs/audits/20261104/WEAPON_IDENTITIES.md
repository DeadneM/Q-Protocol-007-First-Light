# Contrôle des 31 RIDs et reprise de l'historique

4 novembre 2026. Demande : vérifier les identifiants sans faire tester toutes les armes au joueur. Aucune modification des RIDs, INI, assets ou installation dans ce contrôle.

## Résultats vérifiés

- 31/31 RIDs du catalogue sont présents dans les index des RPKG installés, de type TEMP, dans chunk0.rpkg. Les deux petits packages additionnels inspectés ne remplacent ni ne suppriment ces 31 ressources ; aucun ordre de chargement général n'est inféré.
- 31/31 ont un chemin connu dans [Bond-Hashes, révision figée](https://github.com/glacier-modding/Bond-Hashes/blob/94767af01ce524975acbb254e4cd98e28a04d061/paths/TEMP.json). Le blob Git téléchargé est vérifié ; les 31 hashes sont recalculés à partir des chemins (MD5, préfixe First Light 01) et concordent. Ce ne sont pas de nouveaux numéros supposés.
- Silencieux `019973508A5E327B` : `firearm_instance_qpistol_silenced`. Normal `01BE2C45AA55200D` : `firearm_instance_qpistol`. Les deux sont bien des templates d'instances, pas des définitions de propriétés ni des modèles visuels.
- Deux précisions de nom : `AssassinRifle` désigne `firearm_instance_exotic_harpoon_a` ; `ShotgunCompactOneHanded` désigne `firearm_instance_shotgun_sawnoff_a`. Le premier libellé est trompeur. Ne pas renommer une clé INI sans migration/préservation des références personnelles.
- Présence et identité de ressource ne prouvent pas qu'un objet est reçu, jouable, du bon rôle ou autorisé dans chaque mode.

[Détail des 31 entrées et index contrôlés](WEAPON_RESOURCE_IDENTITIES.json). Contrôle reproductible avec `tools/audit_weapon_resources.py` (lecture seule, table Bond-Hashes externe à fournir). Le format First Light réellement inspecté a des métadonnées de 20 octets ; une lecture aveugle au format Hitman 24 octets échoue dès les premières entrées. Le parcours complet des tables et les bornes de chaque ressource sont contrôlés.

## Comparaison avant/après adaptation et dernière stable

`06245ba` (A26/A27 avant remap), `8722000` (v0.9.7 acceptée), puis A34/A35 : aucune différence dans config/QProtocol.ini. La dernière remappage de catalogue explicite est A13 (`2f527c3`), détaillé dans A13_WEAPON_RID_REMAP.md. L'adaptation A28 (`580e57c`) change les RVAs natifs et ajoute les vérifications PE/préimages ; elle ne remappe pas les armes. A29 ajoute updater/menu. A34 remplace surtout disponibilité/session, transport et suivi des commandes. A35 découple observation et dispatch. Les contrôles de périmètre de la build A35 confirment que graphe, RIDs, UI, paramètres publics et primitives acceptées restent préservés.

L'absence de diff de catalogue exclut une modification de ces valeurs dans cette séquence ; elle n'exclut pas une évolution des règles du jeu, de ses données ou du contexte natif. L'ancien EXE d'octobre manque : aucun diff binaire complet avec lui n'est revendiqué.

## Comparaison réelle Tac Sim sur A34

Retour utilisateur : F3 livre les deux autres armes du profil mais pas le Q-Pistol ; F4 seul livre le normal et pas le silencieux. D'autres armes peuvent aussi manquer ; leur liste n'est pas exigée avant d'étudier le chemin commun.

Lecture ciblée, sans injection ni écriture : graphes silencieux, normal, Taser, ShotgunCompact typés et cohérents ; références source valides. La liste native d'objets rattachés au joueur est stable sur trois relevés. Les références conservées dans les résultats des graphes du Taser, ShotgunCompact et normal partagent leur clé/génération avec un objet de cette liste ; celle du silencieux est valide mais absente de la liste. Sa table de composants expose bien ZFirearmCharacterEntity et l'interface IItem attendue par le ramassage natif. Le champ d'identifiant lu par ce ramassage correspond aux armes témoins déjà présentes.

Limite essentielle : ces résultats sont lus dans la capacité conservée du vecteur après sa remise à zéro logique. Leur génération vivante est vérifiée, mais cela ne constitue pas encore un accusé de réception attribué à une requête particulière du mod. Un futur correctif doit capturer son propre résultat pendant la requête ; il ne doit pas adopter aveuglément une ancienne entrée de capacité.

## Prochaine intervention justifiée

Les RIDs contrôlés ne justifient aucun remplacement arbitraire. Vérifier et réparer le chemin commun création → rattachement → confirmation. Le contrat de ramassage natif identifié dans ZCLTriggerPlayerItemPickup utilise une entrée de trois DWORDs, le verrou déjà connu et les primitives d'insertion/notification déjà remappées, avec un autre événement/contexte. Cela reste une identification statique/read-only : aucune publication de cet événement n'a été testée. Ne pas l'intégrer comme solution validée tant que provenance du résultat, thread, cible, idempotence et réception ne sont pas contrôlés.

Pas de patch propre au silencieux, pas de désactivation des gardes, pas de prétendue réparation fondée sur « spawner idle ». A35 compilée reste un candidat de session séparé, pas une réparation de livraison des armes.
