#!/usr/bin/env bash
set -euo pipefail
for n in $(seq 0 20); do
  echo "=== Running n=$n ==="
  cmsRun runEventComparator_cfg.py --n "$n"
done
