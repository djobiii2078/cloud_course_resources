# Monitoring, Firewall, et Port forwarding avec eBPF/XDP

Dans ce TP, nous allons construire un mécanisme de monitoring et de firewall avec eBPF/XDP et vous aurez le soin d'implémenter un système de PORT FORWARDING.
Préparez l'environnement en récupérant de vos machines en installant ces paquets:


```
apt update && \
    apt install -y \
    linux-headers-$(uname -r) \
    build-essential \
    g++ \
    gcc \
    libelf-dev \
    libzstd-dev \
    make \
    autoconf \
    automake \
    autotools-dev \
    libtool \
    pkg-config \
    sudo \
    iproute2 \
    iputils-ping \
    iptables \
    curl \
    iperf3 \
    net-tools \
    tcpdump \
    git \
    python3 \
    python3-pip \
    nano \
    python3-scapy \
    tmux \
    wget \
    libbpfcc-dev \
    libbpf-dev \
    llvm \
    clang \
    gcc-multilib \
    build-essential \
    linux-tools-$(uname -r) \
    linux-tools-common \
    linux-tools-generic
```



## Monitoring / Firewall 

L'objectif est de capturer les paquets arrivant sur l'interface Ethernet principale de votre machine et d'afficher leurs informations (numéro de séquence, protocole, adresses source et destination, ainsi que les ports).

Comme au dernier TCP, nous allons écrire un hook XDP qui parcourra les paquets et y ajouter une logique d'affichage. 
Malheureusement, la primitive d'affichage bpf_printk est limitée en termes d'arguments et rallonger le délai de traitement de paquets sur le chemin critique affecterait les performances. 
Pour pallier ce problème, nous allons transmettre les informations recueillies dans l'espace noyau à l'espace utilisateur qui aura tout le loisir de formater les informations comme il faut. Plusieurs techniques avec eBPF permettent cela, mais où allons-nous utiliser les `perf events` `? Ils permettent d'envoyer des informations et de définir un handler qui sera appelé pour chaque information. 

Pour cela, créer trois fichiers, 

1. `log_xdp.c`: contiendra la logique XDP pour récupérer les différentes informations sur les paquets
2. ` events.h`: continendra la structure de données qu'utilisera notre perf event
3. Et `user_log.c`: contiendra la logique de l'espace utilisateur pour formater les informations du perf event.

`events.h`

```C
#pragma once
#include <linux/types.h>

#define PROTO_UDP 17
#define PROTO_TCP 6

struct packet_event {
    __u64 ts;

    __u8  protocol;
    __u8  ip_version; // 4 or 6
    __u16 sport;
    __u16 dport;

    __u32 sip4;
    __u32 dip4;

    __u64 sip6_hi;
    __u64 sip6_lo;
    __u64 dip6_hi;
    __u64 dip6_lo;
    
    __u32 seq; //sequence number 
};
```
`log_xdp.c`

```C
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/ipv6.h>

#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/pkt_cls.h>    
#include <bpf/bpf_helpers.h>
#include "events.h"



#define DROP_PORT 8080

#ifndef IPPROTO_TCP
#define IPPROTO_TCP 6
#endif
#ifndef IPPROTO_UDP
#define IPPROTO_UDP 17
#endif

#ifndef bpf_htons
#define bpf_htons(x) __builtin_bswap16(x)
#endif
#ifndef bpf_ntohs
#define bpf_ntohs(x) __builtin_bswap16(x)
#endif
#ifndef bpf_htonl
#define bpf_htonl(x) __builtin_bswap32(x)
#endif
#ifndef bpf_ntohl
#define bpf_ntohl(x) __builtin_bswap32(x)
#endif
#define IPV6_HDR_LEN 40   // fixed-length IPv6 base header

#define IPV6_HDR_LEN 40

struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(__u32));
    __uint(value_size, sizeof(__u32));
} events SEC(".maps");


