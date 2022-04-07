# TP2 

The goal of this practical course is to gather several metrics regarding VMs (to later compare them with other isolation units).

## Prerequisites

First, you should install Xen as in the [TP1](https://github.com/djobiii2078/cloud_course_resources/tree/main/TP/TP1). 

## Build a custom dom0 kernel 

Now, we will configure a custom dom0 kernel and check if the dom0 kernel can have an impact on the performance of your VMs. 

### Build a custom kernel 

Let’s build our custom kernel. The following steps assumes that you’re running either Ubuntu or Debian.
- Firstly, download a Linux kernel archive (hereafter, I choose Linux 4.4.262).
```
wget https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-4.4.262.tar.xz
```
- Before building, let’s install the necessary dependencies (you may need additional dependencies)
```
sudo apt-get install git fakeroot build-essential ncurses-dev
xz-utils libssl-dev bc flex libelf-dev bison
```
- Unzip the archive
```
sudo tar xfv linux-4.4.262.tar.xz
```

- At this step, you need to define the modules you want for your linux kernel (you can
do that by entering in the linux kernel folder and typing make menuconfig), but we will
just copy the existing config on your machine
```cd linux-4.4.262 && cp /boot/config-$(uname -r) .config```

- Load the copied menuconfig. Do that by launching `make menuconfig` and loading `.config` file.

- Build the kernel based on the existing config file you just copied.

```
make 
``` 

(If you're confident, you can parallelize the compilation)

- If successful, build the necessary kernel modules : 

```
sudo make modules_install 
```

- Finally, install the binaries at the corresponding locations : 

```
sudo make install 
```

- Update your GRUB to refresh the list of available kernels : 

```
sudo update-grub 
```

- Reboot and select your custom kernel with Xen and check if Xen services launch correctly upon boot.  

## dom0 kernel impact 

Now, let's perform micro-evaluations to see if the dom0 kernel change affects the virtualization system.

### VM creation time 

- Create and launch 3 VMs (one after the other), each VM should have 2vCPUs and 2GB of memory. 
- Record the boot time of the 3 VMs (after the completion of the last VM). 

```
Tip : You can the *time* command or *systemd-analyze* command
```

- Repeat the same operation 5 times to get the average, 95th, and 95th percentiles.
- Repeat the experiment with the default kernel and plot a graph to compare both results 

- **What are your observations ?**

### VM delete time 

- Create and launch a simple VM with 2GB.
- Delete the VM and record the completion time.
- Repeat the operation while giving a different memory size to the VM, 4, 6, and 8GB (if possible).
- Repeat the experiment with the default kernel and plot a graph to compare both results
- **Comment your results.**

```
Remember to repeat each operation 5 times to get the average, 95th, and 95th percentiles.
```

### VM migration time 

To perform this part, you need to collaborate with a neighbour, basically you will *live migrate* your VM to your neighbour and vice-versa.

**Setting network storage**

- You first need to setup a shared folder over the network with the selection neighbour, so that the target host can access the received VM image.
To achieve that, you need to install a network shared folder. We use *NFS*
```
sudo apt-get install nfs-server nfs-common openssh-server
nano /etc/exports
```

Add the corresponding line to the export file : `/home/user/vms *(rw,sync,no_root_squash)`. 
This will share the folder `/home/user/vms` over the network. (Update this line based on your settings)

**Allowing root ssh login over the network**

Restart nfs to update its settings for our new shared folder : `sudo /etc/init.d/nfs-kernel-server restart`

- Now, you should enable `root` login over the network on `ssh`. 
To achieve that, update the `/etc/ssh/sshd_config` file and put `yes` to the `PermitRootLogin` option.
- Reload the `ssh` service : `sudo service ssh reload` 

** Migration of the VM** 
- On the target host, let's mount the shared folder under the same tree (Update based on your settings)

```
sudo apt-get install nfs-common
sudo mount -t nfs <IP address of initial>:/home/user/vms /home/user/vms
```
- Check if the folder is correctly mapped.
- On the initial host (where the VM is running), trigger the migration : `sudo xl migrate vmname <IP address of neighbour>`

**Metrics comparaison**

- Migrate a VM of 2GB to your neighbour and get the migration time.
- Reapeat for different memory sizes of the VM. 
- Repeat the operation with a different dom0 kernel and compare. 

```
Remember to repeat each operation 5 times to get the average, 95th, and 95th percentiles.
```

### Impact on VM applications.

The goal is to vary the dom0 resources and evaluate the impact on the VM application. 



