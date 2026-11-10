#!/usr/bin/env bash
# Free static analysis: cppcheck + clang-tidy, driven by compile_commands.json.
# Gate: fail if total findings exceed MAX_FINDINGS (default 0).
set -euo pipefail
cd "$(dirname "$0")/.."

CDB=${CDB:-build/fw/compile_commands.json}
OUT=build/static_reports; mkdir -p "$OUT"
MAX_FINDINGS=${MAX_FINDINGS:-0}

# --- cppcheck (project-aware, skips third-party/HAL/ThreadX) ---
cppcheck --project="$CDB" --std=c++17 \
  --enable=warning,style,performance,portability \
  --inline-suppr --suppressions-list=ci/cppcheck-suppressions.txt \
  -i third_party -i Drivers -i Middlewares -i threadx \
  --xml --xml-version=2 --error-exitcode=0 2> "$OUT/cppcheck.xml"
CPPCHECK_N=$(grep -c '<error ' "$OUT/cppcheck.xml" || true)

# --- clang-tidy (config in .clang-tidy) on our own sources only ---
FILES=$(python3 - "$CDB" <<'PY'
import json,sys
for e in json.load(open(sys.argv[1])):
    f=e["file"]
    if not any(s in f for s in ("third_party","Drivers","Middlewares","threadx","/test/",
              "Core/Startup","system_stm32","syscalls.c","sysmem.c","stm32f","stm32g","stm32h","stm32l","_hal_msp","_it.c")):
        print(f)
PY
)
# Let host clang-tidy see the ARM GCC headers instead of x86 ones
ARM_INC=$(arm-none-eabi-g++ -E -x c++ - -v </dev/null 2>&1 \
  | sed -n '/#include <...> search starts here:/,/End of search list./p' | grep '^ ' | sed 's/^ //')
EXTRA=(--extra-arg=--target=arm-none-eabi --extra-arg=-nostdinc --extra-arg=-nostdinc++ --extra-arg=-Wno-unknown-warning-option)
for d in $ARM_INC; do EXTRA+=("--extra-arg=-isystem$d"); done
: > "$OUT/clang-tidy.txt"
for f in $FILES; do
  clang-tidy -p "$(dirname "$CDB")" "${EXTRA[@]}" "$f" >> "$OUT/clang-tidy.txt" 2>&1 || true
done
TIDY_N=$(grep -cE ': (warning|error):' "$OUT/clang-tidy.txt" || true)

echo "cppcheck: $CPPCHECK_N  clang-tidy: $TIDY_N  (limit $MAX_FINDINGS)"
[ $((CPPCHECK_N + TIDY_N)) -le "$MAX_FINDINGS" ] || { echo "Static analysis gate FAILED"; exit 1; }
