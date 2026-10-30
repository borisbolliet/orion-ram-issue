# orion RAM issue — PCSpecialist warranty case

**Machine:** orion (PCSpecialist workstation) — sold with 512 GB RAM, 28 TB storage, 2× RTX-6000 Pro GPU.
**CPU / board:** AMD Ryzen Threadripper PRO 9995WX (96-core) on ASUS WRX90-class board (AMI Aptio). BIOS 1203, 18/07/2025, AGESA ComboSP6PI 1.0.0.1a Patch A.
**RAM:** 8× 64 GB Kingston DDR5 ECC RDIMM, P/N 9965844-002.A01G, rated 6400 MT/s, one per channel (8 channels A–H).
**Support ticket:** 3186088 — "only ~377 GiB RAM" [2b07a0]
**Support contact:** Renato Gonzales, PCS Time de Suporte — enquiries@pcspecialist.co.uk / 0333 011 7000
**Diagnosed:** 2026-06-08 · **Updated:** 2026-09-19
**STATUS (2026-09-19): NO REPLY FROM PCS since the 2026-06-15 message — 3 months.** Meanwhile the fault has escalated from "wrong RAM total" to **spontaneous crashes with a firmware-logged FATAL memory error**. Escalation letter drafted in `reply-draft-2.txt` (NOT yet sent) with new evidence: `bert-decoded-20260917.txt`, `bert-record-20260917.bin`, `orion-ram-diag-20260919-160836.txt`. See the 2026-09-19 section at the bottom.

## The problem in one line

The machine **intermittently fails to detect/train its RAM**: the total swings between boots — 512 GB, 256 GB, and "~377 GiB" have all been observed. All eight modules are physically present; on some boots the firmware trains only four of them.

## Timeline of observed totals

| When | Source | Reported total | Notes |
|------|--------|----------------|-------|
| (ticket subject) | OS | ~377 GiB | original report |
| 2026-06-08, earlier boot | Linux `dmidecode -t 17` | 256 GB | only channels A, D, E, H populated; B, C, F, G "No Module Installed" — reproducible within that boot |
| 2026-06-08, after restart | BIOS Main page (photo) | **512 GB @ 6400 MHz** | all 8 channels; full speed |
| 2026-06-15, current boot | Linux `MemTotal` | **377 GiB** (395301064 kB) | dmidecode shows 6 modules (B,F missing); EDAC shows 7 channels; spontaneous reboot Jun 13 |
| 2026-06-16, BIOS (photo `bios-384gb-20260616.jpg`) | BIOS Main page | **384 GB @ 6400 MHz** | = 6 of 8 modules trained; 4th distinct total observed (512/256/377/384) |

→ The count **changes across reboots** = memory-training instability / marginal contact, not absent modules.

### KEY FINDING — the failing channels differ each boot

| Boot | DIMMs trained | Missing |
|------|---------------|---------|
| 256 GB boot | A, D, E, H | **B, C, F, G** |
| 377 GiB boot (2026-06-15) | A, C, D, E, G, H | **B, F** |
| 512 GB boot | all 8 | none |

Different channels drop on different boots. A permanently dead module would drop the **same** slot every time — these don't. Non-deterministic channel failure points to **memory-training instability / marginal electrical contact**, and on AMD Threadripper specifically a likely **CPU socket-seating / LGA-pin issue** (every channel routes through the CPU socket pins) and/or marginal training of 8× 64 GB at 6400 MT/s — NOT a single bad stick.

### Crash evidence (2026-06-15 diag, file `orion-ram-diag-20260615-103912.txt`)

- **`BERT: [Hardware Error]: Skipped 1 error records`** at boot — firmware logged an ACPI hardware-error record from the crash (firmware-level, not software).
- **Spontaneous double-reset Jun 13**: up ~5 days, went down ~20:23, came back, ran only **8 minutes** (20:28→20:36), reset again, up since 20:40.
- **No active ECC errors this boot** (ce_count=0, ue_count=0) — the fault manifests as failed training at boot + occasional hard reset, consistent with marginal contact rather than a steadily-dying chip.
- SMBIOS (6 modules) vs EDAC chip-select map (7 channels) vs MemTotal (377 GiB) disagree — three sources at odds = unstable training.

### Recommended ask to PCS (hands-on hardware service, push for engineer/RMA)

1. Reseat the CPU and inspect the socket for bent/contaminated LGA pins.
2. Reseat all 8 DIMMs (and/or swap modules between slots to see if the fault follows a stick or a slot).
3. Update BIOS/AGESA (current AGESA 1.0.0.1a is early for Zen5 TR PRO).
4. Stress-test memory across **repeated power-cycles** (training stability), not just one memtest pass.

## Evidence in this folder

- `PXL_20260608_164213760.jpg` — BIOS Main page: Total Memory 512 GB (DDR5) @ 6400 MHz; CPU = TR PRO 9995WX; BIOS 1203.
- `PXL_20260608_164246051.jpg` — BIOS Advanced AMD per-socket memory timing/config submenu (context, not DIMM inventory).
- `dmidecode-t16.txt` — Physical Memory Array: 8 devices, 4 TB max.
- `dmidecode-t17.txt` — per-DIMM inventory from the 256 GB boot (A,D,E,H = 64 GB; B,C,F,G empty).
- `email-thread.txt` — correspondence with PCS.
- `reply-draft.txt` — drafted response to Renato (pending send).

## Why this is PCS's responsibility, not a Linux issue

The varying total is reported at the **firmware/POST level** (BIOS Total Memory and the SMBIOS table populated by BIOS), before and independent of the OS. Linux cannot cause a DIMM to be untrained at POST. The fault is hardware/firmware: marginal seating, a flaky module or slot, or unstable memory training of 8× 64 GB RDIMM at 6400 MT/s.

## Next step requested from PCS

Reliable operation at the full 512 GB. Likely remedies (PCS to perform/advise under warranty):
- reseat all 8 DIMMs and retest with repeated power-cycles + a memory stress test (e.g. memtest),
- identify/replace any marginal module or slot,
- consider a BIOS/AGESA update (current AGESA 1.0.0.1a is early for Zen5 TR PRO; newer AGESA often improves memory training).

## Linux readback to capture on the current (512 GB) boot — TODO

```
grep MemTotal /proc/meminfo
sudo dmidecode -t 17 | grep -E "Bank Locator:|Size:|Speed:"
```
Confirms whether the OS now also sees all 8 channels / ~512 GB on this boot.

---

## 2026-09-19 — ESCALATION: fatal memory error caught by the firmware

The fault is no longer just "the RAM total is wrong". The machine now crashes,
and on the last crash the firmware recorded an unambiguous **FATAL** hardware
error. This is the strongest evidence in the case so far.

### The crashes

| When | Behaviour | Firmware error record? |
|------|-----------|------------------------|
| 2026-09-10 ~12:04 | Log stops dead mid-entry; machine stayed down until switched on by hand 2026-09-11 13:45 (~25 h) | **No** BERT record |
| 2026-09-17 21:06:26 | Log stops dead mid-entry (a routine JupyterHub request); self-recovered at 21:10:36 | **Yes — 1 record, FATAL** |

Neither crash left a panic, oops, MCE in the OS log, thermal event, or a shutdown
sequence; `/sys/fs/pstore` is empty. `last -x` shows almost every boot since
2026-08-02 as unclean (only one real `shutdown` record).

### The BERT record (file: `bert-decoded-20260917.txt`, raw: `bert-record-20260917.bin`)

Read from `/sys/firmware/acpi/tables/data/BERT`, 92,292 bytes. Present on the boot
*after* the crash, absent on the previous boot ⇒ firmware generated it for this event.
**168 error data entries; 71 FATAL** (70 CPU + 1 memory).

The substantive entry:

```
Section type : Platform Memory Error
Severity     : FATAL
Error type   : 8  (Parity error)
Location     : Node 0, Card 2, Module 0        (FRU "Perr S0:T002:B15")
MCA bank     : 0x15 (21)
MCA_IPID     : 0x0000009600250f00   -> HWID 0x96 = Unified Memory Controller
MCA_STATUS   : 0xfea000000004080b   -> Val|Overflow|UC|En|MiscV|AddrV|PCC
```

**`PCC` = Processor Context Corrupt ⇒ unrecoverable machine check ⇒ immediate
hardware reset.** That is exactly why the OS log stops mid-entry with nothing after it.

**This rules out a power cut / mains dip** as the cause of the 17 Sep reset: a loss of
power cannot write an MCA status register or populate an ACPI error record. (Worth
knowing: orion has **no UPS at all** — no USB HID UPS, no nut/apcupsd — so a mains
event would leave no trace. The 10 Sep crash, with no BERT record and a 25 h downtime,
is still unexplained and could be one.)

### Why it is NOT a bad DIMM — three independent arguments

1. **Parity, not multi-bit ECC.** Parity is checked on the command/address path.
   Dying cells give ECC errors; a corrupted command/address path gives parity errors.
   This is a **signal-integrity fault on the memory interface** — the same physical
   failure mode as the June training failures, finally caught in the act.
2. **Six memory-controller instances erred in the same instant** (CPER Card 0, 2, 3, 5,
   8, 9 — six of the eight channels populated at the time). No single module reaches
   six controllers. **Common-mode fault.**
3. **The modules are demonstrably good.** Boris (2026-09-19): *"the RAM bars are
   actually ok, when I reboot by hand by pressing button, in general I get back all
   512g (503)."* Confirmed in the logs — a **cold** boot trains all 8 channels
   (536,189,632 kB ≈ 503 GiB usable); the degraded 6/8 (401,972,052 kB ≈ 376 GiB) only
   follows a **warm self-reset**. So the reduced total is a *symptom of the crash*, not
   an independent fault, and the causal arrow is NOT "bad sticks → crash".

### NEW: correctable ECC errors (June: zero)

`rasdaemon` installed + enabled 2026-09-19 (it was **not** installed before; it is now
`enabled`/`active` and will log per-DIMM with timestamps going forward). It immediately
localised the errors that raw dmesg could not:

```
mc#0 csrow#0 channel#5 :  1 CE
mc#0 csrow#1 channel#5 :  5 CE
(every other channel: 0)          ce_count=6, ue_count=0
```

All six correctable errors are on **one channel, both ranks** — but at six *unrelated*
physical addresses (0x82c50580, 0x1593ce280, 0x181914d00, 0x1e939c880, 0xff0094280,
0x73edf100), which is a bus/margin signature, not a failing cell (a bad cell repeats at
one address). Note the fatal parity error was on a **different** controller instance
(Card 2) from the one now producing CEs (channel 5).

**Verified mapping:** the EDAC populated channel set (0, 2, 3, 5, 8, 9) is identical to
the CPER "Card" indices in the BERT record ⇒ **CPER Card == EDAC channel == UMC
instance.** The EDAC-channel → physical slot letter (A–H) mapping is still unknown;
worth establishing on the next 8/8 boot.

