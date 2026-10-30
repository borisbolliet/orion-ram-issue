/*
 * mpstress: multiprocess CPU + memory stress test with hardware-error watchdog.
 *
 * Forks N worker processes per stage. Each worker owns a private buffer and
 * alternates an FMA-heavy compute burst with a write+verify pass over its
 * buffer (address-dependent pattern, so a flipped bit or a misdirected write
 * is detected). Workers report progress through a MAP_SHARED counter block.
 *
 * The parent samples load, Tctl, MemAvailable, EDAC CE/UE counts and worker
 * verify errors once per second, appends one fsync'd line to the log (so the
 * last line survives a hard crash), and aborts the whole run on the first
 * new hardware error, verify mismatch, Tctl over limit or low MemAvailable.
 *
 * Keep the log on persistent disk (not /tmp, which is wiped on reboot).
 *
 * build: gcc -O2 -march=native -o mpstress mpstress.c -lm
 * usage: mpstress <log> <mb_per_worker> <secs_per_stage> <n1> [n2 ...]
 *   e.g. setsid nohup nice -n 10 ./mpstress ~/orion-stress/ramp.log 512 240 24 48 96 192 &
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <glob.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_WORKERS 512
#define TCTL_LIMIT_C 95.0
#define MEMAVAIL_FLOOR_GB 150.0

struct counters {
    volatile uint64_t passes;      /* full write+verify passes over buffer */
    volatile uint64_t bytes;       /* bytes written + verified */
    volatile uint64_t flops;       /* FMA ops (2 flops each) */
    volatile uint64_t bad_words;   /* verify mismatches */
    volatile int64_t first_bad_off;
    volatile uint64_t first_bad_got, first_bad_want;
};

static struct counters *ctr;
static volatile sig_atomic_t stop_all;

static void on_sig(int s) { (void)s; stop_all = 1; }

static inline uint64_t pattern(uint64_t i, uint64_t seed)
{
    uint64_t x = i * 0x9E3779B97F4A7C15ull ^ seed;
    x ^= x >> 31; x *= 0xBF58476D1CE4E5B9ull; x ^= x >> 29;
    return x;
}

static void worker(int id, size_t mb)
{
    size_t n = mb * 1024 * 1024 / sizeof(uint64_t);
    uint64_t *buf = mmap(NULL, n * sizeof(uint64_t), PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) _exit(2);
    struct counters *c = &ctr[id];
    double a[16], b = 1.0000001, d = 0.9999999;
    for (int k = 0; k < 16; k++) a[k] = 1.0 + k * 1e-3;

    for (uint64_t seed = (uint64_t)id << 40;; seed++) {
        for (size_t i = 0; i < n; i++) buf[i] = pattern(i, seed);
        for (size_t i = 0; i < n; i++) {
            uint64_t want = pattern(i, seed);
            if (buf[i] != want) {
                if (c->bad_words == 0) {
                    c->first_bad_off = (int64_t)i;
                    c->first_bad_got = buf[i];
                    c->first_bad_want = want;
                }
                c->bad_words++;
            }
        }
        c->bytes += 2 * n * sizeof(uint64_t);
        c->passes++;

        /* compute burst: 16 independent FMA chains, ~200M FMAs */
        for (int r = 0; r < 12500000; r++)
            for (int k = 0; k < 16; k++) a[k] = fma(a[k], b, d) - 1.0 + 1e-9;
        c->flops += 2ull * 16 * 12500000;
        if (a[0] == 12345.678) { ssize_t r_ = write(2, "", 0); (void)r_; } /* keep result live */
    }
}

static double read_num(const char *path)
{
    FILE *f = fopen(path, "r");
    double v = NAN;
    if (f) { if (fscanf(f, "%lf", &v) != 1) v = NAN; fclose(f); }
    return v;
}

static double edac_sum(const char *name)
{
    char pat[128];
    snprintf(pat, sizeof pat, "/sys/devices/system/edac/mc/mc*/%s", name);
    glob_t g;
    double s = 0;
    if (glob(pat, 0, NULL, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; i++) s += read_num(g.gl_pathv[i]);
        globfree(&g);
    }
    return s;
}

static double memavail_gb(void)
{
    FILE *f = fopen("/proc/meminfo", "r");
    char line[256];
    double kb = NAN;
    while (f && fgets(line, sizeof line, f))
        if (sscanf(line, "MemAvailable: %lf kB", &kb) == 1) break;
    if (f) fclose(f);
    return kb / 1048576.0;
}

static double load1(void) { return read_num("/proc/loadavg"); }

