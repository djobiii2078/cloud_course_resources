# Prologue: TP Cloud — Deux services dans des conteneurs, une microVM et une VM QEMU

## 1. Objectifs

Ce TP compare trois niveaux d'isolation en déployant exactement les mêmes services :

1. **Service A — Health server** : répond `200 OK` sur `/health`.
2. **Service B — Key server** : génère une nouvelle valeur aléatoire de **2 048 octets** à chaque requête `GET /key`.

Les étudiants doivent comprendre, recopier, commenter et adapter les scripts. Aucun script séparé n'est fourni.

> **Attention à l'unité.** 2 048 octets correspondent à 16 384 bits. Il ne s'agit pas d'une clé RSA de 2 048 bits. Dans ce TP, le mot *clé* désigne un bloc de données aléatoires.

Les trois déploiements sont :

- deux conteneurs;
- une microVM QEMU utilisant `-machine microvm` et VirtIO-MMIO;
- une VM QEMU classique utilisant `q35` et VirtIO-PCI.

## 2. Résultats à remettre

- le code des deux serveurs;
- les scripts recopiés et commentés;
- les configurations finales des deux noyaux;
- les mesures brutes au format CSV;
- un tableau p50, p95, p99 et maximum;
- une analyse comparant isolation, démarrage, RSS hôte, taille et surface d'attaque.

## 3. Préparation de l'hôte

Le TP suppose Ubuntu x86-64, KVM et au moins 20 Go libres.

Créer `00_install_host.sh` :

```bash
#!/usr/bin/env bash
set -euo pipefail

if [[ ${EUID} -ne 0 ]]; then
  echo "Utilisation : sudo $0" >&2
  exit 1
fi

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y \
  build-essential bc bison flex libssl-dev libelf-dev dwarves \
  qemu-system-x86 qemu-utils busybox-static cpio gzip \
  curl jq time docker.io

systemctl enable --now docker
usermod -aG kvm,docker "${SUDO_USER}"

test -e /dev/kvm || echo "ERREUR : /dev/kvm est absent" >&2
qemu-system-x86_64 --version | head -n1
docker --version
```

Puis :

```bash
chmod +x 00_install_host.sh
sudo ./00_install_host.sh
newgrp kvm
```

### Questions

1. Quel est le rôle de `/dev/kvm` ?
2. Quelle différence existe entre KVM et QEMU ?
3. Pourquoi l'utilisateur rejoint-il les groupes `kvm` et `docker` ?

---

# TP1 — Déploiement dans deux conteneurs

## 4. Service A : serveur de contrôle

Créer `health_server.py` :

```python
#!/usr/bin/env python3
import json
import os
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

START = time.monotonic_ns()


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path != "/health":
            self.send_error(404)
            return

        body = json.dumps({
            "status": "ok",
            "service": "health",
            "runtime": os.getenv("RUNTIME_KIND", "unknown"),
            "uptime_ms": (time.monotonic_ns() - START) // 1_000_000,
        }).encode()

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):
        return


ThreadingHTTPServer(("0.0.0.0", 8080), Handler).serve_forever()
```

Tester localement :

```bash
python3 health_server.py &
HEALTH_PID=$!
curl -i http://127.0.0.1:8080/health
kill "${HEALTH_PID}"
```

## 5. Service B : générateur de blocs aléatoires

Créer `key_server.py` à partir du squelette suivant. Les parties `TODO` doivent être complétées par les étudiants.

```python
#!/usr/bin/env python3
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

KEY_BYTES = 2048


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path != "/key":
            self.send_error(404)
            return

        # TODO 1 : obtenir exactement KEY_BYTES octets avec le CSPRNG du système.
        payload = b""

        # TODO 2 : vérifier la taille avant d'envoyer la réponse.
        assert False

        self.send_response(200)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("X-Key-Bytes", str(KEY_BYTES))
        self.end_headers()
        self.wfile.write(payload)

    def log_message(self, fmt, *args):
        return


ThreadingHTTPServer(("0.0.0.0", 8081), Handler).serve_forever()
```

