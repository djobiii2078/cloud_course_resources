# Prologue

Le but de ce prologue est d'implémenter l'application qui vous servira de base durant les travaux pratiques.
L'objectif sera de déployer votre application dans une VM, microVM, container, et orchestrer cela afin d'être rodé aux techniques sous-jacentes.

## Application 

Dans le langage de votre choix, implémenter une application de visualisation de la  

- Une page d'accueil simpliste permettant à un utilisateur de s'inscrire et de se connecter 
- Un mécanisme où l'on peut faire une recherche d'une liste d'hôtels par ville (vous pouvez avoir des villes fictives)
- Un mécanisme de réservation qui permet à un utilisateur de sélectionner un hôtel, choisir les dates qui correspondent, et réserver l'hôtel. La réservation est confirmée par l'envoi d'un mail de confirmation.
- Un mécanisme qui permet de visualiser sur la carte du monde (idéalement interactive) les endroits où un hôtel a déjà été réservé ou une réservation à venir.

Toutes les informations des utilisateurs doivent être stockées dans une base de données (SQL ou NoSQL, à votre choix).
Vous pouvez implémenter votre solution en ligne de commande si vous voulez mais la carte devra quand même être générée sous format image (qui pourra être visualisée par l'utilisateur).

Conservez le code de votre application qui servira lors des prochains TPs. 
