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