Contraintes :

- ne pas utiliser `random.random()` ni `random.randbytes()`;
- produire une nouvelle valeur à chaque requête;
- renvoyer des octets bruts, sans hexadécimal ni Base64;
- ne jamais écrire la valeur générée dans les logs;
- gérer au moins dix requêtes concurrentes.

### Test fonctionnel imposé

Créer `test_key_server.sh` :

```bash
#!/usr/bin/env bash
set -euo pipefail

URL=${1:-http://127.0.0.1:8081/key}
TMP=$(mktemp -d)
trap 'rm -rf "${TMP}"' EXIT

curl --fail --silent "${URL}" -o "${TMP}/key1.bin"
curl --fail --silent "${URL}" -o "${TMP}/key2.bin"

test "$(stat -c %s "${TMP}/key1.bin")" -eq 2048
test "$(stat -c %s "${TMP}/key2.bin")" -eq 2048

if cmp -s "${TMP}/key1.bin" "${TMP}/key2.bin"; then
  echo "ERREUR : deux réponses identiques" >&2
  exit 1
fi

echo "Test réussi : deux réponses distinctes de 2 048 octets"
```

### Questions

1. Pourquoi `os.urandom()` ou `secrets.token_bytes()` convient-il mieux que le module `random` ?
2. Pourquoi une représentation Base64 aurait-elle une taille supérieure à 2 048 octets ?
3. Faut-il conserver les valeurs générées ? Justifier selon le modèle de menace.

## 6. Images des conteneurs

Créer `Dockerfile.health` :

```dockerfile
FROM python:3.13-alpine
WORKDIR /opt/app
COPY health_server.py /opt/app/server.py
ENV RUNTIME_KIND=container
USER 65532:65532
EXPOSE 8080
ENTRYPOINT ["python3", "/opt/app/server.py"]
```

Créer `Dockerfile.key` :

```dockerfile
FROM python:3.13-alpine
WORKDIR /opt/app
COPY key_server.py /opt/app/server.py
USER 65532:65532
EXPOSE 8081
ENTRYPOINT ["python3", "/opt/app/server.py"]
```

Construire les deux images :

```bash
docker build -t tp-health:latest -f Dockerfile.health .
docker build -t tp-key:latest -f Dockerfile.key .
docker image ls tp-health tp-key
```

Créer `run_containers.sh` :

```bash
#!/usr/bin/env bash
set -euo pipefail

docker rm -f tp-health tp-key >/dev/null 2>&1 || true

COMMON=(
  --read-only
  --tmpfs /tmp:rw,noexec,nosuid,size=16m
  --cap-drop=ALL
  --security-opt=no-new-privileges
  --memory=256m
  --cpus=1
)

docker run -d --name tp-health "${COMMON[@]}" \
  -p 127.0.0.1:8080:8080 tp-health:latest

docker run -d --name tp-key "${COMMON[@]}" \
  -p 127.0.0.1:8081:8081 tp-key:latest

curl --retry 20 --retry-connrefused http://127.0.0.1:8080/health
./test_key_server.sh http://127.0.0.1:8081/key
```

### Manipulations

1. Retirer `--cap-drop=ALL`, puis comparer `CapEff` dans `/proc/1/status`.
2. Essayer d'écrire dans `/` avec et sans `--read-only`.
3. Limiter le key server à 32 Mio et provoquer 100 requêtes concurrentes.
4. Relever le p50, p95, p99 et le taux d'erreur.

---

# TP2 — MicroVM et VM classique avec QEMU

## 7. Pourquoi deux noyaux ?

La microVM QEMU n'utilise ni PCI ni ACPI. Les périphériques réseau passent par VirtIO-MMIO. La VM classique `q35` utilise PCI et VirtIO-PCI. Les étudiants construiront donc :

- `bzImage-microvm`, sans PCI ni ACPI;
- `bzImage-classic`, avec PCI et VirtIO-PCI;
- un initramfs commun contenant BusyBox et les deux serveurs.

## 8. Configuration minimale de la microVM