### Configuration detail that matters

All 8 modules are **dual-rank** (`Rank: 2`) 64 GiB RDIMMs at **6400 MT/s, 1.1 V**, all
channels populated. This is *nominally in spec* (AMD rates TR PRO 9000 WX / WRX90 up to
DDR5-6400 8-channel; the Kingston modules are rated 6400) but it is the most demanding
validated memory configuration on sTR5 — where marginal socket contact, launch-vintage
AGESA, or board tolerance shows up first.

### Firmware is launch-vintage

Still **BIOS 1203, 18/07/2025, AGESA ComboSP6PI 1.0.0.1a** — the launch firmware for
this CPU generation. ASUS released **BIOS 1317 on 13/02/2026**, AGESA **ShimadaPeak PI
1.0.0.1e_012**, "improved system stability and device compatibility". Four AGESA
revisions of memory-training work not applied. (Caveat: 1317 has a reported erratum
where tREFI resets on POST — appears to affect only users hand-tuning tREFI; JEDEC/default
users reported unaffected.)

### REVISED ask to PCS — do NOT ask for a DIMM swap

Supersedes the June list. In `reply-draft-2.txt`:

1. **CPU socket service** — reseat the processor, inspect sTR5 socket for bent /
   contaminated / poorly-tensioned LGA pins. All channels route through the socket, and
   a common-mode fault across six memory controllers is what marginal socket contact
   looks like.
2. **BIOS/AGESA update** off launch 1.0.0.1a to 1317 / 1.0.0.1e_012.
3. **Validation in the configuration as sold** — 8× dual-rank 64 GB at 6400 MT/s under
   **sustained load** and repeated power-cycles, not a POST check or one memtest pass.
4. Motherboard slot/trace inspection if (1) doesn't resolve it.
5. If their position is that 8× dual-rank at 6400 can't be run reliably on this board,
   **get that in writing** — the machine was sold and invoiced on that spec.

### Planned self-help at the machine (order matters — one variable at a time)

1. Flash **BIOS 1317** via **USB BIOS FlashBack** (safest path; works with no CPU/RAM
   and is the recovery route if a flash fails — do NOT use EZ Flash on a machine that
   spontaneously resets).
2. Boot; confirm 8/8 at 6400. **Expect 5–10 min of blank screen** while 512 GiB
   retrains — do not power-cycle mid-training.
3. Set `Advanced → AMD CBS → UMC Common Options → DDR Memory Features →
   Memory Context Restore = [Disabled]`. Forces a full clean retrain on every boot;
   should stop the machine coming back at 376 GiB after a crash. Cost: slow warm reboots.
   (Treats the symptom, not the fault.)
4. **Only if it faults again**, derate memory: `Ai Tweaker → Ai Overclock Tuner =
   [Manual]` (required — Memory Frequency is hidden while it is [Auto]) → `Memory
   Frequency = [DDR5-4800MHz]`. Alternative AGESA path: `Advanced → AMD Overclocking →
   DDR and Infinity Fabric Frequency/Timings → DDR Options → DDR Timing Configuration →
   Active Memory Timing Settings [Enabled] → Memory Target Speed = 4800`.
   Quiet at 4800 ⇒ bus-margin problem; still erroring ⇒ hard defect.
   **Don't let a successful derate become PCS's exit** — the claim is that the machine
   is unstable *in its shipped configuration*.

Instructions emailed to Boris 2026-09-19 (2 messages, one thread). BIOS paths verified
against the ASUS Pro WS sTR5 BIOS Manual E22761, §6 (Ai Tweaker), §7.18 (AMD
Overclocking), §7.19 (AMD CBS), §12 (Exit).

### Also found (unrelated to the RAM, but broken)

`apt` on orion is in a **broken state**: `linux-image-7.0.0-31-generic` fails its
post-install because the NVIDIA DKMS module won't build against kernel 7.0.0-31
(`common/inc/nv-time.h:85:9: error: implicit declaration of function 'in_irq'`). Every
`apt` run now ends in an error. Packages still install, but the kernel packages stay
unconfigured. Running kernel is 6.14.0-33, so GPUs work today — **but a reboot that
selects 7.0.0-31 will come up with no NVIDIA driver.** Needs deciding separately:
newer driver, or remove the 7.0 HWE kernel.

### New evidence files

- `bert-decoded-20260917.txt` — full CPER decode of the crash record + entry census.
- `bert-record-20260917.bin` — raw 92,292-byte ACPI BERT record from the firmware.
- `bert_decode.py`, `mem_decode.py` — the CPER/Platform-Memory decoders used.
- `orion-ram-diag-20260919-160836.txt` — current full diagnostic (diagnose.sh run).
- `reply-draft-2.txt` — escalation letter to PCS, **drafted, not sent**.

### 2026-09-19 23:04 — firmware names ALL FOUR failed slots (kernel-decoded)

After a **clean** `shutdown -r` (not a crash), the boot at 23:04:27 published a BERT record
that **the kernel decoded and printed itself** — no custom tooling needed. File:
`bert-mapout-20260919-2304.txt`, raw: `bert-record-20260919-2304.bin`.

```
event severity: fatal
Error 0:  fru_text: Locator: P0MC0   card:0   error_type: 15, physical memory map-out event
Error 1:  fru_text: Locator: P0MB0   card:5   error_type: 15, physical memory map-out event
Error 2:  fru_text: Locator: P0MG0   card:6   error_type: 15, physical memory map-out event
Error 3:  fru_text: Locator: P0MF0   card:11  error_type: 15, physical memory map-out event
error_status: Storage error in DRAM memory (0x0000000000040400)
```

**The firmware names slots C, B, G and F** — exactly the four `dmidecode` reports as
"No Module Installed". This is the single best artefact in the case: ASUS's own firmware,
decoded by the stock kernel, stating it has failed and disabled four named DIMM slots.
**No interpretation required — quote it verbatim to PCS.**

Because it appeared after a *clean* reboot, these map-out events are logged during **this
boot's POST training**, not carried over from a crash.

**Complete CPER Card → physical slot mapping (from firmware):**

| Card | Slot |
|------|------|
| 0    | C    |
| 5    | B    |
| 6    | G    |
| 11   | F    |

Self-consistent with the earlier observation that the populated Card set was {0,2,3,5,8,9}
when A,B,C,D,E,H were in ⇒ Cards 2,3,8,9 = A,D,E,H in some order.

**This re-attributes the correctable errors.** The 6 CEs of 17–19 Sep were on EDAC
"channel 5". If EDAC channel numbering matches CPER card numbering (supported: the
populated sets were identical), **channel 5 = DIMM B** — and B has now been mapped out as
failed. So the DIMM that was throwing correctable errors has progressed to outright
failure. Coherent degradation story. (Caveat: the EDAC↔CPER numbering identity is inferred
from matching sets, not stated by firmware.)

**The "not a DIMM swap" ask still stands.** The mapped-out set still *varies by boot*
(B,C,F,G tonight and in June; F,G on 17 Sep; B,F on 15 Jun) — four permanently dead
modules cannot produce a changing set. Common-mode (socket / board / training) remains the
diagnosis. F is absent in every degraded boot ever recorded.

### Same session: NVIDIA driver fixed, apt unbroken

- Unheld the 169 blanket-held nvidia packages; upgraded **580.95.05 → 580.178.04**
  (19 upgraded, 0 removed). DKMS now built for **both** 6.14.0-33 and 7.0.0-31.
- `dpkg --configure -a` clean; `linux-image-7.0.0-31-generic`, `linux-headers-7.0.0-31-generic`,
  `linux-generic-hwe-24.04`, `linux-headers-generic-hwe-24.04` all back to `ii`. **apt works again.**
- Verified after reboot: both RTX PRO 6000 Blackwell on 580.178.04, fleet workers 1–4 active,
  rasdaemon active, ECC counters clean so far this boot.
- `GRUB_DEFAULT` is pinned to `6.14.0-33-generic`, so the box still boots 6.14; 7.0.0-31 is
  now fully built and bootable whenever wanted.
- **Still outstanding: BIOS 1203 → 1317, Memory Context Restore = Disabled, and the
  cold power-cycle that restores 503 GiB.** Warm reboot left it at 4/8 / 250 GiB as expected.

---

## 2026-09-21 — BIOS settings changed at the machine (no flash)

Boris went to the machine and changed BIOS settings. **BIOS was NOT flashed** — still
**1203 (18/07/2025)**, no USB stick with `A5496.CAP` on hand. The 1317 flash remains
outstanding.

### Baseline reading BEFORE the change — important

BIOS Main page reported **Total Memory = 512 GB** on that cold boot, at 6400. So even now,
after three crashes in ten days and with the fault progressing, **a genuine cold start
still trains all eight modules at full rated speed.** This is strong evidence for the
ticket: the modules are good; the fault is training instability, not dead DIMMs.
(Main-page photo taken for the case file, alongside `bios-512gb.jpg` and
`bios-384gb-20260616.jpg`.)

### Settings changed

| Path | Setting | Value |
|------|---------|-------|
| `Ai Tweaker` | Ai Overclock Tuner | **[Manual]** (required — Memory Frequency is hidden under [Auto]) |
| `Ai Tweaker` | BCLK1 Frequency | `100.0000` (verified stock, unchanged) |
| `Ai Tweaker` | **Memory Frequency** | **[DDR5-5600MHz]** — derated from 6400 |
| `Ai Tweaker → DRAM Timing Control` | **Memory Context Restore** | **[Disabled]** |

Note: `Memory Context Restore` was **not** where the current manual puts it
(`Advanced → AMD CBS → UMC Common Options → DDR Memory Features`) — that submenu wasn't
present on BIOS 1203. It was found via the **Ai Tweaker → DRAM Timing Control** mirror,
at the bottom of the page between `Power Down Enable` and `UCLK DIV1 MODE`. Worth
remembering: **on BIOS 1203 use the Ai Tweaker path, not the AMD CBS path.**

### What each change targets (separable symptoms — they do not confound each other)

- **5600 derate** → the *crashes*. Widens timing margin on the command/address bus, which
  is where the fatal parity error of 17 Sep occurred. Costs ~12% memory bandwidth
  (409.6 → 358.4 GB/s theoretical).
- **Memory Context Restore = Disabled** → the *degraded capacity after a reset*. Forces a
  full retrain every boot instead of restoring a saved context. Costs boot time (minutes
  with 512 GiB). Hypothesis, not proven mechanism: it explains why hand power-cycles have
  always restored 503 GiB and self-resets never have.

### What to watch

1. Does it come back with **8/8 at 5600**? If short, the map-outs aren't purely a
   speed-margin problem.
