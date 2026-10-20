# orion-ram-issue

Evidence file for a hardware fault on **orion**, a PCSpecialist workstation
(AMD Threadripper PRO 9995WX / ASUS Pro WS WRX90E-SAGE SE / 8 x 64 GB Kingston
DDR5-6400 ECC RDIMM / 2 x RTX PRO 6000 Blackwell). PCSpecialist ticket **3186088**.

## The fault

The machine hard-resets without warning. The firmware logs an ACPI BERT record
containing a **FATAL DDR5 command/address parity error** in the processor's Unified
Memory Controller, with the Processor Context Corrupt bit set in `MCA_STATUS`. Some
resets leave no firmware trace at all and require a manual power-on.

Evidence to date says this is **not** a failed memory module: the fatal error has
appeared on two different controller cards, six of eight controllers have logged
errors in the same instant, and which channels fail to train changes between boots
with no modules moved.

## Where to start

**`NOTES.md`** is the chronological case file and the only document that needs
reading. Everything else is raw evidence it refers to.

## Layout

| file | what it is |
|---|---|
| `NOTES.md` | the case file, dated entries from 2026-06 onward |
| `reply-draft-2.txt` | long-form escalation letter (superseded; see NOTES for the short version actually sent) |
| `bert_decode.py` | ACPI BERT / CPER parser. `bert_decode.py <file> [n]`, n = entries to print, 0 = all |
| `mem_decode.py` | decoder for CPER Platform Memory Error sections |
| `bert-record-*.bin` | raw BERT records as published by the firmware |
| `bert-decoded-*.txt` | decoded output of the above |
| `orion-ram-diag-*.txt` | full diagnostic dumps |
| `dimm-serial-map-*.txt` | per-slot module serials, to prove modules were not moved |
| `*.jpg` | BIOS screens showing the memory total at POST, before any OS loads |

## Instrumentation on the machine

| unit | what it does |
|---|---|
| `/usr/local/bin/orion-bwmon` + `orion-bwmon.service` | 2 s sampling of DRAM bandwidth per CCD, CPU, load, Tctl, memory, and per-GPU watts/util/temp to `/var/log/orion-bwmon/bw-YYYYMMDD.csv`, fsync'd so it survives a hard reset |
| `orion-bert-capture.service` | copies the ACPI BERT record at boot, since it exists only until the next reset |
| `nvidia-powerlimit.service` | caps both GPUs at 450 W (PSU headroom experiment, 2026-10-14) |

Not tracked here: the 45 MB vendor BIOS binaries, which are re-downloadable.