Créer `microvm.fragment` :

```text
CONFIG_64BIT=y
CONFIG_X86_64=y
CONFIG_SMP=y
CONFIG_PRINTK=y
CONFIG_TTY=y
CONFIG_BINFMT_ELF=y
CONFIG_BINFMT_SCRIPT=y
CONFIG_BLK_DEV_INITRD=y
CONFIG_RD_GZIP=y
CONFIG_DEVTMPFS=y
CONFIG_DEVTMPFS_MOUNT=y
CONFIG_TMPFS=y
CONFIG_PROC_FS=y
CONFIG_SYSFS=y
CONFIG_SERIAL_8250=y
CONFIG_SERIAL_8250_CONSOLE=y
CONFIG_HYPERVISOR_GUEST=y
CONFIG_KVM_GUEST=y
CONFIG_PARAVIRT=y
CONFIG_VIRTIO=y
CONFIG_VIRTIO_MMIO=y
CONFIG_VIRTIO_MMIO_CMDLINE_DEVICES=y
CONFIG_VIRTIO_NET=y
CONFIG_HW_RANDOM=y
CONFIG_HW_RANDOM_VIRTIO=y
CONFIG_NET=y
CONFIG_INET=y
CONFIG_PACKET=y
CONFIG_UNIX=y
CONFIG_NETDEVICES=y
CONFIG_PCI=n
CONFIG_ACPI=n
CONFIG_MODULES=n
CONFIG_DEBUG_KERNEL=n
CONFIG_SOUND=n
CONFIG_USB_SUPPORT=n
CONFIG_WLAN=n
CONFIG_WIRELESS=n
CONFIG_DRM=n
```

## 9. Configuration de la VM classique

Créer `classic.fragment` en reprenant le fragment précédent et en appliquant les changements suivants :

```text
CONFIG_PCI=y
CONFIG_PCI_MSI=y
CONFIG_ACPI=y
CONFIG_VIRTIO_PCI=y
CONFIG_HW_RANDOM=y
CONFIG_HW_RANDOM_VIRTIO=y
CONFIG_VIRTIO_MMIO=n
CONFIG_VIRTIO_MMIO_CMDLINE_DEVICES=n
```

### Travail demandé avant compilation

Pour chaque option, ajouter un commentaire dans le rapport indiquant :

- quel composant l'utilise;
- ce qui se passe si elle est désactivée;
- si elle augmente la surface d'attaque;
- si elle est nécessaire aux deux configurations.

## 10. Construction des noyaux

Créer `build_kernels.sh` :

```bash
#!/usr/bin/env bash
set -euo pipefail

LINUX_VERSION=${LINUX_VERSION:-6.12.116}
JOBS=${JOBS:-$(nproc)}
ROOT=$(pwd)
WORK=${ROOT}/build
SRC=${WORK}/linux-${LINUX_VERSION}
TARBALL=${WORK}/linux-${LINUX_VERSION}.tar.xz

mkdir -p "${WORK}" "${WORK}/artifacts"

if [[ ! -f ${TARBALL} ]]; then
  curl -fL \
    "https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${LINUX_VERSION}.tar.xz" \
    -o "${TARBALL}"
fi

if [[ ! -d ${SRC} ]]; then
  tar -C "${WORK}" -xf "${TARBALL}"
fi

build_one() {
  local name=$1
  local fragment=$2
  local out=${WORK}/kernel-${name}

  make -C "${SRC}" O="${out}" tinyconfig

  while IFS= read -r entry; do
    [[ -z ${entry} || ${entry} == \#* ]] && continue
    key=${entry%%=*}
    value=${entry#*=}

    case ${value} in
      y) "${SRC}/scripts/config" --file "${out}/.config" \
           --enable "${key#CONFIG_}" ;;
      n) "${SRC}/scripts/config" --file "${out}/.config" \
           --disable "${key#CONFIG_}" ;;
      *) echo "Valeur non prise en charge : ${entry}" >&2; exit 1 ;;
    esac
  done < "${fragment}"

  make -C "${SRC}" O="${out}" olddefconfig
  make -C "${SRC}" O="${out}" -j"${JOBS}" bzImage

  cp "${out}/arch/x86/boot/bzImage" "${WORK}/artifacts/bzImage-${name}"
  cp "${out}/.config" "${WORK}/artifacts/config-${name}"
}

build_one microvm microvm.fragment
build_one classic classic.fragment

du -h "${WORK}/artifacts"/*
```