2. Does the **crash interval** extend beyond the recent ~2 days?
3. Do **correctable ECC errors** stay at 0 (`ras-mc-ctl --error-count`)? They were 6 in
   48 h at 6400.
4. Any new **`physical memory map-out event`** in BERT after a reset?

If it goes quiet at 5600 → bus-margin problem at 6400, and PCS must answer for a machine
sold at 6400. If it still crashes → margin isn't the issue and it's a hard defect.
**Either result is useful. Do not let a successful derate become PCS's exit** — the claim
remains that the machine is unstable *in its shipped configuration*.

### 2026-09-21 17:32 — RESULT of the 5600 derate: all 8 DIMMs, clean firmware log

First boot after the change. **Best state recorded since the fault began.**

```
DMI: Memory slots populated: 8/8
MemTotal: 527420272 kB  (502 GiB)
All eight channels A-H @ 5600 MT/s  (dmidecode Configured Memory Speed)
BERT: no record at all -- firmware logged nothing
ras-mc-ctl --error-count: all zero
GPUs: both RTX PRO 6000 @ 580.178.04 | fleet: 4 workers running
```

**The absent BERT record is the headline.** Every boot since 17 Sep carried one, and the
last two were explicit `physical memory map-out event` entries naming failed slots
(F,G on 19 Sep 21:11; C,B,G,F on 19 Sep 23:04). At 5600 the firmware trained all eight
channels and reported nothing. **F and G came back** after being mapped out as failed
twice in four days.

Consistent with the 17 Sep fatal **parity** error (a command/address-bus signal-integrity
failure): drop the bus speed, widen the timing margin, training succeeds.

**Still unproven: crash frequency.** This is one boot. The derate addresses training; the
open question is whether the machine still resets itself. Recent interval was ~2 days
(17th → 19th). Milestones: quiet past Wed 23 Sep = beyond the recent pattern; a full week
clean = the derate has genuinely fixed it; crashes anyway = margin wasn't the cause and
it's a harder defect. Watch `ras-mc-ctl --error-count` — CEs reappearing would be an early
warning before a crash.

**Does not change the PCS position.** The machine was sold and invoiced as 512 GB @ 6400.
It is stable at 5600 and demonstrably not at 6400. That is a defect in the shipped
configuration, and the 5600 workaround is diagnostic information, not a resolution.

---

## 2026-09-30 — core-count hypothesis REFUTED; DRAM bandwidth instrumented

Owner's hypothesis: *"when too many cores are used it crashes, although peak memory is low."*
Tested against historical telemetry (atop 19–30 Sep, sar 21–29 Sep, plus a previously
unknown tracer, below). **Refuted as stated** — but half of it is right and the residue
points somewhere better.

### CPU load at each crash (of 192 threads)

| Crash | CPU busy | loadavg1 | run queue |
|-------|----------|----------|-----------|
| 17 Sep 21:06:26 | 7.60% | 14.66 | 15 |
| 19 Sep 21:07:44 | 6.46% | 19.63 | 24 |
| 22 Sep 21:58:32 | 79.04% | 205.55 | 223 |
| 26 Sep 18:48:44 | **1.39%** | 5.74 | **2** |

**Controls that kill the hypothesis:** 27 Sep 11:42–12:16 ran **32 consecutive minutes at
100.0% busy** (all 192 threads, loadavg1 peak 5016, run queue 5830) with the same 8-DIMM
config as the 22 Sep crash boot — no crash. A `cosmosis` job then ran **~91 processes pegged
at 100% for 9.8 continuous hours** (27 Sep 23:38 → 28 Sep 09:26) — no crash. The two busiest
days of the month had no crash; 26 Sep crashed on a day peaking at 50.5%.

### What IS real

- **"Peak memory is low" CONFIRMED.** Used memory at crash: 20.4 / 39.0 / 20.5 / 10.4 %.
  23 Sep peaked at 51.2% and 25 Sep at 50.3% with no crash. Footprint is not the trigger.
- **Activity is a step, not a dial.** Poisson hazard over 170,407 samples: step at load1=10
  gives 2.40 → 52.86 resets/1000 h (22×, LRT 25.45). Adding a dose term above the step:
  β = 0.05, **p = 0.93**. No dose-response in core count.
- **Time of day is the clean discriminator.** All 13 resets fall in 10:26–23:00; **zero** in
  00:00–09:59 despite 597 h of exposure there — and that window is the busiest.
  Binomial p ≈ 8×10⁻⁴. Suggests an ambient/thermal or mains correlate of human presence.

### Two discoveries worth more than the hypothesis

**1. There have been ~13 hard resets since 30 July, not 4.** User `izubeldia` runs an
undocumented `mem_watch.service` writing `/scratch/scratch-izubeldia/evolve_sim/logs/mem_watch.log`
— 170,401 samples / 1,419 h since 2026-07-30, logging ts, mem, load1, Tctl, per-GPU temp/power.
Its gap structure exposes **seven resets in August** and one on **21 Sep 16:45** not previously
known. The fault is two months old, not two weeks. **Mine this file for the PCS case.**

**2. The firmware names the ADDRESS bus specifically.** Deeper decode of the 17 Sep record:
`ErrorStatus = 0x0000000000011600` → UEFI 2.10 App. N.2.9: bits 8–15 = 22 = `ERR_PARITY`,
**bit 16 (Address) SET**, bit 17 (Control) CLEAR, bit 18 (Data) CLEAR. Validity mask `0x4039`
carries **no** PhysicalAddress, Bank, Row, Column, Device or Rank. This is a DDR5
**command/address link** parity failure at the RCD parity latch — not stored-data corruption.

### The variable nobody could measure — now instrumented

There is **no `amd_umc` and no `amd_df` PMU** on this box, and neither atop, sar nor mem_watch
records DRAM traffic, voltage, VRM or DIMM temperature. So every "load" number in the entire
record is CPU% or loadavg — which says nothing about memory-bus stress, the thing the parity
error actually implicates.

**resctrl MBM was available all along and simply not mounted.** Silicon has
`cqm_mbm_total`/`cqm_mbm_local`/`mba`/`rdt_a`; kernel has resctrl. Mounted 2026-09-30:

```
sudo mount -t resctrl resctrl /sys/fs/resctrl     # 12 mon_L3_* domains = 12 CCDs
```

**First reading was decisive:** ~**17–22 GB/s** of DRAM traffic at **1.8–2.7% CPU busy**,
almost entirely on CCD 0 (a `tborrett` python job, 353% CPU / 13.5 GB RSS). *A near-idle
machine by every historical metric, saturating the memory bus.* The "idle" crashes may not
have been idle in the dimension that matters.

### Instrumentation installed

- `/usr/local/bin/orion-bwmon` — 2 s samples: per-CCD MBM deltas → GB/s, plus CPU%, load1,
  nproc_running, Tctl, mem used/total.
- `/etc/systemd/system/orion-bwmon.service` — enabled, `Restart=always`, `Nice=10`, idle I/O.
  `ExecStartPre` re-mounts resctrl after reboot (the mount is NOT persistent).
- `/var/log/orion-bwmon/bw-YYYYMMDD.csv` — every line `flush()` + `fsync()`, so the trace
  **survives a hard reset mid-write**. ~6.5 MB/day.
- Remove with: `sudo systemctl disable --now orion-bwmon && sudo rm /etc/systemd/system/orion-bwmon.service /usr/local/bin/orion-bwmon`

### Next step: wait, do not stress

No induced stress test is warranted. Core-count stress has already been run by accident,
harder than would have been dared (9.8 h at 91 pegged cores), and it did not crash. Resets
recur every 1–4 days, so **the next natural crash now comes with a 2-second-resolution
bandwidth trace right up to the reset.** That is the experiment, at zero risk and zero cost
to the machine's current healthy 8/8 state.

---

## 2026-10-09 — TWO FAILURE MODES. Bandwidth hypothesis also refuted.

`orion-bwmon` ran 9 days unbroken (surviving every reset and reboot — the resctrl
`ExecStartPre` remount worked). 363,337 samples; **348,876 clean after discarding 4.0%
counter artifacts** (MBM counters reset on RMID recycle; the logger's `delta()` only guarded
negative jumps. **Fixed 2026-10-09**: now also discards anything >4e11 B/s, since 8ch
DDR5-5600 peaks at ~358 GB/s). Clean distribution: p50 = 0.00, p75 = 1.0, p90 = 6.1,
p99 = 40.1, p99.9 = 154.7, max = 279.7 GB/s.

### Three resets in 41 hours — cadence accelerating

| Reset | Back up | Down for | Bandwidth (final min) | CPU | BERT | DIMMs after |
|-------|---------|----------|----------------------|-----|------|-------------|
| 2026-10-07 14:37:39 | 19:28:11 | **4 h 51 m** | 0.01 mean / 0.12 max GB/s (p67) | 0.53% | **none** | 8/8 |
| 2026-10-08 14:41:34 | 14:45:29 | 4 min | **158.75 mean / 173.31 max (p100)** | 86–100% | **yes, large** | **7/8** |
| 2026-10-09 07:33:03 | 07:37:42 | 4.6 min | 1.49 mean / 7.87 max (p93) | 0.85% | **none** | 8/8 |

**DRAM bandwidth is NOT the trigger either.** One reset at the single busiest moment of nine
days; two at ~zero memory traffic on an idle box. Combined with 2026-09-30's refutation of
core count, no workload variable predicts the fault.

### THE KEY FINDING — the resets are TWO DISTINCT FAULTS, ~50/50

- **With a BERT record:** 17 Sep, 19 Sep, 22 Sep, 8 Oct. Where load is known: 79% CPU
  (22 Sep), 100% CPU + 173 GB/s (8 Oct). MCA storm, often a DIMM map-out afterwards.
- **With NO firmware record at all:** 10 Sep, 26 Sep, 7 Oct, 9 Oct. Where load is known:
  idle both times. No DIMM loss either time.

**Mode A (loaded):** DDR5 command/address parity → machine-check storm → ~90 KB BERT record
→ DIMM map-out. This is the characterised fault; it tracks memory activity.

**Mode B (idle):** instant reset at ~0.5% CPU, ~0 GB/s, ~60 °C, **zero firmware trace**, no
DIMM loss. Recovery is erratic — 4.6 min (9 Oct) vs 4 h 51 m (7 Oct) vs ~25 h (10 Sep).

### CORRECTION to the 2026-09-20 conclusion

That entry says the BERT record "rules out a power cut." True **for the 17 Sep crash
specifically** — a mains dip cannot write an MCA status register — but it was wrongly
generalised to the whole fault. Four trace-free idle resets are now on record, and a reset at
0.5% CPU / 60 °C / zero DRAM traffic that writes nothing to firmware and loses no DIMMs is
precisely what a power-delivery event (PSU, VRM rail, or mains) looks like. **orion still has
no UPS**, so Mode B leaves no evidence anywhere by construction. The long unattended
downtimes (4 h 51 m, ~25 h) fit "did not come back until someone pressed the button."

