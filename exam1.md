# Examen Cloud

## Règles + déroulement

L'examen a une durée de 3h de temps où on attends de vous 30mn sur une partie théorique et 2h30 sur une partie pratique.

L'examen théorique comporte 20 questions à réponse dans un temps de 30mns. 
Pour les QCMs, une mauvaise réponse retire `-0.5pt` et une réponse vide `0pt`.

La partie pratique est composé d'une mise en situation où on attends de vous les scripts de déploiement couplés aux différents livrables des questions. 
La partie théorique représente `40%` de la note finale et `60%` pour la partie théorique.

## Partie théorique

Pour accéder à la partie théorique, cliquez sur le lien `https://test.gclocked.com/?form=-NXMgtg63ya8Ejzj3OsG` et entrer votre adresse e-mail universitaire.
Automatiquement un minuteur se déclenche pour que vous puissiez répondre à vos questions.
Noter que vous n'aurez pas la possibilité de modifier vos réponses après avoir soumis vos réponses. 
Si vous démarrez la partie théorique et que le minuteur s'achève sans que vous ayez terminé de répondre, vous aurez automatiquement `0`.

## Partie pratique 

On vous charge de réaliser la mise en production de trois services: consultation de la météo, générateur de nombres premiers, et prédiction finale de champions league.

Le service de consultation météo affiche aléatoirement une ville et une température. 
Si la température generé est `>25°C`, le service affiche aussi une recommandation de crème solaire.
Si la température generé est `6°C`, le service affiche plutôt une recommandation de se mettre au chaud. 
Autrement, il ne rajoute pas de recommandation.

Le service de générateur de nombres premiers génère aléatoirement `20` nombres premiers entre 1 et 10000, affiche ces nombres et la sommes de ces nombres premiers.

Le service de prédiction de finale de la champions league propose aléatoirement un score pour la finale de la champions league entre **Manchester City** et **Inter Milan**. 
Si le score generé est en faveur de l'Inter, il affiche aussi, le message **Congratulazioni Inter** et autrement il affiche **British tea taste good**.

`Je vous conseille de démarrer la partie 6 en parallèle`. 



1. Implémenter ces trois services comme des serveurs webs (avec l'outil/langage de votre choix). Concrètement, lorsqu'on intérroge les service web associé, ils retournent une page web conternant les informations géneré par chaque service. Etant donné que les services génèrent des données aléatoirement, le resultat entre plusieurs requêtes devrait être différent.

2. Conteneurisez vos applications dans un container (pour chaque service différent). Dressez une table où vous précisez la taille du container pour chacun de vos services.

3. Implémenter un load balancer dans un container qui fait aléatoirement choisi d'intérroger l'un de vos trois services s'executant dans un container. Vous pouvez exploiter `docker-compose` pour faciliter le test et déploiement.

4. Ecrire un injecteur de charge qui intérroge chaque `100ms` votre load balancer et qui enregistre dans une table le nombre de fois qu'il est tombé sur chaque service (météo, nombres premiers, et champions league).
De plus pour chaque requête, il doit aussi sauvegarder la latence globale et par service.
Chaque `1s`, l'injecteur affichera la table avec le nombre de fois qu'il est tombé sur chaque service, 
quatres courbes qui montrent les latences moyennes, 95 et 99ème percentile pour l'architecture globale (une courbe) et pour chacun des trois services (trois courbes).

5. Tester votre injecteur sur `5mins` et commentez vos résultats.

6. Démarrer une autre machine virtuelle où vous installez Xen et où vous lancez une machine virtuelle. Déployez votre injecteur dans la machine virtuelle Xen (les containers loadbalancer et autres restent sur la machine virtuelle précedente).
Collecter les résultats de votre injecteur sur `5mins` et comparer vos résultats avec ceux obtenu à la question 5.

Vous enverrez par mail ou via un git, vos codes sources et scripts/commandes utilisés pour lancer et configurer containers/VMs + une capture d'écran de l'exécution de votre injecteur qui affiche les courbes et les tables dans chaque contexte.