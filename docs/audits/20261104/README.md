# Double audit A34 — comparaison et plan

État : **CONTRÔLÉ par lecture et modèles**, défauts fonctionnels toujours ouverts. A34 n'est pas validée. Pas de nouvelle build, installation ou promotion stable dans cette livraison documentaire.

- [Première passe : buts et fonctions](PASS_1.md)
- [Seconde passe : invariants et contre-exemples](PASS_2.md)
- [Manifeste de source](SOURCE_MANIFEST.json), [index de navigation](FUNCTION_INDEX.csv), [résultats des modèles](AUDIT_MODELS.json)

## Comparaison

| Constat | Passe 1 | Passe 2 | Priorité / décision |
|---|---|---|---|
| F01 observation expirée assimilée à perte du joueur | Défaut de cadence | Contre-exemple logique + log compatible | P1, corriger producteur/consommateur et distinguer inconnu de perte confirmée |
| F02 session AUTO sans raison de transition | Contrat incomplet | Règle utilisateur confrontée aux états et fixtures | P1, établir arrivée/réapparition sans répétition menu/pause |
| F03 graphe sélectionné par RID sans contexte suffisant | Risque structurel | Toujours pas de preuve causale pour silencieux | P1 investigation ciblée, aucune substitution spéculative |
| F04 COMPLETE sans possession confirmée | Défaut de sens des statuts | Retour utilisateur invalide toute assimilation à livraison | P1, accusé de réception d'inventaire ou statut honnête « soumis/non confirmé » |
| F05 timeout puis retry après acceptation | Risque de duplication | Contre-exemple de fin native tardive | P1, pas de retry aveugle d'effet irréversible déjà soumis |
| F06 alternance F4 avance trop tôt | Défaut de logique | Conservé | P2, lier au résultat défini, documenter premier mode |
| F07 sauvegarde INI et valeurs de secours | Erreurs silencieuses | Conservé | P2, vérifier écriture/relecture et unifier valeurs sans écraser l'existant |
| F08 cache UI partagé sans synchronisation | Course de données | Chemins des deux threads confirmés | P1, propriétaire unique ou synchronisation cohérente |
| F09 ressources font-upload libérées sur échec GPU | Défaut sur chemin d'erreur | Absence de preuve de fin GPU confirmée | P1, isoler correction avec rollback ; ne pas annoncer résolution du timeout observé |
| F10 attente GPU sous verrou input | Risque de blocage temporaire | Conservé, cause GPU inconnue | P2, contrôler délais et séparation des responsabilités |
| F11 découverte updater abortive | Défaut d'isolation d'erreur | Conservé | P2, diagnostiquer chaque candidat sans masquer les valides |
| F12 transaction updater/relaunch | Courses et reprise incomplète | Conservé | P2, verrou d'installation et reprise vérifiable |
| F13 intention GUI sans session d'origine | Non relevé | Nouveau contre-exemple de transition | P1, capturer provenance dès le clic |
| F14 profil AUTO partiellement capturé | Non relevé | Nouvelle incohérence armes/munitions | P2, snapshot complet de configuration |
| Indices des pools GPU différents | Suspect | Aucun contre-exemple dans modèle borné | Écarté des défauts confirmés ; hypothèses documentées |

P1 : correction nécessaire avant de présenter le comportement concerné comme fiable ; pas une déclaration de crash systématique. P2 : robustesse/cohérence à traiter sans confondre avec le défaut Tac Sim.

## Prochaine correction, objectifs et réussite

1. **Détection/session uniquement** : partir d'A34 avec les acquis stables préservés ; examiner le WIP A35 local sans le considérer validé. Une observation récente ne doit plus dépendre de l'ouverture du menu. Distinguer état inconnu, personnage disponible et événement réel d'arrivée/réapparition. Critère : histoire + lobby + Tac Sim, menu ouvert/fermé et pause ne déclenchent aucun nouvel AUTO ; arrivée et mort/reload en déclenchent exactement un. Tests synthétiques puis confirmation en jeu requise.
2. **Transaction et Q-Pistol** : identifier le spawner du contexte courant et une lecture de possession réellement pertinente. Traçage borné de demande, acceptation, effet observé, sans confondre idle et reçu. Critère : Q-Pistol silencieux effectivement présent via F3, GUI et AUTO en histoire/Tac Sim ; aucune duplication sur retard ou changement de session. Le résultat pour les autres armes en Tac Sim reste à confirmer.
3. **Menu/GPU** : corriger séparément F08/F09, conserver la queue autoritative et les gardes de compatibilité. Critère : chemins d'erreur sans libération GPU en vol, menu utilisable et rendu stable à vérifier sur machine. Ne pas mêler une refonte graphique à la réparation native.
4. **Robustesse complémentaire** : F4, configuration, updater ; critères d'échec explicites et rollback testé. L'updater beta reste désactivé par défaut ; raccourcis et INI personnel sont préservés.

Chaque groupe aura sa branche expérimentale, un objectif mesurable et un retour arrière. ZIP futur : dxgi.dll + QProtocol.asi à la racine ; documentation dans GitHub, INI personnel préservé. Aucun numéro de build ni lien de release nouveau n'est annoncé ici.

## Références et limites

Stable conservée : main `62f75211256c498562a1727a966c27d551dccfb9`, release v0.9.7. Source auditée A34 : `13a843ae576d082dfaae2d5bb1909b6442dc7da2`. Branche documentaire : `audit/asi-double-pass-20261104`. A35 WIP local : `236fcdb38a45a70fd629a310cc62c26d08fc5cfb`, non construit/non publié. Les deux passes partagent le même auditeur ; aucune promesse de perfection ou d'exhaustivité mathématique. Les logs bruts privés restent hors du commit.