SEC("xdp")
int log_packets(struct xdp_md *ctx)
{
    void *data     = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    __u64 offset = 0;

    // --- Ethernet ---
    struct ethhdr *eth = data;
    offset = sizeof(*eth);
    if ((void*)eth + offset > data_end)
        return XDP_PASS;

    __u16 h_proto = eth->h_proto;
    __u16 proto = bpf_ntohs(h_proto);

    struct packet_event evt = {};
    evt.ts = bpf_ktime_get_ns();

    // ==========================
    // === IPv4 PROCESSING =====
    // ==========================
    if (proto == ETH_P_IP) {

        struct iphdr *iph = data + offset;
        if ((void*)iph + sizeof(*iph) > data_end)
            return XDP_PASS;

        offset += sizeof(*iph);
        evt.ip_version = 4;
        evt.protocol = iph->protocol;
        evt.sip4 = iph->saddr;
        evt.dip4 = iph->daddr;

        // TCP
        if (iph->protocol == IPPROTO_TCP) {
            struct tcphdr *tcph = data + offset;
            if ((void*)tcph + sizeof(*tcph) > data_end)
                return XDP_PASS;

            evt.sport = bpf_ntohs(tcph->source);
            evt.dport = bpf_ntohs(tcph->dest);
            ev.seq   = bpf_ntohl(tcp->seq); 

            bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU,
                                  &evt, sizeof(evt));
        }

        // UDP
        else if (iph->protocol == IPPROTO_UDP) {
            struct udphdr *udph = data + offset;
            if ((void*)udph + sizeof(*udph) > data_end)
                return XDP_PASS;

            evt.sport = bpf_ntohs(udph->source);
            evt.dport = bpf_ntohs(udph->dest);
            ev.seq = 0;


            bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU,
                                  &evt, sizeof(evt));
        }

        return XDP_PASS;
    }


    // ==========================
    // === IPv6 PROCESSING =====
    // ==========================
    if (proto == ETH_P_IPV6) {

        struct ipv6hdr *ip6h = data + offset;
        if ((void*)ip6h + IPV6_HDR_LEN > data_end)
            return XDP_PASS;

        offset += IPV6_HDR_LEN;

        evt.ip_version = 6;
        evt.protocol = ip6h->nexthdr;

        // convert IPv6 → 128 bits hi/lo
        __u32 *s32 = (__u32 *)&ip6h->saddr;
        __u32 *d32 = (__u32 *)&ip6h->daddr;

        evt.sip6_hi = ((__u64)bpf_ntohl(s32[0]) << 32) | bpf_ntohl(s32[1]);
        evt.sip6_lo = ((__u64)bpf_ntohl(s32[2]) << 32) | bpf_ntohl(s32[3]);
        evt.dip6_hi = ((__u64)bpf_ntohl(d32[0]) << 32) | bpf_ntohl(d32[1]);
        evt.dip6_lo = ((__u64)bpf_ntohl(d32[2]) << 32) | bpf_ntohl(d32[3]);

        // TCP
        if (evt.protocol == IPPROTO_TCP) {
            struct tcphdr *tcph = data + offset;
            if ((void*)tcph + sizeof(*tcph) > data_end)
                return XDP_PASS;

            evt.sport = bpf_ntohs(tcph->source);
            evt.dport = bpf_ntohs(tcph->dest);

            bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU,
                                  &evt, sizeof(evt));
        }

        // UDP
        else if (evt.protocol == IPPROTO_UDP) {
            struct udphdr *udph = data + offset;
            if ((void*)udph + sizeof(*udph) > data_end)
                return XDP_PASS;

            evt.sport = bpf_ntohs(udph->source);
            evt.dport = bpf_ntohs(udph->dest);

            bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU,
                                  &evt, sizeof(evt));
        }

        return XDP_PASS;
    }

    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
```

`user_log.c`: La particularité de ce code réside dans les fonctions `handle_events`, qui gèrent le mécanisme de perf event, et `bpf_set_link_xdp`, qui charge un programme xdp directement sur une interface cible. 

```C
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <net/if.h>
#include <errno.h>

#include <bpf/libbpf.h>
#include <bpf/bpf.h>

#include "events.h"

static volatile int exiting = 0;

static void sig_handler(int signo)
{
    exiting = 1;
}

static void print_ipv4(__u32 addr)
{
    unsigned char *b = (unsigned char *)&addr;
    printf("%u.%u.%u.%u", b[3], b[2], b[1], b[0]);
}

static void print_ipv6(__u64 hi, __u64 lo)
{
    struct in6_addr addr = {0};

    addr.s6_addr32[0] = htonl((hi >> 32) & 0xffffffff);
    addr.s6_addr32[1] = htonl(hi & 0xffffffff);
    addr.s6_addr32[2] = htonl((lo >> 32) & 0xffffffff);
    addr.s6_addr32[3] = htonl(lo & 0xffffffff);

    char buf[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &addr, buf, sizeof(buf));
    printf("%s", buf);
}

