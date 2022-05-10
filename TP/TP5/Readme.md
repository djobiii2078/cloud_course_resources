# TP5

L'objectif de ce TP est d'explorer le volet automatisation du déploiement des containers. 
Pour cela, nous allons utiliser `docker-compose` 

## docker-compose

`docker-compose` permet de configurer et automatiser le déploiement des containers en spécificiant les charactéristiques à travers un fichier `.yaml`. 
Il permet aussi de déclencher le démarrer de plusieurs containers d'une même image de façon simplifié. 

## Configuration d'un loadbalancer avec docker-compose

Créeons le fichier de configuration `docker-compose.yaml`qui définira notre loadbalancer (composé d'apache, tomcat, nginx)

```
version: '1'
services:
  tomcat:
   image: tomcat:v1
   build:
    context: ./tomcat
    dockerfile: Dockerfile
   hostname: tc1
   ports:
     - 8081:8080
   networks:
     - mynet 
  apache2:
    image: apache2:v1
    build:
        context: ./apache2
        dockerfile: Dockerfile 
    hostname: apache2
    ports:
     - 8082:80
    networks:
     - mynet 
  lb:
    image: lb:v1
    build:
        context: ./lb
        dockerfile: Dockerfile 
    depends_on:
      - tomcat
      - apache2
    ports:
      - 8080:80
    networks:
      - mynet
networks:
  mynet:
    driver: bridge 
```


`Pouvez-vous expliquer le role des options *depends_on* et *build* ?`

Recréer les dossiers apache2, lb, et tomcat pour inserer les Dockerfile de chaque serveur web.

Construisons tout les images à la fois : 

`docker-compose build` 

Démarrer votre cluster de container : 

`docker-compose up -d` ==> Quel est l'ordre de démarrage de vos containers ?

Rajoutons plusieurs instances du container `lb` : 

`docker-compose up -d --scale lb=4` ==> Quel est l'utilité d'une telle commande ?

### Rajout d'un injecteur de charge 

![Injecteur de charges](./injector.png)


Ecrire le code d'un composant qui fera des requêtes à notre loadbalancer et renverras un rapport sur la latence des requêtes. 
Concrètement, chaque 5s, le composant enverra 1000 requêtes et produira en sortie la moyenne, 95ème et 99ème percentile des 1000 requêtes. 

`Utiliser le language de votre choix : mais voici un bout de code`

```
const express=require('https')

const options = {
  hostname: 'loadbalancer_address',
  port: 80,
  path: '/',
  method: 'GET',
  headers: {
    'Content-Type': 'application/json'
  },
};

function issueReq (){
  
for (...)
{
  /*set timer*/
  ...
  const req = https.request(options,res=>{
    ....

    res.on('data', d =>{
      /*end timer and aggregate somewhere */
    })
  })
}

/*Compute mean, 95th and 99th percentiles*/

}

setInterval(issueReq,5000)

```



- Créer un dossier et à l'intérieur définisser le dockerfile pour votre injecteur. 

- Rajouter l'injecteur dans le fichier de configuration du cluster (l'injecteur ne doit s'initialiser avant le load balancer)

- Reconstruire les images de tout vos containers : `docker-compose build` ==> Est-ce qu'il reconstruit tout les images ?

- Démarrer votre cluster et initialiser 100 instances de l'injecteur. 

- Modifier votre injecteur pour qu'il affiche sous forme graphique (en console) le recapitulatif de chaque jet de requêtes.

- Reconstruire les images de tout vos containers et tester votre cluster 


## Apport de l'automatisation dans la portabilité 

Refaites l'exercice de partager votre cluster et tester le cluster de deux de vos voisins. 

Avec `docker-compose`, l'expérience est-elle différente (plus simple, plus compliqué) ou bof ... ?






