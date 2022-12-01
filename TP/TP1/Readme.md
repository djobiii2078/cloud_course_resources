# TP1

## Goal 

The goal of this labwork is to play with VMs and grasp the difficulties around them.
We will use Xen and KVM to create 2 VMs. These VMs will be considered as your physical servers. Then,
in these physical servers, we will install Ubuntu, install Xen (with its hypervisor) and then create a
VM. Then we will collect metrics such as boot time, performance, and latency, and experiment migration.

## Xen install 

There are basically two ways of installing Xen on a Linux kernel operating system. 
The first is to manually build it from sources and the second is to install pre-built binaries. 
We will try both. Useful resources to look can be find here : https://wiki.xenproject.org/wiki/Xen_Project_Beginners_Guide

Before launching, get the list of supported operating systems and kernels in the grub : `ls /boot`
### Manual install 

1. Download Xen 4.15 from the official website. You can just run `wget https://downloads.xenproject.org/release/xen/4.15.0/xen-4.15.0.tar.gz`.
2. Unzip the archive. You can run `tar xvzf xen-4.15.0.tar.gz`
3. Install the dependencies required by Xen 
`sudo apt update
sudo apt install build-essential bcc bin86 gawk bridge-utils
iproute libcurl3 libcurl4-openssl-dev bzip2 module-init-tools
transfig tgif texinfo texlive-latex-base texlive-latex-recommended
texlive-fonts-extra texlive-fonts-recommended pciutils-dev mercurial
make gcc libc6-dev zlib1g-dev python python-dev python-twisted
libncurses5-dev patch libvncserver-dev libsdl-dev libjpeg62-dev
iasl libbz2-dev e2fslibs-dev git-core uuid-dev ocaml ocaml-findlib
libx11-dev bison flex xz-utils libyajl-dev gettext libpixman-1-dev
libaio-dev markdown pandoc libc6-dev-i386`

**Some libraries may not be supported anymore. Adjust by removing the faulty libraries from the list.**

4. Then, install remaining dependencies. 

`sudo apt build-dep xen`


5. Now enter the Xen folder and run `./configure`. This will configure headers and installation scripts based on your system.

6. Build Xen by running : 

`sudo make -j$(nproc)` *What is the meaning of $(nproc)*
`sudo make install` 
`sudo update-grub` *What is the goal of this command*

7. Check the `/boot` folder. 

8. Reboot the machine and boot with the `Xen hypervisor` 


## Too hard ? Check the prebuilt version 

1. Install Xen from the prebuilt store `sudo apt-get install xen-hypervisor-amd64`
2. Open the file `/etc/default/grub.d` et corriger `XEN_OVERRIDE_GRUB_DEFAULT` avec la valeur `1`
3. Mettez à jour la liste des systèmes et redemarrer `sudo update-grub && reboot`.

## Démarrer le service Xen 

1. Lancer le service Xen avec la commande `sudo /etc/init.d/xencommons start` 
2. Vérifier que le service est correctement lancé : `sudo xl list`. Que fait cette commande ? 
3. Quelle quantité de ressources est alloué au `dom0` ? 

## Démarrer une VM paravirtualisé

1. Téléchargeons l'image d'un système d'exploitation (Ubuntu) : `wget http://cloud-images.ubuntu.com/releases/focal/release/ubuntu-20.04-server-cloudimg-amd64.img -P /home/ubuntu/images/ -O vm.qcow2`
2. Créons le fichier de configuration pour notre futur VM. Par la suite, je suppose que le fichier s'appelle `vm1.cfg` : 
```
 bootloader = 'pygrub'
 vcpus = 2
 memory = 1024
 root = '/dev/xvda1 ro'
 disk = [
  '/home/ubuntu/images/vm.qcow2,qcow2,hda,rw'
 ]
 name = 'myvm'
 type = 'pvh'
 vif = [ 'bridge=br0' ] 
 ```

Changer le chemin de l'image pour correspondre à vos répertoires. 

3. Modifions le mot de passe de l'image pour pouvoir y accéder : 

 ```
 modprobe nbd max_part=8
 qemu-nbd --connect=/dev/nbd0 /home/vms/images/vm.qcow2 
 fdisk /dev/nbd0 -l
 mount /dev/nbd0p1 /mnt/
 chroot /mnt/
 passwd
 ```
 
 
Modifier le mot de passe pour mettre celui de votre choix et enregistrer cela 
```
 umount /mnt/
 qemu-nbd --disconnect /dev/nbd0
 rmmod nbd
 ```
 

4. Que pouvez vous dire du fichier de configuration ? Que nous-manque t'il ? 

5. Démarrons la VM : `sudo xl create -c /chemin/vers/vm1.cfg` 

6. Que vous donne la commande : `sudo xl list`

7. Pouvez-vous rajouter des coeurs à la VM en cours d'exécution ? Et la mémoire ? 

## Création du bridge 

1. Ouvrir le fichier `/etc/network/interfaces`
2. Rajoutons ceci 

```
auto br0
iface br0 inet dhcp 
  bridge_ports eth0 
  bridge_stp off       # disable Spanning Tree Protocol
  bridge_waitport 0    # no delay before a port becomes available
  bridge_fd 0   
``` 

3. Lancer `/etc/init.d/networking restart` et vérifier si le bridge est crée


## Little customization 

Update Xen's source code to tweak Xen initialization message (/etc/init.d/xencommons start) 
to add a custom message with your name, e.g.,

```
############################
#.      .CUSTOMIZED BY.    #
#.        DJOBIII2078.     #
############################
```