### Vérification des options réellement conservées

```bash
grep -E 'CONFIG_(PCI|ACPI|VIRTIO_MMIO|VIRTIO_PCI)=' \
  build/artifacts/config-microvm

grep -E 'CONFIG_(PCI|ACPI|VIRTIO_MMIO|VIRTIO_PCI)=' \
  build/artifacts/config-classic

echo "microVM: $(grep -c '=y$' build/artifacts/config-microvm) options actives"
echo "classique: $(grep -c '=y$' build/artifacts/config-classic) options actives"
```

Ne pas supposer que `scripts/config --enable` a réussi : Kconfig peut retirer une option lorsque ses dépendances ne sont pas satisfaites.

## 11. Key server statique pour le rootfs

Python ne sera pas ajouté au rootfs minimal. Le second serveur doit donc être réécrit en C et compilé statiquement.

Créer `key_server.c` à partir du squelette :

```c
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <unistd.h>

#define KEY_BYTES 2048
#define PORT 8081

static int fill_random(unsigned char *buffer, size_t length) {
    size_t done = 0;
    while (done < length) {
        /* TODO 1 : appeler getrandom() et gérer EINTR. */
        ssize_t ret = -1;
        if (ret < 0) {
            return -1;
        }
        done += (size_t)ret;
    }
    return 0;
}

static int send_all(int fd, const void *buffer, size_t length) {
    const unsigned char *cursor = buffer;
    while (length > 0) {
        /* TODO 2 : appeler send() jusqu'à l'envoi complet. */
        ssize_t ret = -1;
        if (ret <= 0) {
            return -1;
        }
        cursor += ret;
        length -= (size_t)ret;
    }
    return 0;
}

int main(void) {
    int server = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    if (bind(server, (struct sockaddr *)&address, sizeof(address)) < 0 ||
        listen(server, 64) < 0) {
        perror("bind/listen");
        return 1;
    }

    for (;;) {
        int client = accept(server, NULL, NULL);
        if (client < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            return 1;
        }

        char request[1024];
        ssize_t received = recv(client, request, sizeof(request) - 1, 0);
        if (received > 0) {
            request[received] = '\0';
            unsigned char key[KEY_BYTES];

            if (strncmp(request, "GET /key ", 9) == 0 &&
                fill_random(key, sizeof(key)) == 0) {
                const char *header =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/octet-stream\r\n"
                    "Content-Length: 2048\r\n"
                    "Connection: close\r\n\r\n";
                send_all(client, header, strlen(header));
                send_all(client, key, sizeof(key));
                memset(key, 0, sizeof(key));
            } else {
                const char *not_found =
                    "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                send_all(client, not_found, strlen(not_found));
            }
        }
        close(client);
    }
}
```

Compiler :

```bash
gcc -O2 -static -Wall -Wextra -Werror key_server.c -o key_server
file key_server
ldd key_server || true
```

Le résultat attendu de `ldd` est `not a dynamic executable`.

## 12. Création du rootfs minimal

Créer `init` :

```sh
#!/bin/sh
set -eu

mount -t proc proc /proc || true
mount -t sysfs sysfs /sys || true
mount -t devtmpfs devtmpfs /dev || true
mkdir -p /dev/pts /tmp /run
mount -t devpts devpts /dev/pts || true
mount -t tmpfs tmpfs /tmp || true

ip link set lo up
ip link set eth0 up
udhcpc -i eth0 -q -n -s /etc/udhcpc.script || true

httpd -f -p 8080 -h /www &
/bin/key_server &

echo "READY health=8080 key=8081"

while true; do
  sleep 3600 &
  wait $!
done
```

