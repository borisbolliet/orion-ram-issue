# mpstress — a reproducer for the orion memory fault

A multiprocess CPU + memory stress test with a hardware-error watchdog. It was written to
answer one question: can the spontaneous hard resets on **orion** be triggered on demand?

They can. **Two runs, two resets, in 4 m 40 s and 9 m 56 s**, each leaving the same fatal
command/address-bus parity error in the firmware log and each permanently costing a DIMM.

> **This test hard-resets the machine.** It is not a benchmark. Everything unsaved is lost,
> every other user's session dies without warning, and on orion each reset has so far mapped
> out one more memory module permanently. Do not run it on a machine anyone depends on, and
> warn other users first.

## What it does

Each stage forks *N* worker processes. Every worker owns a private buffer and alternates:

1. **write + verify** over its whole buffer with an address-dependent pattern
   (`splitmix64(i, seed)`), so a flipped bit, a stuck cell or a misdirected write is caught
   and reported with its offset and both values;
2. an **FMA-heavy compute burst** — 16 independent dependency chains, ~200 M fused
   multiply-adds — to keep the cores and the uncore busy between memory passes.

Workers publish progress through a `MAP_SHARED` counter block. The parent samples once per
second and appends one **`fsync`'d** line per sample, so the final line survives a hard
reset — which is the whole point, and which is how the time-to-failure below was measured.

### The watchdog

The parent aborts the entire run on the first sign of trouble, so that a *detected* error
stops the test rather than being buried by continued hammering:

| trigger | meaning |
|---|---|
| EDAC `ce_count` rises | a new corrected ECC error |
| EDAC `ue_count` rises | a new uncorrected ECC error |
| any worker `bad_words > 0` | silent data corruption — written value did not read back |
| `Tctl` > 95 °C | thermal limit, abort before the hardware throttles or trips |
| `MemAvailable` < 150 GB | about to swap; the result would be meaningless |

On orion **none of these ever fired.** The machine resets with EDAC counters at zero, no
corrected-error precursor, and no data corruption, so the watchdog is left holding nothing.
That null result is itself evidence: whatever kills the machine outruns every error-reporting
path the OS has.

## Build

```sh
gcc -O2 -march=native -o mpstress mpstress.c -lm
```

No dependencies beyond libm. Reads `/proc/meminfo`, `/proc/loadavg`,
`/sys/devices/system/edac/mc/*/[cu]e_count` and the `k10temp` hwmon node, so EDAC and
`k10temp` want to be loaded for the watchdog to have anything to watch.

## Run

```
mpstress <log> <mb_per_worker> <secs_per_stage> <n1> [n2 ...]
```

Each `n` is a worker count, run as its own stage for `secs_per_stage`. Detach it, because it
must outlive your shell:

```sh
setsid nohup nice -n 10 ./mpstress ~/orion-stress/runs/ramp.log 512 1800 24 48 96 \
  >/dev/null 2>&1 </dev/null &
```

**Keep the log on persistent disk.** The first run on orion was lost because its log sat on a
path that did not survive the reboot, and the whole run had to be reconstructed from an
unrelated sampler. `run_stress.sh` exists to stop that happening again: it captures a
pre-run state snapshot (DIMM census with serials and configured speed, BIOS version, EDAC
counters, BERT table size, Tctl, GPU draw) alongside the log, then launches detached.

```sh
./run_stress.sh <tag> <mb_per_worker> <secs_per_stage> <n1> [n2 ...]
# writes runs/<tag>.log, runs/<tag>.state, runs/<tag>.{out,err}
```

### Choosing stage length

Make it long. The first orion run used 240 s stages and that produced a false result: stage 1
appeared to survive 232 s at 24 workers, so the stage-2 death 38 s later looked like it was
caused by doubling the worker count. Rerunning stage 1 with a 1800 s stage killed the machine
at **596 s** — the first stage had not survived, it had simply been cut short before it
finished dying, and the concurrency reading was wrong. A stage that ends before the failure
does is worse than no data.

## Log format

A header line, a column line, then one sample per second:

```
# start epoch=1793362443 ce0=0 ue0=0 memavail=356.0GB tctl=67.5
# epoch stage workers t load1 tctl_C memavail_GB ce ue mem_GBps gflops bad_words passes
1793363039 1 24 596 26.4 69.5 343.3 0 0 79.47 29.6 0 44693
```

`ce`/`ue` are deltas since start. `mem_GBps` counts only bytes written **and** verified, so
it is roughly half the figure an external bandwidth counter reports for the same work.
`t` is seconds into the current stage. A run that ends without a `# end` line means the
machine died at that timestamp.

## Results on orion

BIOS 1317, memory at Auto/default (6400 MT/s), both GPUs idle, 2026-10-30:

| run | workers | mem/worker | DRAM traffic | per channel | Tctl max | outcome |
|---|---|---|---|---|---|---|
| 11:12 stage 1 | 24 | 512 MB | ~160 GB/s | 20.1 GB/s | 66 °C | survived 232 s — **stage timer advanced it** |
| 11:16 stage 2 | 48 | 512 MB | ~163 GB/s | 20.4 GB/s | 63 °C | **reset at 38 s** (4 m 40 s cumulative) |
| 12:14 stage 1 | 24 | 512 MB | ~126 GB/s | 20.9 GB/s | 71 °C | **reset at 596 s** |

Both resets produced an ACPI BERT record containing
`ErrorStatus 0x0000000000011600`, `ErrorType 8 (Parity error)` — a DDR5 command/address-bus
parity failure, with no reportable physical address because the address itself arrived
corrupted. Identical bit-for-bit to records from 17 September and 13 October taken on the
previous BIOS.

What the runs establish, and what they do not:

- **Reproducible on demand**, in under ten minutes, from a cold documented state.
- **Thermal, GPU and PSU excluded.** Tctl peaked 24 °C below its limit, both GPUs idle at
  ~17 W, and 341 GB of 377 GB stayed free. The load is light for the machine's
  specification: 24 processes, 12 GB resident, CPU ~13 % busy, and DRAM traffic well under
  half the rated bandwidth of the memory configuration.
- **Not a dose-response curve.** 232 s survived at 20.1 GB/s/channel and 596 s died at
  20.9 is two points, and the gap between the two runs' total time-to-failure
  (4 m 40 s vs 9 m 56 s) is unexplained. Worker count being irrelevant is *suggested*, not
  shown — 24 is enough, but 24 and 48 are the only values tried.
- **No silent corruption.** 44,693 full write+verify passes, zero mismatches. The machine is
  killed outright rather than returning wrong answers.

Full analysis, the firmware records and the rest of the case are in [`../NOTES.md`](../NOTES.md).

## Files

| file | what it is |
|---|---|
| `mpstress.c` | the test |
| `run_stress.sh` | pre-run state snapshot + detached launch |
| `ramp-20261030-1215.log` | the 589-sample log of the 596 s run, last line intact through the reset |
| `ramp-20261030-1215.state` | machine state captured immediately before that run |
