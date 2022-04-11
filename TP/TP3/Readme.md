# TP3 : Manipulation des containers 

Le but du TP est de pouvoir manipuler les containers et évaluer la dégradation de performance.
Pour ce TP, nous allons utiliser Docker.

## Installation de Docker 

Pour installer le Docker runtime, nous allons utiliser les mêmes instructions que le [dépot officiel](https://docs.docker.com/engine/install/ubuntu/).

```
sudo apt-get remove docker npm nodejs docker-engine docker.io containerd runc

sudo apt-get install ca-certificates curl gnupg lsb-release

curl –fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor –o /usr/share/keyrings/docker-archive-keyring.gpg 

echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu  \
  $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

sudo apt-get update && sudo apt-get install docker-ce docker-ce-cli containerd.io

```

Pour tester votre installation, executer `docker run hello-world` 

## Démarrage d'un container basique.

Pour démarrer, un container a besoin d'un fichier de spécification `Dockerfile` qui donne les directives sur les couches du container.
Créeons un projet `myapp`: 

```
mkdir myapp && npm init . && npm install --save express
```

Créeons un Dockerfile avec le contenu suivant : 

```
FROM ubuntu
RUN apt-get update && apt install nodejs npm
WORKDIR .
COPY  package.json /
RUN npm install .
CMD [ "node", "app.js" ]

```

Le fichier `app.js` crée un serveur http sur le port 3000 : 

```
const express = require('express')
const app = express()
const port = 3000

app.get('/', (req, res) => {
  res.send('Hello World!')
})

app.listen(port, () => {
  console.log(`Example app listening at http://localhost:${port}`)
})

```

Construisons le container : 

```
docker build –t myappcontainer .
docker images 
docker run –d 3000:3000 myappcontainer
```

Tester le container : 

```
curl -X GET 127.0.0.1:300
```

## Evaluer la dégradation 

Mesurer la latence des requêtes `http` de votre serveur containerisé et en natif

## Votre container, vos métriques.