Créer `udhcpc.script` :

```sh
#!/bin/sh
set -eu

case "${1:-}" in
  deconfig)
    ip addr flush dev "${interface}"
    ;;
  bound|renew)
    ip addr flush dev "${interface}"
    ip addr add "${ip}/${subnet:-24}" dev "${interface}"
    if [ -n "${router:-}" ]; then
      ip route replace default via "${router%% *}" dev "${interface}"
    fi
    ;;
esac
```

Créer `build_rootfs.sh` :

```bash
#!/usr/bin/env bash
set -euo pipefail

ROOT=$(pwd)
ROOTFS=${ROOT}/build/rootfs
OUT=${ROOT}/build/artifacts/rootfs.cpio.gz

rm -rf "${ROOTFS}"
mkdir -p "${ROOTFS}"/{bin,etc,proc,sys,dev,tmp,run,www}

cp /usr/bin/busybox "${ROOTFS}/bin/busybox"
cp ./key_server "${ROOTFS}/bin/key_server"
cp ./init "${ROOTFS}/init"
cp ./udhcpc.script "${ROOTFS}/etc/udhcpc.script"

for command in sh mount mkdir ip udhcpc httpd sleep; do
  ln -s busybox "${ROOTFS}/bin/${command}"
done

printf '%s\n' '{"status":"ok","service":"health"}' \
  > "${ROOTFS}/www/health"

chmod +x "${ROOTFS}/init" "${ROOTFS}/etc/udhcpc.script" \
  "${ROOTFS}/bin/key_server"

mkdir -p "$(dirname "${OUT}")"
(
  cd "${ROOTFS}"
  find . -print0 | cpio --null -o --format=newc 2>/dev/null | gzip -9
) > "${OUT}"

du -h "${OUT}" "${ROOTFS}/bin/key_server"
```

## 13. Lancement de la microVM

Créer `run_microvm.sh` :

```bash
#!/usr/bin/env bash
set -euo pipefail

ART=build/artifacts
mkdir -p build/run

qemu-system-x86_64 \
  -machine microvm,accel=kvm,pit=off,pic=off,rtc=off \
  -cpu host -smp 1 -m 256M \
  -nodefaults -no-user-config -display none -monitor none \
  -kernel "${ART}/bzImage-microvm" \
  -initrd "${ART}/rootfs.cpio.gz" \
  -append "console=ttyS0 reboot=t panic=1 pci=off quiet" \
  -netdev user,id=net0,hostfwd=tcp:127.0.0.1:8082-:8080,hostfwd=tcp:127.0.0.1:8182-:8081 \
  -device virtio-net-device,netdev=net0 \
  -device virtio-rng-device \
  -serial file:build/run/microvm.log \
  -pidfile build/run/microvm.pid \
  -daemonize
```

Tests :

```bash
curl --retry 50 --retry-connrefused http://127.0.0.1:8082/health
./test_key_server.sh http://127.0.0.1:8182/key
```

## 14. Lancement de la VM classique

Créer `run_classic.sh` :

```bash
#!/usr/bin/env bash
set -euo pipefail

ART=build/artifacts
mkdir -p build/run

qemu-system-x86_64 \
  -machine q35,accel=kvm \
  -cpu host -smp 1 -m 512M \
  -display none -monitor none -no-reboot \
  -kernel "${ART}/bzImage-classic" \
  -initrd "${ART}/rootfs.cpio.gz" \
  -append "console=ttyS0 panic=1 quiet" \
  -netdev user,id=net0,hostfwd=tcp:127.0.0.1:8083-:8080,hostfwd=tcp:127.0.0.1:8183-:8081 \
  -device virtio-net-pci,netdev=net0 \
  -device virtio-rng-pci \
  -serial file:build/run/classic.log \
  -pidfile build/run/classic.pid \
  -daemonize
```

Tests :

```bash
curl --retry 50 --retry-connrefused http://127.0.0.1:8083/health
./test_key_server.sh http://127.0.0.1:8183/key
```

### Questions