### NEXT ACTION — buy a UPS with USB monitoring

First raised 2026-09-20; now the highest-value action available. It would (a) eliminate Mode B
outright if it is mains, and (b) give `nut` a log of every dip, which **classifies** Mode B
definitively. Half this fault is currently unobservable.

Mode A remains PCS's responsibility with overwhelming evidence. **Mode B may be a different
problem entirely, and possibly a cheap one.** Add PSU / power delivery to the PCS ask.

### 2026-10-11 22:44:56 — Mode B again; 20 h 17 m outage

```
reset        Sun 2026-10-11 22:44:56+   (bwmon last fsync'd line; journal stops 22:44:47)
back up      Mon 2026-10-12 19:02:27    -> DOWN 20 h 17 m
on return    8/8 DIMMs, 502 GiB, no DIMM lost
BERT record  NONE  -> Mode B
F<->A swap   NOT yet done (serials unchanged: A=CA0A997B, F=9A0AA154)
```

**`orion-bwmon` proved its design.** It logged to 22:44:56 — **nine seconds past the
journal's last entry** — because journald buffers while bwmon fsyncs every 2 s. Final line is
complete and intact, consistent with power loss between fsync'd writes. Best crash
timestamp obtained to date.

State at death: **8.14 GB/s, 1.89% CPU, load1 3.0, Tctl 82.4 °C** — idle. Preceding hour:
bw mean 9.99 / max 91.58 GB/s; cpu mean 4.08 / max 99.39%; Tctl 59.1–85.6 °C.

### Mode B is now 5 of 9, and splits by recovery time

| Mode B reset | Down for | Interpretation |
|--------------|----------|----------------|
| 10 Sep | ~25 h | needed manual power-on |
| 7 Oct | 4 h 51 m | needed manual power-on |
| **11 Oct** | **20 h 17 m** | needed manual power-on |
| 26 Sep | 13 min | ambiguous |
| 9 Oct | 4.6 min | auto-recovered = internal reset |

Three multi-hour unattended outages, all Mode B. **This is what AC power loss looks like on a
board with "Restore on AC Power Loss" at its default "Power Off"** — it stays off until a
human presses the button. The ~4-minute ones are internal resets with power maintained.

Mode A tally: 17/19/22 Sep, 8 Oct (4). Mode B: 10/26 Sep, 7/9/11 Oct (5).

### UPS IS NOW THE TOP PRIORITY

Standing recommendation since 2026-09-20, unactioned. Three multi-hour outages now, the
latest costing a full working day. **No UPS = Mode B leaves no evidence by construction.**
A USB-monitored UPS (£100–200) either eliminates five of nine resets and all three long
outages, or `nut` logs prove the mains was clean and it becomes a PSU/rail fault for PCS.
Either outcome is worth more than any further analysis of the existing data.

## 2026-10-13 15:07 — crash #10. First BERT caught by auto-capture; first CE precursor; slot F exonerated

The `orion-bert-capture.service` built on 2026-10-09 worked. For the first time we
have the firmware's error record from a crash we did not have to reconstruct.

### Timeline

| time (BST) | event |
|---|---|
| 2026-10-12 19:02 | boot after my warm reboot — trained **8/8**, 536,189,768 K |
| 2026-10-13 15:03:56 | **corrected ECC error, channel 5, csrow 0**, addr 0x826797d00 |
| 2026-10-13 15:07:12 | last bwmon sample: 96.1 GB/s, 41.9% CPU, load1 71.6, 103 running, Tctl 88.8 °C, 126.6 GiB used |
| 2026-10-13 15:10:59 | back up after 3.9 min — trained **6/8**, 401,972,052 K |

Mode A: short internal reset, under genuine memory load, large BERT record, DIMMs lost.
Ran 20 h 05 m from the Oct 12 reboot.

### The CE precursor — new

A corrected ECC error landed **3 min 16 s before** the fatal reset:

    2026-10-13 15:03:56  CECC  bank=22 (UMC)  memory_channel=5  csrow=0
                         addr=0x826797d00  status=0xdc2040000400011b (Error_overflow)

This is the first time a correctable error has preceded a Mode A reset within the
same boot. Not yet a usable predictor (n=1, and Oct 8's reset had no CE), but it is
the first hint that the interface degrades measurably before it fails.

### BERT record: bert-record-20261013.bin (165,668 B, 288 entries, 45 FATAL)

Decoded to `bert-decoded-20261013.txt`. Same fault signature as 17 Sep, different card:

    Section      : Platform Memory Error
    Severity     : FATAL
    FRU          : 'Perr S0:T008:B15'      (MCA bank 0x15 = UMC)
    ErrorStatus  : 0x0000000000011600      bits 8-15 = 22 = ERR_PARITY
                                           bit 16 Address = SET
                                           bit 17 Control = clear
                                           bit 18 Data    = clear
    valid mask   : 0x4039  -> no PhysicalAddress / Bank / Row / Column / Rank
    ErrorType    : 8 (Parity error)
    Location     : Node 0, Card 8, Module 0

Six of the eight trained memory controllers reported a memory section in the same
instant — Cards **2, 3, 6, 8, 9, 11**. Five were Recoverable with a real physical
address and `ErrorStatus = 0x40400`; one (Card 8) was the FATAL parity error with no
address at all. Six-of-eight is exactly the census of 17 Sep. The remaining 44 FATAL
entries are banks B00/B01 (L1/L2) spread over 44 distinct threads — downstream
collateral from cores with loads outstanding when the controller died, not independent
faults.

Card 8 is a *third* distinct card for the fatal parity error: Card 2 on 17 Sep,
Card 8 today.

### Slot F is not the fault — settled

This boot trained **A, D, E, F, G, H** and dropped **B and C**:

    P0 CHANNEL A   64 GB   CA0A997B
    P0 CHANNEL D   64 GB   A80A9950
    P0 CHANNEL E   64 GB   A00AA31B
    P0 CHANNEL F   64 GB   9A0AA154
    P0 CHANNEL G   64 GB   9D0AA163
    P0 CHANNEL H   64 GB   AB0A997B
    (B and C absent)

F had been missing in 6 of 6 previous degraded boots. It now trains, while B — which
had produced 8 of the first 9 corrected errors and had always trained — fails to train.
No DIMM was moved (serials unchanged). **Which channels survive training is not a
property of the modules.** This retires the slot-F hypothesis for good and is the
cleanest common-mode evidence in the file so far.

Channel 11 also produced 12 corrected errors on 10 Oct (03:28–19:40) plus one on
11 Oct, having produced none before. The CE population has now moved across three
channels (5, 6, 11) over three weeks.

### Thermal context

Tctl averaged **89.4 °C over the five minutes before the reset** and peaked at
95.2 °C today — the highest in the whole bwmon record, and at the 95 °C Tctl limit
for this part. Not a cause on its own: the 8 Oct Mode A reset happened at a 70.5 °C
five-minute mean. Logged as an aggravating condition for signal margin, worth
keeping on the chart.

### Crash census from bwmon (gaps > 2 min, 513,229 samples since 7 Oct)

| last sample | gap | GB/s | CPU% | load1 | Tctl | mode |
|---|---|---|---|---|---|---|
| 10-07 14:37:22 | 290.9 min | 0.0 | 0.2 | 2.2 | 60.5 | B (manual power-on) |
| 10-08 14:41:40 | 3.9 min | 168.1 | 100.0 | 201.4 | 73.6 | A |
| 10-09 07:33:23 | 4.4 min | 0.2 | 0.1 | 2.7 | 56.0 | B |
| 10-11 22:44:56 | 1217.7 min | 8.1 | 1.9 | 3.0 | 82.4 | B (manual power-on) |
| 10-13 15:07:12 | 3.9 min | 96.1 | 41.9 | 71.6 | 88.8 | A |

Both Mode A resets are the two highest-bandwidth moments in the gap table; all three
Mode B resets are at essentially zero bandwidth. The two modes remain cleanly separated
by load.

### Still outstanding

BIOS is still **1203 / 18-07-2025**. Nothing in the hardware configuration has changed
since 21 Sep. Unactioned: UPS, BIOS 1317, and `reply-draft-2.txt` to PCS.

## 2026-10-13/14 — crashes #11 and #12: a 90-minute cascade, and a blind spot in my own monitoring

Three resets in 91 minutes, each returning with fewer DIMMs, then a 17-hour outage and a
cold start that restored all eight.

| # | died | gap | GB/s | DIMMs after | firmware record |
|---|---|---|---|---|---|
| 10 | 10-13 15:07:12 | 3.9 min | 96.1 | 6/8 (401,972,052 K) | 288 entries, FATAL parity, Card 8 |
| 11 | 10-13 16:06:02 | 3.8 min | 116.0 | **5/8** (334,863,188 K) | 780 B: map-out of B and C + 3 corrected |
| 12 | 10-13 16:37:55 | **1014 min** | 91.6 | 8/8 after cold start | **none — affirmative "no error"** |

Uptimes: 20 h 05 m, then 55 min, then 28 min.

### Crash #11 — the firmware's own map-out record

`bert-record-20261013-1609.bin`, decoded in full by the kernel. Two FATAL entries are not
runtime errors at all, they are the firmware reporting its own decision:

    Error 0  fatal  fru_text: Locator: P0MC0   node:0 card:0 module:0
             error_type: 15, physical memory map-out event
    Error 1  fatal  fru_text: Locator: P0MB0   node:0 card:5 module:0
             error_type: 15, physical memory map-out event

The firmware names the slots in plain text: **C = card 0, B = card 5**. It then lists three
*corrected* errors with full addresses on cards 2, 6 and 9 (rank 1 on all three). No parity
error this time.

So this reset was not itself a logged parity failure. What the record proves is that the
firmware deliberately disabled B and C, and that the map-out is **sticky across warm resets
and cleared only by a cold start** — which is exactly what Boris reported in September
("when I reboot by hand by pressing button, in general I get back all 512g"). Now documented
from the firmware side.

### Crash #12 — nothing at all

The BERT table on the 14 Oct boot is the 20-byte stub:

    00000000: 0000 0000 1400 0000 0000 0000 0000 0000
    00000010: 0300 0000
    -> data_length = 0, error_severity = 3 (None)

`orion-bert-capture.service` ran and exited 0 on this boot, so this is not a missed capture.
The firmware is affirmatively stating that no hardware error preceded this boot. Combined
with the 17-hour outage needing a manual power-on, the machine simply stopped receiving
power.

### Per-channel bandwidth is the dose variable

Total bandwidth never predicted the resets and neither did core count. Dividing bandwidth by
the number of *trained* channels does. Medians and p95 below; pre-10-09 artifacts clamped at
400 GB/s on read.

| boot | DIMMs | uptime | bw p95 | **per-channel p95** | ended by |
|---|---|---|---|---|---|
| 10-07 19:28 | 8 | 19.22 h | 42.7 | 5.34 | A (parity) |
| 10-08 14:45 | 7 | 16.80 h | 13.8 | 1.96 | B (no trace) |
| 10-09 07:37 | 8 | 63.12 h | 24.1 | 3.01 | B (no trace) |
| 10-12 19:02 | 8 | 20.08 h | 42.1 | 5.26 | A (parity) |
| 10-13 15:10 | 6 | **0.92 h** | 77.9 | **12.99** | A (map-out) |
| 10-13 16:09 | 5 | **0.47 h** | 99.9 | **19.97** | ? (no trace) |

Complete separation with no overlap: at or below 5.34 GB/s per channel the machine lives
16–63 hours; at or above 12.99 it lives under an hour. The two shortest boots on record are
the two with the highest per-channel load.

**This makes the fault self-accelerating.** A reset costs DIMMs; the same workload is then
concentrated on fewer channels; per-channel signalling activity rises; the next reset comes
sooner and costs more DIMMs. That is the 91-minute cascade, start to finish.

**Operational rule, effective now: never leave orion running degraded.** After any reset that
returns fewer than 8 DIMMs, do a full power-off cold start immediately. Continuing on 6 or 5
channels raised the crash rate roughly twentyfold.

### GPU power — a blind spot in this monitoring, now closed

GPU 0 was found at **599.88 W, 90% util, 88 °C** — pegged at its 600 W limit, serving the
qwen LLM endpoint. Neither `cpu_busy_pct` nor `gbs_total` sees any of this. Every "idle"
classification in this file was idle in CPU and DRAM terms only.

That undermines the Mode A / Mode B split as stated on 10-09. The trace-free resets of
10-07 (0.0 GB/s, 0.2% CPU) and 10-09 (0.2 GB/s, 0.1% CPU) were called idle, and a GPU at
600 W would have been invisible in both. Two candidate causes now fit equally:

  a) AC power loss at the wall, as previously assumed;
  b) **PSU over-current protection tripping**, which latches the supply off and looks
     identical from the inside — no firmware trace, machine stays dead until manually
     powered on.

