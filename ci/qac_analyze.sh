#!/usr/bin/env bash
# Perforce/Helix QAC for C++ - CLI driven. Flags differ slightly between QAC
# versions: check `qacli <command> --help` on your server and adjust the lines
# marked ADJUST. Everything else is plain shell.
set -euo pipefail
cd "$(dirname "$0")/.."

: "${QAC_PROJECT:?}"                       # QAC project dir (kept on the server)
: "${QAC_MAX_VIOLATIONS:=0}"
OUT=build/qac_reports; mkdir -p "$OUT"

# One-time project creation (ADJUST: config names for your install, e.g. MISRA C++:2008 / AUTOSAR C++14)
if [ ! -d "$QAC_PROJECT" ]; then
  qacli admin -P "$QAC_PROJECT" --qaf-project-config \
        --cct "$QAC_CCT" --rcf "$QAC_RCF" --acf "$QAC_ACF"
fi

# Feed the real compile commands to QAC (ADJUST: sync type for your version, e.g. JSON compile db)
qacli sync -P "$QAC_PROJECT" -t JSON -O build/fw/compile_commands.json

# Analyse (incremental; -f forces full re-analysis if you prefer)
qacli analyze -P "$QAC_PROJECT" -f

# Reports for humans / audit evidence
qacli report -P "$QAC_PROJECT" -t RCR -o "$OUT"      # rule compliance report
qacli report -P "$QAC_PROJECT" -t SUP -o "$OUT"      # suppression report
qacli view   -P "$QAC_PROJECT" -m STDOUT > "$OUT/violations.txt" || true

# Quality gate: count violation lines (ADJUST the grep to your view output format)
COUNT=$(grep -cE '^\S+:[0-9]+:[0-9]+' "$OUT/violations.txt" || true)
echo "QAC violations: $COUNT (limit $QAC_MAX_VIOLATIONS)"
[ "$COUNT" -le "$QAC_MAX_VIOLATIONS" ] || { echo "QAC gate FAILED"; exit 1; }