1. Pourquoi `virtio-net-device` fonctionne-t-il avec `microvm` ?
2. Pourquoi la VM `q35` utilise-t-elle `virtio-net-pci` ?
3. Pourquoi le rootfs peut-il être identique alors que les noyaux diffèrent ?
4. Quel déploiement possède la plus grande base logicielle de confiance ?
5. La taille du noyau prédit-elle directement le temps de démarrage ?
6. Pourquoi un périphérique VirtIO RNG est-il fourni aux deux invités ? Que peut-il arriver à `getrandom()` pendant un démarrage très précoce sans source d'entropie suffisante ?

## 15. Script de mesure commun

Créer `benchmark.sh` :

```bash
#!/usr/bin/env bash
set -euo pipefail

RUNS=${1:-20}
OUT=results.csv
echo "target,run,startup_ms,health_code,key_bytes,rss_kib" > "${OUT}"

now_ns() { date +%s%N; }

wait_http() {
  local url=$1
  local deadline=$((SECONDS + 60))
  while (( SECONDS < deadline )); do
    code=$(curl -s -o /dev/null -w '%{http_code}' --max-time 0.2 "${url}" || true)
    [[ ${code} == 200 ]] && return 0
    sleep 0.02
  done
  return 1
}

measure_vm() {
  local name=$1 run_script=$2 health_port=$3 key_port=$4
  local start end pid rss key_bytes

  "./${run_script}"
  start=$(now_ns)
  wait_http "http://127.0.0.1:${health_port}/health"
  end=$(now_ns)

  curl -s "http://127.0.0.1:${key_port}/key" -o key.tmp
  key_bytes=$(stat -c %s key.tmp)
  rm -f key.tmp

  pid=$(cat "build/run/${name}.pid")
  rss=$(awk '/VmRSS:/ {print $2}' "/proc/${pid}/status")

  echo "${name},${run},$(((end-start)/1000000)),200,${key_bytes},${rss}" \
    >> "${OUT}"

  kill "${pid}"
  rm -f "build/run/${name}.pid"
}

for run in $(seq 1 "${RUNS}"); do
  measure_vm microvm run_microvm.sh 8082 8182
  measure_vm classic run_classic.sh 8083 8183
done
```

### Erreur à identifier

Le script précédent contient volontairement un problème méthodologique dans la mesure du temps de démarrage. Identifier le problème, le corriger et expliquer son impact sur les résultats.

## 16. Analyse de la latence de queue

Chaque configuration doit recevoir au moins 1 000 requêtes vers `/key`, avec une concurrence de 1, 10 puis 50 clients.

Pour chaque expérience, rapporter :

- débit en requêtes par seconde;
- p50;
- p95;
- p99;
- maximum;
- taux d'erreur;
- RSS côté hôte;
- utilisation CPU;
- taille des artefacts.

Le p99 est important car un système peut avoir une bonne moyenne tout en produisant régulièrement des réponses très lentes. Ces réponses déterminent souvent les timeouts, les retries et l'expérience des utilisateurs.

## 17. Questions finales

1. Quel coût provient du démarrage de l'application et quel coût provient du noyau invité ?
2. Pourquoi le conteneur partage-t-il le noyau de l'hôte ?
3. Quelle configuration limite le mieux les conséquences d'une compromission du key server ?
4. Quelles options noyau faudrait-il ajouter pour eBPF, cgroups, namespaces ou stockage persistant ?
5. La suppression des modules améliore-t-elle toujours la maintenabilité ?
6. Comment signer et versionner ensemble le noyau, le rootfs et l'application ?
7. Comment mesurer l'énergie sans confondre énergie de démarrage et énergie de traitement ?


## 18. Références

- QEMU microvm : https://www.qemu.org/docs/master/system/i386/microvm.html
- QEMU invocation : https://www.qemu.org/docs/master/system/invocation.html
- Linux Kconfig : https://docs.kernel.org/kbuild/kconfig.html
- Linux stable releases : https://www.kernel.org/releases.html
- Docker Ubuntu : https://docs.docker.com/engine/install/ubuntu/
