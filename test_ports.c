/*
 * test_ports.c - unit tests for the port set (ports.h / ports.c).
 *
 * Build and run:  make unit
 * It is compiled with the sanitizers and run under `timeout`, so a loop that
 * never ends (for example a uint16_t counter that cannot reach max + 1)
 * is reported as a failure instead of hanging the terminal.
 *
 * If you rename a function or change its signature in ports.h, update the
 * calls below. The tests only use the five functions from ports.h.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ports.h"

#define LAST_PORT 65535
#define ELEMS     4096            /* 65536 bits / 16 bits per element */

static int checks, failures;

#define CHECK(cond, ...)                                  \
    do {                                                  \
        checks++;                                         \
        if (!(cond)) {                                    \
            failures++;                                   \
            printf("FAIL: ");                             \
            printf(__VA_ARGS__);                          \
            printf("\n");                                 \
            fflush(stdout);                               \
        }                                                 \
    } while (0)

static bitmap *new_set(void)
{
    bitmap *m = calloc(ELEMS, sizeof *m);
    if (m == NULL) {
        perror("calloc");
        exit(2);
    }
    return m;
}

static int count_set(bitmap *m)
{
    int n = 0;
    for (int p = 0; p <= LAST_PORT; p++)
        if (getport(m, (uint16_t)p))
            n++;
    return n;
}

/* every port in [lo, hi] is set and every port outside it is clear */
static int exactly(bitmap *m, int lo, int hi)
{
    for (int p = 0; p <= LAST_PORT; p++) {
        int want = (p >= lo && p <= hi);
        if ((int)getport(m, (uint16_t)p) != want)
            return 0;
    }
    return 1;
}

static void test_empty_set(void)
{
    bitmap *m = new_set();
    CHECK(count_set(m) == 0, "a new set is not empty");
    CHECK(!getport(m, 0) && !getport(m, 65535), "ports 0 and 65535 read as set in a new set");
    free(m);
}

/* every one of the 65536 ports must have its own bit */
static void test_every_port_has_its_own_bit(void)
{
    bitmap *m = new_set();
    int collided = -1, notset = -1;

    for (int p = 0; p <= LAST_PORT; p++) {
        if (getport(m, (uint16_t)p)) { collided = p; break; }   /* an earlier port aliased onto p */
        setport(m, (uint16_t)p);
        if (!getport(m, (uint16_t)p)) { notset = p; break; }
    }
    CHECK(collided < 0, "port %d was already set before it was added (two ports share a bit)", collided);
    CHECK(notset < 0, "port %d reads as clear right after setport", notset);

    int full = 1;
    for (int i = 0; i < ELEMS; i++)
        if (m[i] != 0xFFFF)
            full = 0;
    CHECK(full, "after setting all 65536 ports some bits are still clear (bitmap too small?)");
    free(m);
}

/* setting one port must set exactly that port */
static void test_single_port_isolation(void)
{
    static const int ports[] = {0, 1, 7, 8, 15, 16, 17, 31, 32, 33, 255, 256,
                                4095, 4096, 32767, 32768, 65534, 65535};
    for (size_t i = 0; i < sizeof ports / sizeof *ports; i++) {
        bitmap *m = new_set();
        setport(m, (uint16_t)ports[i]);
        CHECK(exactly(m, ports[i], ports[i]), "setport(%d) did not set exactly port %d", ports[i], ports[i]);
        free(m);
    }
}

/* clearing a port must clear that port and nothing else */
static void test_unsetport(void)
{
    static const int ports[] = {0, 1, 15, 16, 17, 33, 255, 256, 32768, 65534, 65535};
    for (size_t i = 0; i < sizeof ports / sizeof *ports; i++) {
        int p = ports[i];
        bitmap *m = new_set();
        setport(m, 0);
        setport(m, 1);
        setport(m, 2);
        setport(m, (uint16_t)p);
        if (p > 0) setport(m, (uint16_t)(p - 1));
        if (p < LAST_PORT) setport(m, (uint16_t)(p + 1));
        int before = count_set(m);

        unsetport(m, (uint16_t)p);

        CHECK(!getport(m, (uint16_t)p), "unsetport(%d) left the port set", p);
        CHECK(count_set(m) == before - 1, "unsetport(%d) changed %d ports instead of 1", p, before - count_set(m));
        if (p > 0)       CHECK(getport(m, (uint16_t)(p - 1)), "unsetport(%d) also cleared port %d", p, p - 1);
        if (p < LAST_PORT) CHECK(getport(m, (uint16_t)(p + 1)), "unsetport(%d) also cleared port %d", p, p + 1);
        free(m);
    }
}

static void test_set_ran(void)
{
    struct { int lo, hi; } r[] = {
        {5, 5}, {10, 20}, {15, 16}, {16, 31}, {0, 15}, {1, 65535}, {0, 65535},
        {65535, 65535}, {65530, 65535}, {32760, 32775}, {4090, 4100},
    };
    for (size_t i = 0; i < sizeof r / sizeof *r; i++) {
        bitmap *m = new_set();
        set_ran(m, (uint16_t)r[i].lo, (uint16_t)r[i].hi);
        CHECK(exactly(m, r[i].lo, r[i].hi), "set_ran(%d, %d) did not set exactly that range", r[i].lo, r[i].hi);
        free(m);
    }

    /* overlapping and repeated ranges merge into one set */
    bitmap *m = new_set();
    set_ran(m, 20, 30);
    set_ran(m, 25, 40);
    set_ran(m, 25, 40);
    CHECK(exactly(m, 20, 40), "overlapping ranges 20-30 and 25-40 are not the set 20-40");
    free(m);
}

static void test_unset_ran(void)
{
    struct { int lo, hi; } r[] = {
        {100, 200}, {0, 15}, {16, 31}, {65530, 65535}, {65535, 65535}, {0, 65535},
    };
    for (size_t i = 0; i < sizeof r / sizeof *r; i++) {
        bitmap *m = new_set();
        set_ran(m, 0, 65535);
        unset_ran(m, (uint16_t)r[i].lo, (uint16_t)r[i].hi);
        int ok = 1;
        for (int p = 0; p <= LAST_PORT; p++) {
            int want = !(p >= r[i].lo && p <= r[i].hi);
            if ((int)getport(m, (uint16_t)p) != want) { ok = 0; break; }
        }
        CHECK(ok, "unset_ran(%d, %d) did not clear exactly that range", r[i].lo, r[i].hi);
        free(m);
    }
}

#define RUN(t)                          \
    do {                                \
        printf("running %s\n", #t);      \
        fflush(stdout);                 \
        t();                            \
    } while (0)

int main(void)
{
    /* the names are printed first, so a test that never ends is easy to spot */
    RUN(test_empty_set);
    RUN(test_every_port_has_its_own_bit);
    RUN(test_single_port_isolation);
    RUN(test_unsetport);
    RUN(test_set_ran);
    RUN(test_unset_ran);

    printf("%d checks, %d failed\n", checks, failures);
    return failures != 0;
}