The envelope makes (b) worth taking seriously: 2 x 600 W GPUs + ~350 W CPU PPT + DRAM,
drives and fans is roughly 1.65 kW sustained before transients, and Blackwell workstation
cards have large microsecond spikes. Crash #12 happened at the highest per-channel memory
load on record, which argues against pure coincidence with a wall outage.

`/usr/local/bin/orion-bwmon` patched 14 Oct to log `gpu<N>_w`, `gpu<N>_util`, `gpu<N>_c`
per GPU via `nvidia-smi` (48 ms per call, 1.5 s timeout, empty on failure); pre-patch copy
at `orion-bwmon.bak-20261014`. Verified live: `600.46,90,88,25.59,0,57`. Note that
`bw-20261014.csv` was already open when the service restarted, so its header row names 22
columns while rows after 10:06:34 carry 28; the six trailing columns are in the order above.

### Two things now needed

1. **A USB-monitored UPS.** It is no longer only insurance — it is the one instrument that
   discriminates (a) from (b). If AC is clean through the next trace-free reset, the PSU is
   implicated and the warranty claim changes shape.
2. **Ask PCS what PSU is fitted and its rating**, to add to the letter. No board-level power
   sensors exist on this machine (`hwmon` exposes only nvme, k10temp and a bare `asus`
   node), so this cannot be answered in software.

Still on BIOS 1203 / 18-07-2025. `reply-draft-2.txt` still unsent.

## 2026-10-14 — the PSU is undersized for the machine as sold

From the PCSpecialist pro-forma invoice (3186088, P/O KK3-4278706, 12 Sep 2025,
£28,599.00 inc VAT), file `~/Downloads/pcspecialist-orion-specs3186088.pdf`:

    Power Supply   CORSAIR 1600W PRO SERIES(TM) TITANIUM AX1600i DIGITAL MODULAR PSU
    Graphics Card  96GB PNY NVIDIA RTX Pro 6000 Blackwell Workstation Edition
    2nd Graphics   96GB PNY NVIDIA RTX Pro 6000 Blackwell Workstation Edition
    Processor      AMD Ryzen Threadripper PRO 9995WX 96 Core
    Memory         512GB Kingston DDR5 6400MHz ECC Registered (8 x 64GB)

AX1600i: 1600 W continuous, single +12 V rail rated 133.3 A, with OCP and OPP.

### Power budget

Measured: `nvidia-smi` reports `power.limit = 600.00 W` and `power.max_limit = 600.00 W`
on both cards, and GPU 0 was observed at **599.88 W** sustained while serving the qwen
endpoint. These are the 600 W Workstation Edition, not the 300 W Max-Q.

| item | W on 12 V | source |
|---|---|---|
| 2 x RTX PRO 6000 Blackwell WE @ 600 W | 1200 | measured limit |
| TR PRO 9995WX, 350 W TDP, ~90% VRM efficiency | ~390 | datasheet + estimate |
| 8 x DDR5-6400 64 GB dual-rank RDIMM | ~70 | estimate |
| 2 x Samsung 9100 PRO PCIe 5.0 NVMe | ~24 | estimate |
| Seagate IronWolf Pro 28 TB | ~10 | estimate |
| WRX90E-SAGE SE (2 x 10 GbE, chipset, VRM losses) | ~80 | estimate |
| XE360-TR5 pump + fans, Define 7 XL fans | ~30 | estimate |
| **total** | **~1804 W** | |

**Roughly 13% over the PSU's continuous rating**, and essentially all of it on the single
+12 V rail whose limit is 1600 W / 133.3 A. A conservative recount still lands near 1720 W.

The machine is inside the rating most of the time because GPU 1 usually idles (~27 W
observed). It goes over whenever both cards load together.

### Why this matters for the trace-free resets

An OPP or OCP trip shuts the supply down instantly and latches it off. No firmware trace is
possible, because the firmware loses power in the same instant, and the machine stays dead
until someone presses the button. That is precisely the signature of crashes #12 (17 h down),
#7 (291 min) and #9 (1218 min) — all of which I previously attributed to AC loss at the wall.

### Hypothesis worth testing: one root cause, not two

If the 12 V rail is near its limit, it sags, and the board's memory VRM loses input headroom.
VDD/VDDQ ripple rises and CA-bus margin falls. That would make PSU overload the common cause
of **both** failure modes — the trace-free shutdowns *and* the parity errors — rather than
two unrelated faults. Unverified. It is testable with the two actions below.

### Action 1 (remote, reversible, available now): cap the GPUs

    sudo nvidia-smi -i 0 -pl 450
    sudo nvidia-smi -i 1 -pl 450

`power.min_limit` is 150 W, so 450 W is well inside range. This removes 300 W from the worst
case, bringing it to ~1504 W, inside the rating. Costs maybe 5-10% on GPU throughput. If the
trace-free resets stop, the PSU is confirmed. Not persistent across reboots without a unit.

### Action 2 (physical, 5 minutes, next time the case is open): connect the PSU's USB cable

The AX1600i is a digital supply that reports total output watts, +12 V volts/amps, PSU
temperature and fan speed over an internal USB header. Linux has the driver already:

    $ modinfo corsair-psu
    description: Linux driver for Corsair power supplies with HID sensors interface
    $ sudo modprobe corsair-psu     # loads fine
    $ lsusb | grep -i 1b1c          # nothing
    hwmon: nvme, nvme, k10temp, asus   # no corsairpsu node

The driver loads and finds no device, so **the Corsair Link USB cable is not plugged in**.
Connecting it to any internal USB 2.0 header gives exact PSU wattage, which can then be
logged next to bandwidth and GPU power in `orion-bwmon` and read directly against the next
crash. This would settle the question outright and is the single highest-value physical
action available.

### Action 3: add to the PCS letter

PCSpecialist specified, built and sold this configuration. A 1600 W supply against a ~1800 W
nameplate load is a specification error on their side, independent of the memory fault and
much harder to argue with. Ask them to state in writing what total system power they
calculated for this build.

## 2026-10-14 14:30 — crash #13. The PSU-overload hypothesis is falsified

Not a clean restart: `journalctl -b -1` ends mid-stream at 14:30:03 with no shutdown
sequence. Died 14:30:05, back 14:47:23 after a manual power-on (17.3 min). Returned at
full **8/8** (536,189,768 K). BERT table is the no-error stub again (`data_length = 0`,
`severity = 3`), and the capture service ran. Uptime was 4 h 58 m.

### First crash with GPU telemetry — and it clears the PSU

The patch from this morning paid for itself immediately. Run-up to the reset:

| ts | GB/s | cpu% | load1 | Tctl | GPU0 W | util | GPU1 W | util |
|---|---|---|---|---|---|---|---|---|
| 14:26:44 | 24.4 | 4.0 | 8.03 | 71.6 | 238.9 | 97 | 15.9 | 0 |
| 14:28:07 | 2.2 | 0.6 | 4.08 | 66.9 | 25.6 | 0 | 19.6 | 0 |
| 14:29:51 | 62.7 | 2.7 | 3.62 | 66.0 | 15.7 | 0 | 14.9 | 0 |
| **14:30:05** | **67.7** | **2.8** | **3.50** | **65.8** | **15.4** | **0** | **15.3** | **0** |

**Both GPUs were idle at ~15 W when it died.** Whole-system draw was on the order of
300–400 W against a 1600 W supply. The machine did not crash because the PSU ran out of
headroom, and the over-power hypothesis I raised earlier today does not explain this reset.

Supporting evidence from 7,697 instrumented samples (~4.3 h): GPU0 min 14.3 / max 602.2 /
mean 121.1 W; **GPU1 min 11.8 / max 103.6 / mean 16.4 W**. GPU 1 never meaningfully loads
under this workload, so the ~1800 W two-GPU worst case is essentially theoretical here.

The PSU *specification* finding stands and still belongs in the PCS letter — 1600 W against
an ~1800 W nameplate build is their error. It is not the operational cause of these crashes.

### Correction to yesterday's dose-response

I wrote that per-channel bandwidth separated the boots "completely, with no overlap". The
seventh boot breaks that at the low end:

| per-channel p95 | uptime | firmware record |
|---|---|---|
| 1.96 | 16.80 h | none |
| 3.01 | 63.12 h | none |
| **3.27** | **4.96 h** | **none** |
| 5.26 | 20.08 h | parity |
| 5.34 | 19.22 h | parity |
| 12.99 | 0.92 h | map-out |
| 19.97 | 0.47 h | none |

What survives: **above ~13 GB/s per channel the machine reliably dies inside an hour.** What
does not survive: the claim that low per-channel load protects it. Boots at 3.01 and 3.27
lasted 63 h and 5 h. A second failure process operates at low load, and it is the one that
leaves no firmware record.

