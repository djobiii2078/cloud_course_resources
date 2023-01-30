# TP7: Initiation à Kubernetes

L'objectif de ce TP est la prise en main des fonctionalités de 
Kubernetes. Pour cela, nous allons réaliser un déploiement local grâce à minikube
et refaire la même procédure avec un cluster de machines physiques.

## En local: MiniKube

**MiniKube** est une machine virtuelle minimaliste qui vous permet de mettre sur pied 
un environement prêt à accueillir kubernetes. 
Il est très utile pour des déploiement rapide et des test hors production. 

### Installation de MiniKube

Pour installer minikube, telecharger le package sur le dépot officiel [https://storage.googleapis.com/minikube/releases/latest/minikube_latest_amd64.deb](https://storage.googleapis.com/minikube/releases/latest/minikube_latest_amd64.deb).
En fonction de votre distribution, vous pouvez vérifier le site officiel: [https://minikube.sigs.k8s.io/docs/start/](https://minikube.sigs.k8s.io/docs/start/).

```
curl -LO https://storage.googleapis.com/minikube/releases/latest/minikube_latest_amd64.deb
sudo dpkg -i minikube_latest_amd64.deb
```

Maintenant, vous pouvez demarrer votre cluster kubernetes local : `minikube start`.

Pour executer des commandes kubernetes, il suffit de préciser `minikube <cmd>`, par exemple pour afficher les pods en cours d'exécution: `minikube kubectl -- get pods -A`.

### Déploiement avec Kubernetes --- avec les images 

Un déploiement consister à déclencher l'exécution d'un ou plusieurs pods.
Dans notre exemple, nous allons déployer un simple serveur web.

```
kubectl create deployment hello-minikube --image=kicbase/echo-server:1.0
kubectl expose deployment hello-minikube --type=NodePort --port=8080
```

Concrètement, on crée un déploiement qui s'appelle `hello-minikube` et on expose son port 8080.

Pour vérifier vos déploiements: `kubectl get deployments` et les services : `kubectl get services`

Essayons de visualiser notre serveur web avec du port forwarding: `kubectl port-forward service/hello-minikube 7080:8080`.
Qu'affiche la page [http://localhost:7080/](http://localhost:7080)

### Déploiement avec Kubernetes --- avec un fichier de configuration

A l'instar de `docker-compose`, vous pouvez aussi automatiser votre déploiement kubernetes à l'aide d'un fichier .yml.
Créer un point accès (un service ingress) qui nous redirigera sur deux containers en fonction de l'url.

```
kind: Pod
apiVersion: v1
metadata:
  name: foo-app
  labels:
    app: foo
spec:
  containers:
  - name: foo-app
    image: kicbase/echo-server:1.0
---
kind: Service
apiVersion: v1
metadata:
  name: foo-service
spec:
  selector:
    app: foo
  ports:
  # Default port used by the image
  - port: 8080
---
kind: Pod
apiVersion: v1
metadata:
  name: bar-app
  labels:
    app: bar
spec:
  containers:
  - name: bar-app
    image: kicbase/echo-server:1.0
---
kind: Service
apiVersion: v1
metadata:
  name: bar-service
spec:
  selector:
    app: bar
  ports:
  # Default port used by the image
  - port: 8080
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: example-ingress
spec:
  rules:
  - http:
      paths:
      - pathType: Prefix
        path: "/foo"
        backend:
          service:
            name: foo-service
            port:
              number: 8080
      - pathType: Prefix
        path: "/bar"
        backend:
          service:
            name: bar-service
            port:
              number: 8080
---
```

Examinez le contenu de ce fichier et expliquer le fonction du service de type `Ingress`.

Lancer le déploiement de cette architecture: `kubectl  apply -f <nom_de_votre_fichier.yaml>` 

Vérifiez l'état de vos pods. S'ils sont tous en cours d'exécution: alors obtenez l'adress de votre service `ingress`: `kubectl get ingress` 

Tester les deux chemin avec curl: 

```
curl <ip_ingress>/foo 
curl <ip_ingress>/bar 
```

### Configuration avancée: Passage à l'échelle et mise à jour

Kubernetes donne la possibilité de créer plusieurs replicas à la volée et réalisez des mises à jour de vos services. 
Pour la suite du TP, nous allons continuer sur l'interface de prise en main offerte en ligne par Kubernetes.
Vous avez le choix de continuer entièrement en ligne ou repercuter les commandes qui vous sont exposés sur votre poste de travail.

L'objectif est de terminer le tutoriel *Apprendre les bases de Kubernetes*: [https://kubernetes.io/fr/docs/tutorials/kubernetes-basics/](https://kubernetes.io/fr/docs/tutorials/kubernetes-basics/).

## En mode cluster

Jusqu'à présent on travaillait avec un seul noeud : `kubectl get nodes`.
A présent, nous allons essayer de créer un cluster de machines administré par `kubernetes`.

Pour cela, vous allez vous mettre par groupe de trois ou quatre.
Choisissez celui qui sera le noeud master et qui joueront le role des workers.

### Configuration du master et workers 

**Installer les dépendences sur tout les noeuds (master + worker)**

```
sudo apt-get update
sudo apt-get install -y apt-transport-https ca-certificates curl
sudo curl -fsSLo /etc/apt/keyrings/kubernetes-archive-keyring.gpg https://packages.cloud.google.com/apt/doc/apt-key.gpg
echo "deb [signed-by=/etc/apt/keyrings/kubernetes-archive-keyring.gpg] https://apt.kubernetes.io/ kubernetes-xenial main" | sudo tee /etc/apt/sources.list.d/kubernetes.list
sudo apt-get update
sudo apt-get install -y kubelet kubeadm kubectl
sudo apt-mark hold kubelet kubeadm kubectl
sudo swapoff -a
```

**Sur le master**

```
kubeadm init
mkdir -p $HOME/.kube
sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
sudo chown $(id -u):$(id -g) $HOME/.kube/config
```

Cela va initialiser le noeud master et vous donner les commandes nécessaires pour 
pour rajouter des noeuds à votre cluster. 

**Sur les workers**

Utiliser la commande fourni par le master --- elle devrait être similaire à ceci:

```
kubeadm join --token 702ff6.bc7aacff7aacab17 174.138.15.158:6443 --discovery-token-ca-cert-hash sha256:68bc22d2c631800fd358a6d7e3998e598deb2980ee613b3c2f1da8978960c8ab
```

**Sur le master** 

Vérifier que vos workers sont bien présent avec `kubectl get nodes` 

### Déploiement d'une architecture 3-tier 

**Sur le master**

Déployons l'add-on `weave` qui facilite la création des politiques réseau et est utilisé par notre application cible

```
kubectl apply -f https://github.com/weaveworks/weave/releases/download/v2.8.1/weave-daemonset-k8s.yaml
```

*Je vous invite à regarder le contenu du fichier `weave.yaml`.*

Vérifiez que vos pods sont tous en cours d'exécution avec de continuer: `kubectl get pods --all-namespaces`

Cloner l'application cible et déployons là: 

```
git clone https://github.com/microservices-demo/microservices-demo.git
cd microservices-demo/deploy/kubernetes
kubectl create namespace sock-shop
kubectl apply -f complete-demo.yaml
kubectl get pods --namespace sock-shop
```

Exploitez la commande `kubectl describe -n <namespace> <nom_pod>` pour déterminer sur quels noeuds chaque pod est deployé.

Trouvez le point d'accès de votre architecture et les zones liées au monitoring.

*Hint: 30001/, 31300/, 31090/ et les ReadMe :)*

### Déploiement d'une architecture personalisée

Créer votre propre fichier de déploiement pour deployer votre load balancer à trois niveaux sur une Kubernetes sur votre cluster.
