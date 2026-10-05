#!/bin/bash
set -e
cd "$(dirname "$0")/.."
make
for m in constant burst random; do ./tbf --mode $m --csv results_$m.csv; done
if [ -e /dev/tbf ]; then
    make ctl
    ./tbf_ctl reset
    ./tbf_ctl config 500000 5000
    ./tbf_ctl send 1000 1000 1000
    ./tbf_ctl stats
    cat /proc/tbf_stats
fi