The operational rule is unaffected, because a degraded boot forces the same work onto fewer
channels and lands in the fast-death regime: **cold-start after any reset returning < 8 DIMMs.**

### What is left for the no-record resets

Five now (10-08, 10-09, 10-13 16:37, 10-14 14:30, plus 10-07). Load-independent, no firmware
trace, and mostly needing a manual power-on. Remaining candidates:

  a) **AC supply** — wall socket, the 1.5 m IEC C19 cable, or the PSU inlet. A loose or
     marginal connection produces exactly this and is indifferent to load.
  b) **PSU internal fault** — a failing supply can trip OVP/UVP/OTP at low load. Distinct
     from the overload hypothesis just falsified.

### Three cheap actions

1. **Reseat the C19 mains cable** at both ends and check the wall socket. Ten seconds, free,
   and addresses (a) directly. Consider swapping the cable.
2. **BIOS: set "Restore AC Power Loss" to Power On (or Last State).** If it is currently
   Power Off, every supply blip costs hours of downtime instead of a reboot. It is also
   diagnostic: auto-recovery implies AC returned.
3. **UPS, and the Corsair Link cable.** Still the two instruments that discriminate (a) from
   (b). Boris is sourcing the cable.

GPU caps from this morning survived the reset — `nvidia-powerlimit.service` is enabled and
re-applied 450 W on both cards at boot. Keeping them for now: they cost little and the
experiment is cheap to continue.

## 2026-10-15 14:08 — 23 h clean, under heavier load than the boots that died at 20 h

No crash since 10-14 14:47. Full 8/8 (527,420,332 kB), no new BERT capture, ECC counters
0/0. The only deliberate change in place is the 450 W GPU cap.

This boot, 22.5 h covered, 40,519 samples:

| metric | median | p95 | max |
|---|---|---|---|
| DRAM GB/s | 16.33 | 70.11 | 129.58 |
| **per-channel GB/s** | 2.04 | **8.76** | 16.20 |
| cpu busy % | 4.35 | 19.03 | 48.45 |
| load1 | 10.54 | 130.85 | 211.60 |
| Tctl °C | 75.90 | 87.00 | **95.90** |
| GPU0 W | 15.40 | 449.99 | 458.19 |
| GPU1 W | 16.14 | 24.35 | 276.09 |

6.9 h above 5 GB/s per channel, 3.7 min above 13. GPU 0 sat at the 450 W cap for 2.3 h, so
the cap is actively binding — this workload would otherwise be drawing 600 W.

**This is a harder test than the two boots it has already outlasted.** Per-channel p95 of
8.76 exceeds the 5.26 and 5.34 of the 10-12 and 10-07 boots, both of which died with parity
errors at ~20 h. Not yet evidence of a fix: 19.2 h, 20.1 h and 63.1 h boots all occurred
before any change, so 23 h is unremarkable against this machine's own history. The first
genuine signal would be passing **63 h** (the best run on record) at comparable load.

Note the GPU cap is not cleanly falsified as a mitigation even though the *overload*
hypothesis is dead. Crash #13 proved the machine can die at ~350 W total, which rules out
overload as a universal cause; it does not rule out high GPU current contributing to rail
droop and memory margin loss during the parity crashes. Keeping the cap.

Tctl reached 95.90 °C this boot, the highest in the record and at the limit for this part.

## 2026-10-26/27 — BIOS 1317 flashed. It did not fix the fault.

PCSpecialist (Summer Phillips, second-line tech, 15 Oct) asked for two things: stop touching
the CPU/socket, and flash the BIOS then re-test with memory settings at default. Both done.

### The flash took, and the sold spec held

    BIOS version   : 1317          (was 1203)
    BIOS date      : 02/03/2026    (was 18/07/2025)
    MemTotal       : 536,207,768 K = 8/8 DIMMs
    Configured     : 6400 MT/s on all 8, with Ai Overclock Tuner on Auto

6400 at defaults confirms these are JEDEC-rated 6400 RDIMMs, not an overclock. PCS's test
condition and the sold specification are the same thing, which is the ideal outcome.

### It crashed 10 h 06 m later

| | |
|---|---|
| post-flash boot | 2026-10-26 14:26:57 |
| last sample | 2026-10-27 00:33:30 |
| outage | **489.6 min (8 h 10 m)**, manual power-on at 08:43:07 |
| firmware record | **none** — BERT table is the no-error stub (`data_length 0`, `severity 3`) |
| DIMMs on return | 8/8 |

Conditions at the reset:

    DRAM      0.532 GB/s      cpu 0.75%      load1 2.55
    Tctl      94.1 C
    GPU0      449.99 W        GPU1      449.95 W

**Both GPUs at their 450 W caps simultaneously — the first time on record.** Roughly 900 W of
GPU plus an idle CPU, board, drives and cooling: on the order of 1.2–1.3 kW, the highest
system draw measured at any crash. DRAM traffic and CPU were at idle, so this is not the
memory-bandwidth mode.

Tctl at 94.1 °C with the CPU 0.75% busy is itself odd and worth watching — that is case heat
from two 450 W cards, not CPU work.

### The question that now matters most

Was **Restore AC Power Loss** set to **Power On** in the BIOS after the flash? It is the
difference between two readings of the 8-hour outage:

  - **Set to Power On, and it still stayed dead** → mains returning would have restarted it.
    That rules out a simple blip and points hard at the PSU latching itself off.
  - **Not set** → the 8 hours tells us nothing.

Unanswered as of this entry. Cannot be read from Linux.

### Other crashes this window (both pre-flash, BIOS 1203)

**10-23 18:21:10** — 3.9 min reset at **154.8 GB/s**, 33.8% CPU, Tctl 71.5, GPUs idle.
Returned 7/8 (469,080,904 K). BERT captured (`bert-record-20261023.bin`, 780 B): five memory
sections on cards **2, 3, 5, 8, 9**, all Recoverable or Corrected with real physical
addresses, **no FATAL parity entry and no map-out**. Memory-bandwidth mode.

**10-25 22:14:54** — 972 min (16.2 h) outage at idle (3.1 GB/s, 2.6% CPU), GPUs idle,
trace-free, manual power-on. This is the outage Boris ended on 26 Oct by powering up and
flashing.

### Corrected ECC errors, 23–25 Oct (all pre-flash)

    10-23 18:20:03  channel 11 csrow 1
    10-24 22:43:29  channel  5 csrow 0
    10-25 07:16:55  channel  5 csrow 1
    10-25 11:11:46  channel  5 csrow 1

None since the flash, but there have only been ~10 h of post-flash runtime, so that means
nothing yet.

### Where this leaves the warranty case

PCS asked for the firmware update and it has been done exactly as instructed, at their
chosen version, with memory at defaults. The machine still resets. That is a much stronger
position than before: the next message asks them to action the two items they did not
respond to — socket inspection, and validation at the sold specification under sustained
load.

Services all survived the flash: `orion-bwmon` active, `orion-bert-capture` enabled,
`nvidia-powerlimit` active with both caps at 450 W.

Note: `bw-*.csv` timestamps around 2026-10-25 01:00 repeat for an hour (BST→GMT). Use
wall-clock `ts_iso` deltas rather than `ts_epoch` when hunting gaps across that boundary.

## 2026-10-27 13:13 — crash #N on BIOS 1317, and the thermal regime has changed completely

Second crash on the new firmware. Died 13:13:51 after **4 h 31 m**, back at 13:21 by manual
power-on. Trace-free again (BERT no-error stub). Returned 8/8.

Conditions at the reset — the highest-everything crash on record:

    DRAM      120.1 GB/s  = 15.0 GB/s per channel
    cpu       35.7%       load1 61.12
    Tctl      95.2 C
    GPU0      450.02 W    GPU1      450.01 W

### The regime change

Comparing whole boots. Column "hot" is the fraction of samples above 94 °C.

| boot | duration | Tctl med | hot | GPU0 mean | GPU1 mean | both >400 W |
|---|---|---|---|---|---|---|
| 10-14 14:47 | **194.8 h** | **56.9** | 0.5% | 32 W | 22 W | 0.1% |
| 10-23 18:25 | 43.8 h | 61.4 | 3.3% | 24 W | 126 W | 0.0% |
| 10-25 15:28 | 6.6 h | 57.2 | 0.0% | 27 W | 20 W | 0.0% |
| 10-26 14:26 | 9.8 h | 60.8 | 3.8% | 81 W | 88 W | 13.3% |
| **10-27 08:43** | **4.4 h** | **94.8** | **68.0%** | **426 W** | **440 W** | **93.4%** |

The 8-day crash-free run had **both GPUs essentially idle** (32 W and 22 W means) and a CPU
median of 56.9 °C. Since a sustained dual-GPU workload started, the CPU sits at a **median
of 94.8 °C** and spends **68% of its time above 94 °C**. That is 900 W of GPU heat being
dumped into a Define 7 XL and recirculated through the CPU cooler — the CPU is at 95 °C
while only 35% busy, so this is case air, not CPU work.

### Refining this morning's position

Earlier today I argued heat was not the cause, on the grounds that the machine sat *hotter*
than the crash temperature for 24 minutes of the same boot without dying. That within-boot
argument still holds: temperature is not a trigger.

Across boots the picture is different. The longest run on record by a factor of four was
also by far the coolest. Heat and dual-GPU load are not sufficient to cause a crash, but
they track how long the machine survives. Treat them as a stressor that shortens life, not
as the trigger — which is exactly what loss of signal margin on a marginal memory interface
would look like.

Note also the within-boot numbers: the machine was above 13 GB/s per channel for only
**6.6 minutes** of a 4.5-hour boot, and died inside one of those windows, with both GPUs
loaded for 93% of the boot. The memory-bandwidth stressor and the thermal stressor landed
together.

### Action available now

Lower the GPU caps from 450 W to ~300 W each. Removes 300 W of case heat, is remote,
reversible, and testable against the regime that produced the 8-day run. Costs GPU
throughput on someone's job, so it is Boris's call. `nvidia-powerlimit.service` already
makes any cap persistent across reboots.

Longer term this machine needs its case airflow looked at if two 450 W cards are going to be
a normal workload. 900 W of GPU in a quiet-oriented tower with a 360 mm AIO is marginal.

### Still unanswered

`Restore AC Power Loss` — Boris was in the BIOS at 13:15 and confirmed memory and BIOS
version, but not this setting. Baseline `Setup` efivar saved at
`/var/log/orion-bwmon/efivar/Setup-baseline-20261027.bin` (sha256 ae5f528f…6820) for diffing
once it is changed.

### Restore AC Power Loss: it was OFF the whole time. Now set to Power On (2026-10-27)

Boris found it set to **Power Off** and changed it to **Power On**.

