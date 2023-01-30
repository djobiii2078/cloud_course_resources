# TP1: Rappel + Mise en bouche pour Kubernetes

Ce TP a pour objectif de rappeler les concepts de base des containers et faire une mise en bouche pour Kubernetes.
Concrètement, vous allez être amener à implementer certaines fonctionnalités de Kubernetes pour saisir la complexité de le l'outil.

## Rappel: Déploiement d'un load balancer à trois instances.

L'objectif est de recréer un load balancer similaire à celui implementée les années antérieures.

1. Ecrire la spécification pour les containers pour trois serveurs web de votre choix e.g., Tomcat, Apache, et Express.
2. Ecrire la spécification pour le composant **LB**, qui se chargera de recevoir les requêtes et de choisir **aléatoirement** le serveur web cible.
3. Démarrer les containers et assurer vous qu'ils communiquent dans un réseau propre qui aura pour test **orchestrateurtest**.
4. Tester votre architecture et vérifier que le load balancer fonctionne correctement.

## Gestion du passage à l'échelle 

L'objectif est d'essayer d'implémenter la fonctionnalité de *passage à l'échelle* d'un orchestrateur.
1. Pour cela, vous allez écrire un script (dans le language de votre choix) sous la forme: 

`customOrc --scale <nom_container> <latence_seuil> <nbs>`

où:

- *customOrc* est le nom de votre executable
- *nom_container* est le nom du container à surveiller 
- *latence_seuil* est la latence de réponse du container en millisecondes au dessus de laquelle il faut déclencher le mécanisme de passage à l'échelle.
- *nbs* est le nombre d'instances du container à démarrer en cas de passage à l'échelle.

Concrètement, votre script doit déterminer l'adresse ip du container ciblé (ou pas) , puis chaque *100ms*, effectuer des heartbeats et déterminer l'état du container : **bon** si la latence
  est inférieure à **<latence_seuil>** et **faible** si au dessus.
  Lorsque l'état devient faible, la procédure de passage à l'échelle est declenchée.
  
  **NB: Chaque fois qu'un heartbeat est effectuée, votre script affiche l'état et la latence sous le format `<nom_container>  <latence>  <état>`.**
  
  2. Tester votre solution en écrivant un mini client qui envoie 1000 requêtes chaque 10ms au load balancer afin de surcharger votre infrastructure.
  D'après vous, votre script est-il efficace ? Commentez votre résultat avec des métriques à l'appui.
  
  3. Containeriser votre script et fournisser le à deux voisins pour qu'il le teste sur son infrastructure.
  D'après vous, les deux scripts que vous avez testé sont-ils plus efficace que le votre ? Commentez votre résultat avec des métriques à l'appui. 
  
  ## Mise à jour en production
  
  L'objectif est d'essayer d'implémenter la fonctionnalité de *mise à jour* d'un orchestrateur.
  Pour cela, étendez votre script *customOrc* pour ajouter un nouveau paramètre *--update* qui prend en paramètre ceci:
  
  `customOrc --update <nom_container> <nouvelle_image>`
  
  où:
  
  - *nom_container*: le nom du container en cours d'exécution. S'il n'est pas en cours d'exécution rien ne se produit.
  - *nouvelle_image*: le nom de la nouvelle image à utiliser pour votre container.

Trouver une facon intélligente de tester votre mécanisme de mise à jour et calculer le temps nécessaire pour mettre à jour un container en cours d'exécution.
Réeffectuer le même mécanisme de partage avec vos voisins et commentez la performance de leur mécanisme.

## Gestion d'un cluster 

L'objectif est d'implémenter la surveillance d'un cluster de machine pour déterminer si elles sont aptes à accueillir des containers.
Pour cela, étendez votre script *customOrc* pour ajouter un nouveau paramètre *--monitor* qui prend en paramètre ceci:

`customOrc --monitor <ip1,ip2,ip3...,ipn>`

où:

- *ip1,ip2,...,ipn*: correspond à la liste des adresses IPs des machines de votre cluster (separé par des virgules)

Concrètement, grâce à `ssh`, vous envoyer des commandes à chaque machine identifié par les addresses ips passés en paramètre, qui vous permettent d'obtenir le taux d'utilisation *CPU* et *memoire* sur ces serveurs.
Puis votre script doit les afficher à l'écran chaque *5* secondes sous le format suivant:

```
ips CPU(%) MEM(%)
ip1 75.2  32.1
ip2 23.1  8.3
 ...
ipn 22.7  3.4
```

En géneral, quelles sont vos observations sur le travail d'un orchestrateur ? Qu'est-ce qui est le plus fastidieux ? Qu'elles sont les conseils que vous donneriez à quelqu'un voulant developer son orchestrateur ?

