#!/usr/bin/env bash
# Free static analysis: cppcheck + clang-tidy, driven by compile_commands.json.
# Gate: fail if total findings exceed MAX_FINDINGS (default 0).
set -euo pipefail
cd "$(dirname "$0")/.."

CDB=${CDB:-build/fw/compile_commands.json}
OUT=build/static_reports; mkdir -p "$OUT"
MAX_FINDINGS=${MAX_FINDINGS:-0}

SUPP=""; [ -f ci/cppcheck-suppressions.txt ] && SUPP=ci/cppcheck-suppressions.txt
[ -n "$SUPP" ] || echo "WARNING: ci/cppcheck-suppressions.txt not found - running without suppressions"

# --- cppcheck (project-aware, skips third-party/HAL/ThreadX) ---
cppcheck --project="$CDB" --std=c++17 \
  --enable=warning,style,performance,portability \
  --inline-suppr ${SUPP:+--suppressions-list=$SUPP} \
  -i third_party -i Drivers -i Middlewares -i AZURE_RTOS -i threadx -i Core \
  --xml --xml-version=2 --error-exitcode=0 2> "$OUT/cppcheck.xml"

# --- clang-tidy (config in .clang-tidy) on our own C/C++ sources only (never .s/.S) ---
FILES=$(python3 -I - "$CDB" <<'PYEOF'
import json,sys
SKIP=("/Core/","third_party","Drivers","Middlewares","threadx","/test/","/Test/","Core/Startup","AZURE_RTOS",
      "system_stm32","syscalls.c","sysmem.c","stm32f","stm32g","stm32h","stm32l","_hal_msp","_it.c")
for e in json.load(open(sys.argv[1])):
    f=e["file"]
    if f.lower().endswith((".c",".cc",".cpp",".cxx")) and not any(s in f for s in SKIP):
        print(f)
PYEOF
)
# Let host clang-tidy see the ARM GCC headers instead of x86 ones
ARM_INC=$(arm-none-eabi-g++ -E -x c++ - -v </dev/null 2>&1 \
  | sed -n '/#include <...> search starts here:/,/End of search list./p' | grep '^ ' | sed 's/^ //' || true)
EXTRA=(--extra-arg=--target=arm-none-eabi --extra-arg=-nostdinc --extra-arg=-nostdinc++
       --extra-arg=-Wno-unknown-warning-option --extra-arg=-Wno-unused-command-line-argument)
for d in $ARM_INC; do EXTRA+=("--extra-arg=-isystem$d"); done
: > "$OUT/clang-tidy.txt"
for f in $FILES; do
  clang-tidy -quiet -p "$(dirname "$CDB")" "${EXTRA[@]}" "$f" >> "$OUT/clang-tidy.txt" 2>&1 || true
done

# --- readable reports (unique findings; the gate counts exactly these) ---
read -r CPPCHECK_N TIDY_N < <(python3 -I ci/static_report.py "$OUT")
[ -z "${GITHUB_STEP_SUMMARY:-}" ] || cat "$OUT/summary.md" >> "$GITHUB_STEP_SUMMARY"
cat "$OUT/findings.txt"

echo "cppcheck: $CPPCHECK_N  clang-tidy: $TIDY_N  (limit $MAX_FINDINGS)"
[ $((CPPCHECK_N + TIDY_N)) -le "$MAX_FINDINGS" ] || { echo "Static analysis gate FAILED"; exit 1; }