**This retires a line of reasoning.** Every long outage in this file — 17 h, 16.2 h, 20.3 h,
8.2 h — was a machine configured to stay dead after losing power. "It needed a manual
power-on" therefore says only that power was interrupted; it says nothing about whether the
interruption was a brief mains blip or the PSU latching itself off. Those two remained
indistinguishable, and earlier entries that leaned on the long outages as evidence for a
latch should be read with that in mind.

It also means a large part of the lost time was avoidable: outages that ran 16–20 hours
would have been a reboot.

**From now on the next long outage is diagnostic:**

  - comes back by itself → power was lost and restored. Blip, or a PSU that recovered.
  - still stays dead → power never returned on its own, i.e. the supply latched off hard.

#### Reading the setting remotely without a reboot

Diffing the `Setup` efivar across the change (baseline saved before it) isolated it:

    offset 198 (0x0c6):  0 -> 2     Restore AC Power Loss  (0 = Power Off, 2 = Power On)
    offset 437 (0x1b5):  1 -> 0     unidentified; likely a settings-changed flag

Only two bytes moved for one menu change, so offset 198 is the setting with high confidence
and 437 is incidental. To read it later:

    ssh orion "sudo sh -c 'cat /sys/firmware/efi/efivars/Setup-ec87d643-eba4-4bb5-a1e5-3f3e36b20da9' | od -An -tu1 -j198 -N1"

Confirmed reading 2 as of 2026-10-27. Snapshots in `/var/log/orion-bwmon/efivar/`.
Note the offsets are specific to BIOS 1317 and will move if the firmware is updated again.

## 2026-10-27 17:14 — third crash on BIOS 1317, at complete idle. The envelope is now fully covered.

Died 17:14:09, back 17:15:37. **Gap 88 seconds** — the fastest recovery in the whole record
(previous internal resets ran 3.9–4.4 min) and apparently unattended. Trace-free, 8/8
retained. Uptime 3 h 53 m.

Conditions: **everything idle.**

    DRAM   2.739 GB/s     cpu 2.30%      load1 0.67     mem 18.12 GiB
    Tctl   70.4 C
    GPU0   15.25 W        GPU1   19.51 W

### Three crashes on 1317, spanning the entire operating envelope

| time | uptime | DRAM | cpu | Tctl | GPU0 | GPU1 | outage |
|---|---|---|---|---|---|---|---|
| 10-27 00:33 | 10 h 06 m | 0.5 GB/s | 0.8% | 94.1 | 450 W | 450 W | 8 h 10 m |
| 10-27 13:13 | 4 h 31 m | 120.1 GB/s | 35.7% | 95.2 | 450 W | 450 W | 7.2 min |
| 10-27 17:14 | 3 h 53 m | 2.7 GB/s | 2.3% | **70.4** | **15 W** | **20 W** | **88 s** |

Idle and loaded. Hot and cool. GPUs pegged and GPUs off. High memory bandwidth and none.
**No operating condition is common to all three.**

This retires the thermal/dual-GPU line opened this afternoon. Heat and GPU load correlate
with survival time across the longer boots — the 8-day run really was the cool one — but
they cannot be the driver, because the machine has now died at 70 °C with both cards idle.
The earlier reading stands only as "a stressor that shortens life", and even that is now
doing less work than it appeared to this afternoon.

### The intervals are shortening

On BIOS 1317: **10 h 06 m → 4 h 31 m → 3 h 53 m**. Three crashes in 28 hours.

For comparison, the preceding boots on 1203 ran 194.8 h, 43.8 h and 6.6 h. The firmware
update has not helped, and the machine is getting worse rather than holding steady.

### On the AC Power Loss setting

Now confirmed live at the firmware level (byte 198 = 2, Power On). This crash recovered in
88 s, which is too fast to distinguish an internal warm reset from a power interruption
followed by auto-restart — and a true AC loss would force a full retrain of 512 GB and boot
*slower*, not faster, so a warm internal reset remains the better reading.

The setting earns its keep on the next **long** outage: if the machine comes back by itself
after minutes or hours, power was lost and restored; if it still sits dead, the supply
latched off.

## 2026-10-28 02:11 — THE FATAL PARITY ERROR RECURS ON BIOS 1317

Fourth crash on the new firmware, and the first to leave a FATAL firmware record. This is
the decisive result: **the AGESA update did not fix the fault.**

### The record — `bert-record-20261028.bin`, 17,364 B, 34 entries, severity FATAL

**Two** fatal entries this time, both MCA bank 0x15 (Unified Memory Controller):

    Section      : Platform Memory Error      Section      : Platform Memory Error
    Severity     : FATAL                      Severity     : FATAL
    FRU          : 'Perr S0:T002:B15'         FRU          : 'Perr S0:T008:B15'
    ErrorStatus  : 0x0000000000011600         ErrorStatus  : 0x0000000000011600
    ErrorType    : 8 (Parity error)           ErrorType    : 8 (Parity error)
    valid mask   : 0x4039                     valid mask   : 0x4039
    Location     : Node 0, Card 2, Module 0   Location     : Node 0, Card 8, Module 0

`ErrorStatus = 0x0000000000011600` is **bit-for-bit identical** to the records of 17 Sep and
13 Oct, taken on BIOS 1203: bits 8–15 = 22 = `ERR_PARITY`, bit 16 (Address) set, bits 17
(Control) and 18 (Data) clear. Validity mask `0x4039` again carries no PhysicalAddress,
Bank, Row, Column or Rank — there is no address to report because the address itself
arrived corrupted.

Four AGESA revisions of memory-training and stability work changed nothing about this fault.

**It has also doubled.** Previously one fatal parity entry per event — Card 2 on 17 Sep,
Card 8 on 13 Oct. Both cards now fire in the same instant. Six controllers reported
altogether (Cards 2, 3, 5, 6, 8, 9), the same six-of-eight census as every prior event.

### Conditions

    DRAM   167.571 GB/s  = 20.9 GB/s per channel   <- highest per-channel at any crash
    cpu    36.51%        load1 59.32               mem 53.15 GiB
    Tctl   68.0 C                                  <- cool
    GPU0   14.50 W       GPU1   12.68 W            <- both idle

4.0 min reset, returned **7/8** — **slot F dropped** (461,378,504 kB ≈ 440 GiB). F was the
6/6 absentee in early October, trained fine on 13 Oct when B and C dropped instead, and is
now out again. Serials unchanged: A `CA0A997B`, B `A00A9902`, C `9E0AA2C7`, D `A80A9950`,
E `A00AA31B`, G `9D0AA163`, H `AB0A997B`.

This is the memory-bandwidth mode in its purest form yet — highest per-channel traffic on
record, cool, GPUs idle. It confirms the thermal and GPU lines are irrelevant to *this*
mode, and it is the mode that produces firmware records and costs DIMMs.

### Correction to yesterday's "intervals are shortening"

I called the cadence accelerating on three points (10 h 06 → 4 h 31 → 3 h 53). The fourth
breaks it: this boot ran **8 h 56 m**. Four crashes on 1317 in 36 hours, but no monotonic
trend. The rate is high; it is not provably rising.

### Where this leaves PCSpecialist

They asked for exactly one thing: flash the BIOS and re-test with memory at defaults. Done,
at their chosen version, verified at 8 x 64 GB / 6400 MT/s on Auto. The identical fatal
hardware error recurred within 36 hours, now on two controllers at once, and cost a DIMM.

The remaining asks — socket inspection, and validation at the sold specification under
sustained load — are no longer requests they can defer to a firmware update.

## 2026-10-28 14:22 — the first crash with a visible precursor

Crash #18. Boot 02:15:53 → last sample **14:22:20** = **12 h 06 m**, the longest run on
1317. Self-reset of 3.8 min, back at 14:26:11. Returned at **5/8 DIMMs** (`MemTotal`
329,258,012 kB ≈ 314 GiB) — the smallest total of the whole investigation.

### Linux saw it coming

For the first time the OS logged the fault *before* the machine died. Eight corrected ECC
errors in the final 2 h 11 m of an otherwise clean 12-hour boot, every one with the
overflow bit set:

| time | slot | channel | bank |
|---|---|---|---|
| 12:11:05 | **C** | 0 | MC21 |
| 12:11:05 | **C** | 5 | MC22 |
| 12:32:56 | **B** | 5 | MC22 |
| 13:16:37 | **C** | 0 | MC21 |
| 13:27:32 | **B** | 5 | MC22 |
| 13:38:28 | **B** | 5 | MC22 |
| 13:38:28 | **B** | 0 | MC21 |
| 14:22:09 | **B** | 5 | MC22 |

`MC22_STATUS[Over|CE|MiscV|AddrV|-|-|SyndV|CECC|-|-|-]`, `cache level: L3/GEN, tx: GEN,
mem-tx: RD`, `Unified Memory Controller Ext. Error Code: 0`. The last one:

    14:22:09  FRU Text: Locator: P0MB0
              Error Addr: 0x0000000c5b7d3f40
              Syndrome: 0x020002000a801209
              EDAC MC0: 1 CE on mc#0csrow#1channel#5

**Eleven seconds later the machine was gone, and B and C did not come back.** The
precursor named the casualties in advance. Nothing like this appears in any earlier boot —
every previous DIMM loss was silent until the firmware record was read after the fact.

`Over` on all eight means each logged CE is the survivor of a burst the bank could not
hold, so eight entries understate the real count.

### Firmware record — `bert-record-20261028-1426.bin`

141,924 bytes, severity FATAL, **268 entries**: 262 IA32/X64 Processor Error sections
spanning threads T000–T095 (90 Cache Check, 172 Micro-Architectural Check, uncorrected
L4/GEN instruction fetches — the machine-check fallout across the whole part) and 6
Platform Memory Error sections.

    entry  1   FATAL        FRU 'Locator: P0MF0'   Card 11  ErrorType 15 (Physical memory map-out)
    entry  9   FATAL        Perr S0:T002:B16       Card  3  ErrorType  8 (Parity error)  ErrorStatus 0x11600
    entry  8   Recoverable  Card 2   0x40400  PA 0x780bc340     bank 31  col 576  rank 1
    entry 16   Corrected    Card 6   0x40400  PA 0x507e30f900   bank 18  col 864  rank 0
    entry 19   Corrected    Card 8   0x40400  PA 0x507e2f5d40   bank  7  col 768  rank 0
    entry 20   Recoverable  Card 9   0x40400  PA 0x20e68ca900   bank  1  col 224  rank 0

Two things are new in the firmware record. First, a **map-out event and a fatal parity
error in the same record** — on 13 Oct 16:09 the map-out (slots C and B) came in a record
of its own with no parity error, and at 02:11 today the two parity errors came with no
map-out. Second, the map-out names **`P0MF0`, slot F**, which was already untrained at this
boot, so the firmware is re-asserting a map-out for a slot it had already dropped.

