# Audit ASI — première passe : fonctions et contrats

Date : 4 novembre 2026. Source figée : `13a843ae576d082dfaae2d5bb1909b6442dc7da2` (A34). La stable v0.9.7 reste la référence acceptée ; A34 est expérimentale et présente des échecs signalés. Aucun résultat CI ne transforme A34 en version validée en jeu.

## Périmètre et méthode

Lecture des chemins actifs : initialisation, contrôles PE/préimages, hook ASM, lecture native, session, transactions armes/munitions, commandes clavier/menu, configuration, rendu/input, proxy et updater. Les tests et le workflow sont examinés comme moyens de contrôle. Les anciens overlays non compilés ne sont pas supprimés. Les bibliothèques tierces ne font pas l'objet d'un audit intégral ; leurs intégrations modifiées sont incluses. Le manifeste fixe les fichiers examinés. L'index de fonctions est une aide de navigation issue d'une extraction syntaxique, pas une preuve de couverture sémantique exhaustive.

## But des fonctions et comportement effectif

| Ensemble | But et contrat attendu | Conclusion de lecture |
|---|---|---|
| Bootstrap, guards, hook ASM | Refuser un EXE/préimage incompatible ; conserver ABI et exécuter les appels du jeu sur le hook | Contrôles conservés. Pas de preuve de panne ABI actuelle. Pin du module nécessaire durant toute la vie des hooks ; l'échec du pin ne doit pas laisser ce contrat implicite. |
| NativePlayer, guards natifs | Résoudre personnage local, génération, propriétaire et état natif cohérents | Chaîne typée et bornée, ambiguïtés refusées. Vivant/hors séquence n'est pas une preuve universelle de contrôle effectif (véhicule, contrôle forcé, tous types de transition). |
| ObserveGameplayPlayer, ResolvePlayer | Publier une observation assez récente pour autoriser les commandes | F01 : publication liée au hook ; le worker réutilise le cache expirant après 500 ms. Absence de callback peut devenir faux NOT READY sans perte native du personnage. |
| PlayableSession, ResetWeaponRuntime | Définir une session jouable et appliquer AUTO une fois par arrivée/réapparition | F02 : identité et disponibilité ne distinguent pas mort/rechargement, pause, lecture impossible et interruption. Même identité retrouvée peut garder AUTO traité ; expiration peut interrompre AUTO. |
| GameplayMailbox, Tick | Transporter une commande immuable ; refuser résultat périmé ou mauvais personnage | Tickets, session/epoch, ownership en vol et annulation constituent une protection utile à préserver. Cela ne prouve pas la livraison de l'objet. |
| BuildGraphIndex, ValidateGraph | Trouver une entrée et un spawner correspondant à l'objet et au contexte courant | F03 : sélection du premier spawner par RID ; validation type/RID sans preuve suffisante de propriétaire, génération et monde courant. Candidat à la panne Tac Sim, causalité non démontrée. |
| BeginWeaponRequest, ProcessWeaponQueue, completion/retry | Déclencher une fois, suivre, confirmer et traiter les erreurs | F04 : accepted + idle est présenté comme COMPLETE sans inventaire confirmé. F05 : timeout après acceptation peut relancer sans preuve d'annulation ou d'idempotence. Le timeout avant déclenchement dépend aussi de la cadence du hook. |
| Loadout, AUTO, QueueAmmoProfile | Appliquer un profil cohérent, puis munitions, une fois par événement autorisé | Publication/traitement d'une requête ne prouve ni possession ni réserve. Les phases Failed/Processed ne couvrent pas une récupération partielle démontrée sûre. |
| F3, F4, commandes catalogue | Réutiliser le même moteur de demande pour les actions explicites | F06 : F4 alterne son choix dès l'acceptation en file, même si l'exécution échoue ensuite. Les raccourcis sont volontairement filtrés quand le menu prend les entrées ; AUTO n'est pas conditionné à la visibilité du menu. |
| Chargement/sauvegarde INI | Préserver le fichier personnel et appliquer explicitement les préférences | Création initiale sans écrasement utile. F07 : résultats des écritures ignorés, sauvegarde partielle possible et message de succès trop affirmatif. Valeurs de secours UI/core différentes sur INI partiel. |
| Overlay rendu/input et backend DX12 modifié | Utiliser la queue DIRECT du bon device ; protéger ressources et rendre la main sur échec | F08 : g_uiLoaded écrit par le worker sans la synchronisation du rendu. F09 : nettoyage du font upload après échec de signal/attente sans preuve de fin GPU. F10 : verrou commun rendu/WndProc peut retarder les entrées pendant une attente GPU. |
| Capture queue/swapchain, resize, shutdown | Conserver les références nécessaires et ne pas libérer en vol | Drain frontend prudent à préserver. Association par pointeur de swapchain non possédé : réutilisation d'adresse à considérer ; pas de cause GPU établie. Différence d'index frontend/backend : suspicion à falsifier en passe 2. |
| Updates.cpp, moteur C# | Vérifier canal, version, EXE, hash et ZIP ; remplacer après sortie, préserver INI et rollback | F11 : un ancien manifeste invalide/inaccessible peut arrêter toute la découverte. F12 : pas de verrou de transaction par installation ; course avec relancement et remplacement de deux fichiers non atomique face à un arrêt brutal. Retour d'échec post-sortie peu visible. |
| Proxy DXGI | Relayer exports/ABI au DXGI système et charger ASI | Chemin System32 et initialisation unique conservés. Fallback générique d'export manquant ne représente pas toutes les signatures possibles ; risque conditionnel, pas panne actuelle démontrée. |

## Preuves et limites

Le log A34 consulté contient des bascules PLAYABLE/NOT PLAYABLE pour la même identité/epoch, des demandes AUTO/F3/F4 traitées nativement, et un timeout Render.FrameFence. Les journaux bruts restent locaux. Le retour utilisateur confirme l'absence du Q-Pistol silencieux même avec F3 et menu fermé ; le menu seul ne peut donc expliquer ce symptôme.

La CI A34 avait réussi : elle contrôle compilation, fixtures et paquet, pas livraison native des armes ni comportement après mort. A35 `236fcdb` reste un travail local séparé, non construit, non publié ; il ne résout pas tous les constats ci-dessus.
