#!/usr/bin/env bash
# orion RAM diagnostic — PCSpecialist ticket 3186088
# Run on orion (Linux). Read-only. Saves a timestamped report you can attach to the case.
#
#   chmod +x diagnose.sh && sudo ./diagnose.sh
#
# (sudo is needed for dmidecode / dmesg / journalctl error access)

set -u
OUT="orion-ram-diag-$(date +%Y%m%d-%H%M%S).txt"

{
  echo "############################################################"
  echo "# orion RAM diagnostic  —  $(date)"
  echo "# host: $(hostname)   kernel: $(uname -r)"
  echo "############################################################"

  echo
  echo "===== 1. What this boot sees ====="
  grep MemTotal /proc/meminfo
  echo
  sudo dmidecode -t 17 | grep -E "Bank Locator:|Size:|Speed:"

  echo
  echo "===== 2. Per-channel summary (populated vs empty) ====="
  sudo dmidecode -t 17 | awk '/Bank Locator:/{b=$0} /Size:/{print b"  ->  "$0}'

  echo
  echo "===== 3. Memory / ECC errors (the smoking gun) ====="
  sudo dmesg | grep -iE "mce|machine check|edac|hardware error|memory error|corrected|uncorrect" \
    || echo "(no matching lines in current dmesg)"

  echo
  echo "===== 4. Why it rebooted — previous boot's errors ====="
  sudo journalctl -b -1 -p err --no-pager 2>/dev/null | tail -60 \
    || echo "(no previous-boot journal available)"

  echo
  echo "===== 5. Boot history (shows the unexpected restart) ====="
  journalctl --list-boots 2>/dev/null | tail -12 \
    || echo "(journalctl --list-boots unavailable)"

  echo
  echo "===== 6. ECC error counters (rasdaemon, if installed) ====="
  sudo ras-mc-ctl --errors 2>/dev/null \
    || echo "ras-mc-ctl not installed (optional: sudo apt install rasdaemon)"

  echo
  echo "===== 7. EDAC counters via sysfs (fallback ECC check) ====="
  echo "(ce_count = corrected errors, ue_count = uncorrectable -> these cause resets)"
  sudo grep -H . /sys/devices/system/edac/mc/mc*/ce_count /sys/devices/system/edac/mc/mc*/ue_count 2>/dev/null \
    || echo "no EDAC sysfs counters"

  echo
  echo "===== 8. Last reboots / shutdowns ====="
  last -x reboot shutdown 2>/dev/null | head -15 || echo "(last unavailable)"

  echo
  echo "############################################################"
  echo "# end of report"
  echo "############################################################"
} 2>&1 | tee "$OUT"

echo
echo ">>> Saved report to: $OUT"
echo ">>> Send me that file (or paste its contents) and copy it into ~/GitHub/orion-ram-issue/"