`ErrorStatus 0x11600` on Card 3 is again bit-for-bit the 17 Sep / 13 Oct / 02:11 signature.
Fifth occurrence, third BIOS-1317 occurrence, now on a third distinct controller (Card 8,
then Cards 2+8, now Card 3).

### Conditions

    DRAM    56.2 GB/s   = 8.0 GB/s per channel over 7 channels
    cpu      9.72%      load1 19.10              mem 88.92 GiB
    Tctl    83.1 C                               (boot max 95.1, p95 94.6)
    GPU0   229 W  100% util  55 C                GPU1  237 W  100% util  69 C

Mid-envelope on every axis. Both GPUs pinned at 100 % but drawing ~230 W each, well inside
the 450 W cap. Not the ≥13 GB/s/channel fast-kill regime, not idle either.

### What this changes

The dose-response picture does not explain this crash — 8.0 GB/s/channel ran fine for ten
hours before the errors started, and the errors started while nothing about the load
changed. What the precursor shows instead is **progressive degradation on specific
channels**: B and C accumulated correctable errors for two hours, the error rate rose, and
then the controller gave up and the box reset. That is a different shape of story from
"load crosses a threshold and it dies instantly", and it is the first mechanism in this
investigation that is directly observable while the machine is still alive.

It also means the two failure modes may not be two. A silent crash is what this looks like
if the degradation runs faster than the 20-minute gaps between logged bursts.

Slots lost so far, cumulatively: F (02:11), B and C (14:22). Trained now: A, D, E, G, H —
serials `CA0A997B`, `A80A9950`, `A00AA31B`, `9D0AA163`, `AB0A997B`, all at 6400 MT/s.

### Action: watch the precursor

`rasdaemon` already stores these to `/var/lib/rasdaemon/ras-mc_event.db` and
`ras-mc-ctl --errors` reads them back. The useful change is to alert on the *first* CE of a
burst rather than discover it afterwards, which buys 2 hours of warning to drain work off
the machine before the reset.

## Slot-failure census — only ever B, C, F, G

Assembled across the whole case. Every DIMM-loss event, and which slots were lost:

| date | slots lost |
|---|---|
| 2026-06-08 | B, C, F, G |
| 2026-06-15 | B, F |
| 2026-09-19 21:11 | F, G |
| 2026-09-19 23:04 | B, C, F, G |
| 2026-09-21 (derated to 5600) | **none — all 8 trained, F and G came back** |
| 2026-10-13 | B, C |
| 2026-10-28 02:11 | F |
| 2026-10-28 14:22 | B, C |

**A, D, E and H have never been lost. Not once, in five months, across two BIOS versions
and every load condition.** Every single casualty has come from {B, C, F, G}.

Today's precursor confirms it at the OS level independently of POST training: all eight
pre-crash corrected ECC errors were on **B and C**, no other slot.

### What the pattern means

The failure follows **slot position**, deterministically. It does not follow the module —
no module has ever been moved and the serials are fixed, so a travelling defect is
excluded. And it is not random: a 7-event run confined to the same four of eight slots is
not chance.

It is also **mirror-symmetric**: {B, C} on one bank and {F, G} on the other are the same
two positions within each four-slot bank, with the first and last of each bank surviving.
A position-dependent property replicated on both halves of the board fits a routing,
termination or DRAM-power-zone characteristic — and fits the CPU's channel pin assignment
equally well, since that is itself laid out symmetrically to permit symmetric board
routing. **The symmetry does not discriminate CPU from board.**

ASUS's own population order in the manual (1 DIMM → A1; 2 → A1,E1; 4 → +B1,F1;
6 → +C1,G1; 8 → all) does *not* predict the failures: D and H are populated last and never
fail, while A and E are populated first and also never fail. So the failing set is not
simply "the channels ASUS ranks worst". Don't lean on it in either direction.

### Combined with the 5600 result and today's precursor

Three facts that have to be explained together:

1. Four channels are marginal and four are not — a **static** property, stable for months.
2. At 5600 MT/s all eight train and the machine is stable — a **margin** deficit, not a
   dead cell or a broken trace.
3. Today, B and C accumulated corrected errors over two hours at a rising rate before
   collapsing — a **time- or temperature-dependent** erosion of what margin is left.

So: four channels sit near the edge of their timing budget at 6400, and something
progressive eats the remainder. Tctl p95 was **94.6 °C** on that boot (max 95.1), at the
limit for this part. The UMCs are on the CPU's I/O die; sustained operation at 95 °C is a
credible margin-erosion mechanism, and a credible long-term degradation mechanism for
socket contact resistance through thermal cycling.

This is *not* the thermal hypothesis retired on 2026-10-27. That one claimed temperature at
the instant of the crash was the trigger, and the 68–70 °C crashes falsified it. This claim
is about cumulative exposure eroding a static margin deficit, and it is consistent with
crashes occurring at any instantaneous temperature.

### Operational consequence — go back to 5600

The machine is at **5/8 DIMMs (314 GiB)** and has lost three modules in 24 hours. It is
only at 6400 because PCS asked for defaults for their test. **That test is complete**: three
crashes on 1317 at defaults, the fatal parity signature reproduced, three DIMMs mapped out.
Continuing at 6400 buys no further information and costs availability. The 2026-09-21
result says 5600 restores 8/8 and goes quiet.

Revert to DDR5-5600 in `Ai Tweaker` → `Memory Frequency` and keep the 6400 record as the
warranty evidence.

## 2026-10-30 11:17 — first deliberate reproduction. 38 seconds.

`mpstress` (source and binary at `orion:/home/cmbagent/orion-stress/`) was run as a
worker-count ramp: `512` MB per worker, `240` s per stage, stages of 24 / 48 / 96 / 192
workers. It did not reach stage 3.

**The run's own log did not survive.** The binary that executed at 11:12 is not the one now
in `/home/cmbagent/orion-stress/` (both files there were created at 11:34, after the
reboot), and no `ramp.log` exists anywhere on the filesystem. Everything below is
reconstructed from `orion-bwmon`, which sampled throughout at ~2 s. Next run: put the log
under `/scratch` or `/var/log/orion-bwmon/` and confirm it exists before starting.

### Starting condition — fully recovered, 41 hours clean

The 2026-10-28 18:14:48 boot trained **8/8 at 503 GiB**, back up from 5/8 (314 GiB) after
the 14:22 crash. It then ran **41 h 03 m** at light load with no incident — the
second-longest run on record, behind only 63.1 h. `ce_count` and `ue_count` zero.

### The run

| | stage 1 | stage 2 |
|---|---|---|
| workers | 24 | 48 |
| window | 11:12:23 → 11:16:25 | 11:16:27 → **11:17:05** |
| duration | **232 s — survived** | **38 s — died** |
| DRAM sustained | 160 GB/s | 163 GB/s |
| per channel (8 ch) | **20.1 GB/s** | **20.4 GB/s** |
| cpu_busy | 13.3 % | 26.3 % |
| mem_used | 112.8 GiB | 124.4 GiB |
| Tctl | 59–66 °C | 60–63 °C |
| GPUs | idle, 17–20 W | idle, 17–20 W |

The 288–296 GB/s in the first ~15 s of stage 1 is a first-touch transient (page allocation
and zeroing), not sustained load. Discount it.

4.3 min self-reset, back at 11:21:20 with **6/8 — F and G dropped** (377 GiB).

### Thermal and GPU: conclusively excluded

Tctl never exceeded **65.9 °C**, 29 °C below the 95 °C limit this part has been hitting on
ordinary workloads. Both GPUs idle at under 20 W, so total system draw was a few hundred
watts against a 1600 W PSU. Memory pressure was 124 GiB of 503. The machine was killed with
every other axis of the envelope slack. **Memory traffic alone, in isolation, hard-resets
this machine in 38 seconds.** That is the single cleanest result in the case.

### It is NOT per-channel bandwidth

This is a correction to the dose-response model. Sustained bandwidth in the two stages was
**the same** — 160 vs 163 GB/s, 20.1 vs 20.4 GB/s per channel. Both stages were
bandwidth-saturated; that is this access pattern's ceiling. Stage 1 held 20.1 GB/s/channel
for four minutes without trouble. Stage 2 held 20.4 and died in 38 seconds.

So the "≥13 GB/s/channel → dies within the hour" rule from the observational data was
fitting the wrong variable. What actually doubled between the stages:

- worker processes 24 → 48, so **concurrent independent address streams** 24 → 48
- `cpu_busy` 13.3 % → 26.3 %
- resident buffer set 12 GiB → 24 GiB

At constant data throughput, doubling the number of scattered streams roughly doubles the
**row-activation and command rate** — ACT/PRE/RD/WR traffic on the command/address bus —
because each stream walks its own rows. Sequential streaming moves many bytes per
activation; 48 independent streams move the same bytes with far more commands.

**The fatal signature in this case has always been CA-bus parity with the Address bit set**
(`ErrorStatus 0x11600`, `ERR_PARITY`, no reportable address because the address itself
arrived corrupted). An experiment that holds data throughput constant and doubles command
rate, and kills the machine six times faster, points straight at the command/address bus
rather than the data path. The two lines of evidence now agree.

Caveats, stated plainly: n = 1, two stages. Stage 1 ran only 232 s and might have died at
300. Doubling workers also doubled CPU utilisation, so CA rate is not cleanly isolated from
SoC/Infinity-Fabric activity. This is a strong hint, not a proof.

### No fatal firmware record, and no warning

`bert-record-20261030.bin`, 780 bytes, **severity 2 (Corrected)**. Five Platform Memory
Error sections, all the routine `0x40400` DRAM storage errors with resolved addresses on
Cards 2, 3, 5, 8, 9. **No parity entry. No map-out entry.** Yet the machine hard-reset and
permanently lost two DIMMs.

`mpstress` aborts on the first new EDAC CE. It never fired — `ce_count` was still 0 at the
reset, and the previous boot's journal contains **zero** `Hardware Error` lines. So unlike
28 Oct, there was no precursor at all: no corrected errors, no two-hour ramp, nothing. The
box simply stopped.

This blurs the two failure modes further. A reset under heavy memory traffic can come with
a full fatal parity record (17 Sep, 13 Oct, 28 Oct 02:11), with a map-out record
(19 Sep, 13 Oct 16:09), with a two-hour corrected-error precursor (28 Oct 14:22), or with
nothing but routine corrected entries and no warning (today). The firmware record is not a
reliable discriminator and the absence of one means nothing.

### Census holds

F and G. Eight loss events now, every casualty still from **{B, C, F, G}**; A, D, E and H
have never been lost.

### Still at 6400

The 2026-10-28 recommendation to revert to DDR5-5600 was not applied — `dmidecode` reports
6400 MT/s on all six trained modules. The machine is now a reproducible test rig for the
fault, which is useful, but it is not a usable workstation at this setting.
