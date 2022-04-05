# TP1

## Goal 

The goal of this labwork is to play with VMs and grasp the difficulties around them.
We will use Xen and KVM to create 2 VMs. These VMs will be considered as your physical servers. Then,
in these physical servers, we will install Ubuntu, install Xen (with its hypervisor) and then create a
VM. Then we will collect metrics such as boot time, performance, and latency, and experiment migration.

## Xen install 

There are basically two ways of installing Xen on a Linux kernel operating system. 
The first is to manually build it from sources and the second is to install pre-built binaries. 
We will try both. 

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