static double tctl_c(void)
{
    glob_t g;
    double t = NAN;
    if (glob("/sys/class/hwmon/hwmon*/name", 0, NULL, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; i++) {
            char nm[64] = {0};
            FILE *f = fopen(g.gl_pathv[i], "r");
            if (f) { if (!fgets(nm, sizeof nm, f)) nm[0] = 0; fclose(f); }
            if (strncmp(nm, "k10temp", 7) == 0) {
                char p[256];
                snprintf(p, sizeof p, "%.*s/temp1_input",
                         (int)(strrchr(g.gl_pathv[i], '/') - g.gl_pathv[i]), g.gl_pathv[i]);
                t = read_num(p) / 1000.0;
            }
        }
        globfree(&g);
    }
    return t;
}

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void logw(int fd, const char *s, int len, int sync)
{
    ssize_t r = write(fd, s, len);
    (void)r;
    if (sync) fsync(fd);
}

int main(int argc, char **argv)
{
    if (argc < 5) {
        fprintf(stderr, "usage: %s <log> <mb_per_worker> <secs_per_stage> <n1> [n2 ...]\n", argv[0]);
        return 1;
    }
    int logfd = open(argv[1], O_WRONLY | O_CREAT | O_APPEND, 0644);
    size_t mb = strtoul(argv[2], NULL, 10);
    int secs = atoi(argv[3]);
    ctr = mmap(NULL, sizeof(struct counters) * MAX_WORKERS, PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    signal(SIGINT, on_sig);
    signal(SIGTERM, on_sig);

    double ce0 = edac_sum("ce_count"), ue0 = edac_sum("ue_count");
    char line[512];
    int len = snprintf(line, sizeof line,
        "# start epoch=%ld ce0=%.0f ue0=%.0f memavail=%.1fGB tctl=%.1f\n"
        "# epoch stage workers t load1 tctl_C memavail_GB ce ue mem_GBps gflops bad_words passes\n",
        (long)time(NULL), ce0, ue0, memavail_gb(), tctl_c());
    logw(logfd, line, len, 1);

    const char *abort_reason = NULL;
    for (int st = 4; st < argc && !abort_reason && !stop_all; st++) {
        int nw = atoi(argv[st]);
        if (nw > MAX_WORKERS) nw = MAX_WORKERS;
        memset((void *)ctr, 0, sizeof(struct counters) * MAX_WORKERS);
        pid_t pids[MAX_WORKERS];
        for (int i = 0; i < nw; i++) {
            pids[i] = fork();
            if (pids[i] == 0) { signal(SIGTERM, SIG_DFL); worker(i, mb); }
        }
        double t0 = now(), last_t = t0;
        uint64_t last_bytes = 0, last_flops = 0;
        while (!abort_reason && !stop_all) {
            sleep(1);
            double t = now(), dt = t - last_t;
            uint64_t bytes = 0, flops = 0, bad = 0, passes = 0;
            for (int i = 0; i < nw; i++) {
                bytes += ctr[i].bytes; flops += ctr[i].flops;
                bad += ctr[i].bad_words; passes += ctr[i].passes;
            }
            double ce = edac_sum("ce_count") - ce0, ue = edac_sum("ue_count") - ue0;
            double ma = memavail_gb(), tc = tctl_c();
            len = snprintf(line, sizeof line, "%ld %d %d %.0f %.1f %.1f %.1f %.0f %.0f %.2f %.1f %lu %lu\n",
                (long)time(NULL), st - 3, nw, t - t0, load1(), tc, ma, ce, ue,
                (bytes - last_bytes) / dt / 1e9, (flops - last_flops) / dt / 1e9,
                (unsigned long)bad, (unsigned long)passes);
            logw(logfd, line, len, 1);
            last_t = t; last_bytes = bytes; last_flops = flops;

            if (ce > 0) abort_reason = "new corrected ECC error (EDAC ce_count rose)";
            else if (ue > 0) abort_reason = "new uncorrected ECC error";
            else if (bad > 0) abort_reason = "worker verify mismatch (silent data corruption)";
            else if (tc > TCTL_LIMIT_C) abort_reason = "Tctl over limit";
            else if (ma < MEMAVAIL_FLOOR_GB) abort_reason = "MemAvailable below floor";
            else if (t - t0 >= secs) break;
        }
        for (int i = 0; i < nw; i++) kill(pids[i], SIGKILL);
        for (int i = 0; i < nw; i++) {
            int status;
            waitpid(pids[i], &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 2 && !abort_reason)
                abort_reason = "worker mmap failed";
        }
        for (int i = 0; i < nw; i++)
            if (ctr[i].bad_words) {
                len = snprintf(line, sizeof line,
                    "# worker %d bad_words=%lu first_off=%ld got=%016lx want=%016lx\n",
                    i, (unsigned long)ctr[i].bad_words, (long)ctr[i].first_bad_off,
                    (unsigned long)ctr[i].first_bad_got, (unsigned long)ctr[i].first_bad_want);
                logw(logfd, line, len, 0);
            }
    }
    len = snprintf(line, sizeof line, "# end epoch=%ld result=%s\n", (long)time(NULL),
                   abort_reason ? abort_reason : (stop_all ? "interrupted" : "completed"));
    logw(logfd, line, len, 1);
    return abort_reason ? 3 : 0;
}