static void handle_event(void *ctx, int cpu, void *data, __u32 len)
{
    struct packet_event *e = data;

    printf("[TS=%llu] ", e->ts);

    if (e->ip_version == 4) {
        printf("IPv4 ");
        printf(e->protocol == PROTO_TCP ? "TCP " : "UDP ");
        print_ipv4(e->sip4);
        printf(":%u -> ", e->sport);
        print_ipv4(e->dip4);
        printf(":%u", e->dport);
        if (e->protocol == PROTO_TCP)
            printf(" SEQ=%u", e->seq);
    }

    else if (e->ip_version == 6) {
        printf("IPv6 ");
        printf(e->protocol == PROTO_TCP ? "TCP " : "UDP ");
        print_ipv6(e->sip6_hi, e->sip6_lo);
        printf(":%u -> ", e->sport);
        print_ipv6(e->dip6_hi, e->dip6_lo);
        printf(":%u", e->dport);
        if (e->protocol == PROTO_TCP)
            printf(" SEQ=%u", e->seq);
    }

}

static void handle_lost(void *ctx, int cpu, __u64 lost_cnt)
{
    fprintf(stderr, "Lost %llu events on CPU %d\n", lost_cnt, cpu);
}

int main(int argc, char **argv)
{
    struct bpf_object *obj;
    struct bpf_program *prog;
    struct perf_buffer *pb;
    struct perf_buffer_opts pb_opts = {};
    int ifindex, map_fd, prog_fd, err;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <iface> <prog.o>\n", argv[0]);
        return 1;
    }

    ifindex = if_nametoindex(argv[1]);
    if (!ifindex) {
        perror("if_nametoindex");
        return 1;
    }

    obj = bpf_object__open_file(argv[2], NULL);
    if (libbpf_get_error(obj)) {
        fprintf(stderr, "Failed to open BPF ELF\n");
        return 1;
    }

    if (bpf_object__load(obj)) {
        fprintf(stderr, "Failed to load BPF object\n");
        return 1;
    }

    prog = bpf_object__find_program_by_title(obj, "xdp");
    if (!prog) {
        fprintf(stderr, "XDP program not found in object\n");
        return 1;
    }

    prog_fd = bpf_program__fd(prog);

    if ((err = bpf_set_link_xdp_fd(ifindex, prog_fd, 0)) < 0) {
        fprintf(stderr, "Failed to attach XDP: %d\n", err);
        return 1;
    }

    printf("XDP program attached to %s\n", argv[1]);

    map_fd = bpf_object__find_map_fd_by_name(obj, "events");
    if (map_fd < 0) {
        fprintf(stderr, "Could not find 'events' map\n");
        return 1;
    }
	
	//struct perf_buffer_opts pb_opts = {};
	pb_opts.sample_cb = handle_event;
	pb_opts.lost_cb = handle_lost;

	pb = perf_buffer__new(map_fd, 8, &pb_opts);
	if (!pb) {
	    fprintf(stderr, "Failed to setup perf buffer\n");
	    return 1;
	}


    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    printf("Listening for events... press Ctrl-C to exit.\n");

    while (!exiting) {
        err = perf_buffer__poll(pb, 100);
        if (err < 0 && err != -EINTR) {
            fprintf(stderr, "Error polling perf buffer: %d\n", err);
            break;
        }
    }

    printf("Detaching XDP...\n");
    bpf_set_link_xdp_fd(ifindex, -1, 0);

    perf_buffer__free(pb);
    bpf_object__close(obj);

    return 0;
}
```

Compilons le programme xdp pour obtenir son exécutable : `clang -O2 -g -target bpf -c log_xdp.c -o log_xdp.o`

Compilons le programme de l'espace utilisateur : `gcc user_log.c -o user_log $(pkg-config --libs --cflags libbpf) -lelf -lz`

Lancez le programme: `sudo ./user_log <interface> log_xdp.o` où <interface> est l'interface où vous voulez rattacher votre programme de monitoring.

**A vous**

(Tester la fonctionnalité de votre code avec `iperf3`) 

Modifier le programme précédent pour filtrer les paquets arrivant sur les ports 3000 et 8008, comme dans le dernier TP. Pour ces paquets rejetés, ajouter une colonne "ACCEPT" indiquant si le paquet correspondant a été accepté ou rejeté. (Les paquets arrivant sur les ports 3000/8008 doivent être affichés en "REJET").

## PORT FORWARDING

Modifier votre précédent afin de rediriger les paquets arrivant sur le port 3000/8008 vers deux machines différentes en round robin. 
Concrètement, à l'initialisation, votre programme devra charger les adresses mac des deux serveurs dans un `map`, puis rediriger les paquets vers chaque machine, à raison d'un paquet par machine. L'idée, c'est qu'au lieu de le rejetter, on le forwarde vers une autre machine. 
