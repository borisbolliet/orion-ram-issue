#!/bin/bash
# Launch an mpstress ramp with a full pre-run state snapshot, detached so it
# survives the ssh session and the crash. Usage: run_stress.sh <tag> <mb> <secs> <n...>
set -u
D=/home/cmbagent/orion-stress/runs
TAG=$1; MB=$2; SECS=$3; shift 3
LOG=$D/$TAG.log
SNAP=$D/$TAG.state

{
  echo "# mpstress run $TAG   launched $(date -Is)"
  echo "# args: mb_per_worker=$MB secs_per_stage=$SECS stages=$*"
  echo
  echo "## uptime"; uptime; who -b
  echo
  echo "## MemTotal"; grep MemTotal /proc/meminfo
  echo
  echo "## trained DIMMs (bank / serial / configured speed)"
  sudo dmidecode -t 17 | awk '/Bank Locator:/{bl=$3" "$4} /Serial Number:/{sn=$3} /Configured Memory Speed:/{print bl, sn, $4" "$5}'
  echo
  echo "## BIOS"; sudo dmidecode -s bios-version; sudo dmidecode -s bios-release-date
  echo
  echo "## EDAC counters"
  for f in /sys/devices/system/edac/mc/mc0/ce_count /sys/devices/system/edac/mc/mc0/ue_count; do
    echo "$f = $(cat $f)"
  done
  echo
  echo "## BERT table size (pre-run)"; ls -la /sys/firmware/acpi/tables/data/BERT
  echo
  echo "## hardware errors this boot"
  echo "count: $(journalctl -b 0 --no-pager | grep -c 'Hardware Error')"
  echo
  echo "## Tctl / GPUs"
  sensors 2>/dev/null | grep -iA2 k10temp | head -5
  nvidia-smi --query-gpu=index,power.draw,power.limit,utilization.gpu,temperature.gpu --format=csv
} > "$SNAP" 2>&1

sync
# detached: survives ssh exit; nice so it does not starve the sampler
setsid nohup nice -n 10 /home/cmbagent/orion-stress/mpstress "$LOG" "$MB" "$SECS" "$@" \
  > "$D/$TAG.out" 2> "$D/$TAG.err" < /dev/null &
echo "launched pid $! -> $LOG"
sleep 3
echo "--- first log lines ---"
head -5 "$LOG" 2>/dev/null || echo "(log not yet written)"
