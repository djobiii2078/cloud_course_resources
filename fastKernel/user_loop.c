// user_loop.c
#include <stdio.h>
#include <time.h>

static inline double now() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main() {
    volatile long x = 0;
    double t0 = now();
    for (long i = 0; i < 100000000; i++) {  // 100 million
        x++;
    }
    double t1 = now();
    printf("User-space loop: %.6f seconds\n", t1 - t0);
}

