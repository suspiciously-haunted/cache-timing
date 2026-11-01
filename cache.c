#include <pthread.h>
#include <pthread/qos.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define LINE 128
typedef struct { unsigned char key; char pad[LINE - 1]; } item;

static volatile uintptr_t sink;

static uint64_t bubble(item a[], size_t n) {
    uint64_t comparisons = 0;
    for (size_t pass = 0; pass + 1 < n; pass++) {
        int swapped = 0;
        size_t end = n - 1 - pass;
        comparisons += end;
        for (size_t j = 0; j < end; j++) {
            if (a[j].key > a[j + 1].key) {
                unsigned char tmp = a[j].key;
                a[j].key = a[j + 1].key;
                a[j + 1].key = tmp;
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
    return comparisons;
}

static void fill(item a[], size_t n, int random_input) {
    for (size_t i = 0; i < n; i++)
        a[i].key = random_input ? (unsigned char)arc4random_uniform(256)
                                : (unsigned char)(255 - i * 256 / n);
}

static double bubble_ns(item *a, size_t n, int random_input, int reps) {
    double best = INFINITY;
    for (int r = 0; r < reps; r++) {
        fill(a, n, random_input);
        uint64_t t0 = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
        uint64_t comparisons = bubble(a, n);
        uint64_t t1 = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
        sink = a[n / 2].key;
        double ns = (double)(t1 - t0) / comparisons;
        if (ns < best) best = ns;
    }
    return best;
}

/* Links every line into one random cycle (Sattolo's algorithm). */
static double chase_ns(char *buf, size_t n, int reps) {
    size_t *order = malloc(n * sizeof *order);
    for (size_t i = 0; i < n; i++) order[i] = i;
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = arc4random_uniform((uint32_t)i);
        size_t tmp = order[i]; order[i] = order[j]; order[j] = tmp;
    }
    for (size_t i = 0; i < n; i++)
        *(void **)(buf + order[i] * LINE) = buf + order[(i + 1) % n] * LINE;
    free(order);

    long steps = 20000000;
    void *p = buf;
    for (size_t i = 0; i < n; i++) p = *(void **)p;   /* warm up */

    double best = INFINITY;
    for (int r = 0; r < reps; r++) {
        uint64_t t0 = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
        for (long i = 0; i < steps; i++) p = *(void **)p;
        uint64_t t1 = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
        double ns = (double)(t1 - t0) / steps;
        if (ns < best) best = ns;
    }
    sink = (uintptr_t)p;
    return best;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s reversed|random|chase\n", argv[0]);
        return 1;
    }
    int chase = strcmp(argv[1], "chase") == 0;
    int random_input = strcmp(argv[1], "random") == 0;
    size_t max_bytes = (chase ? 256u : 32u) << 20;

    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    printf("test,bytes,ns_per_op\n");

    for (int i = 0; ; i++) {
        size_t bytes = (size_t)(16384 * pow(2.0, i / 4.0)) / LINE * LINE;
        if (bytes > max_bytes) break;
        size_t n = bytes / LINE;
        void *buf;
        if (posix_memalign(&buf, LINE, bytes)) return 1;
        memset(buf, 0, bytes);
        double ns = chase ? chase_ns(buf, n, 5)
                          : bubble_ns(buf, n, random_input, bytes <= (8u << 20) ? 3 : 1);
        printf("%s,%zu,%.3f\n", argv[1], bytes, ns);
        fflush(stdout);
        free(buf);
    }
    return 0;
}
