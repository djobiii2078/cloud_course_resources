# TP2 

The goal of this practical course is to perform live migration of a VM and analyse the impact of configuration on administrative tasks.

## Prerequisites

First, you should install Xen as in the [TP1](https://github.com/djobiii2078/cloud_course_resources/tree/main/TP/TP1). 


### VM creation time 

- Create and launch 3 VMs (one after the other), each VM should have 2vCPUs and 2GB of memory. 
- Record the boot time of the 3 VMs (after the completion of the last VM). 

```
Tip : You can the *time* command or *systemd-analyze* command
```

- Repeat the same operation 5 times to get the average, 95th, and 95th percentiles.
- Vary the number of allocated vCPUs and memory, and repeat the same process. 

- **What are your observations ?**

### VM delete time 

- Create and launch a simple VM with 2GB.
- Delete the VM and record the completion time.
- Repeat the operation while giving a different memory size to the VM, 4, 6, and 8GB (if possible).
- **Comment your results.**

```
Remember to repeat each operation 5 times to obtain the average, 95th percentile, and 95th percentile.
```

### VM migration time 

To perform this part, you need to collaborate with a neighbour; basically, you will *live migrate* your VM to your neighbour and vice-versa.
In your VM, launch a bash program that prints the date and time on a single line (in append mode) every 10ms. It will help determine the downtime of the live migration process. 

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

```
Remember to repeat each operation 5 times to get the average, 95th, and 95th percentiles.
```



