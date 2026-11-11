#!/usr/bin/env bash
# Host-side unit tests: GoogleTest + ThreadX host shim, with gcov/gcovr coverage.
#
# Outputs (uploaded as CI artifacts):
#   build/ut/junit/idr_unit_tests.xml   test results (JUnit-style XML written by GoogleTest)
#   build/ut/coverage/index.html        coverage report (open index.html)
#   build/ut/coverage/coverage.xml      coverage report (Cobertura XML)
#   build/ut/coverage/summary.txt       coverage summary table
#
# Optional environment:
#   COVERAGE_MIN   fail if line coverage of Application/ is below this percent (default 0 = report only)
set -euo pipefail
cd "$(dirname "$0")/.."

OUT=build/ut
COVERAGE_MIN=${COVERAGE_MIN:-0}

cmake -S test -B "$OUT" -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="--coverage -O0"
cmake --build "$OUT"

mkdir -p "$OUT/junit" "$OUT/coverage"

# --- run the tests; keep going so reports are produced even when a test fails ---
set +e
"$OUT/idr_unit_tests" --gtest_color=no --gtest_output=xml:"$OUT/junit/idr_unit_tests.xml"
TEST_RC=$?
set -e

# --- coverage of the application code only (not tests, shim or third-party) ---
set +e
gcovr -r . --object-directory "$OUT" \
      --filter 'IndRover_MotorECU/Application/' \
      --exclude 'IndRover_MotorECU/Application/UnitTest/' \
      --xml-pretty --xml "$OUT/coverage/coverage.xml" \
      --html-details "$OUT/coverage/index.html" \
      --txt "$OUT/coverage/summary.txt" \
      --print-summary \
      --fail-under-line "$COVERAGE_MIN"
COV_RC=$?
set -e

# --- show results on the GitHub run summary page ---
if [ -n "${GITHUB_STEP_SUMMARY:-}" ] && [ -f "$OUT/junit/idr_unit_tests.xml" ]; then
  python3 - "$OUT/junit/idr_unit_tests.xml" >> "$GITHUB_STEP_SUMMARY" <<'PY'
import sys, xml.etree.ElementTree as ET
root = ET.parse(sys.argv[1]).getroot()
tests, failed, disabled = (int(root.get(k, 0)) for k in ("tests", "failures", "disabled"))
print("### Unit tests\n")
print("| Tests | Passed | Failed | Disabled |\n|---|---|---|---|")
print(f"| {tests} | {tests - failed - disabled} | {failed} | {disabled} |\n")
for tc in root.iter("testcase"):
    fail = tc.find("failure")
    if fail is not None:
        print(f"- FAILED `{tc.get('classname')}.{tc.get('name')}`: {(fail.get('message') or '')[:200]}")
PY
  if [ -f "$OUT/coverage/summary.txt" ]; then
    { echo; echo "### Coverage (Application/)"; echo '```'; cat "$OUT/coverage/summary.txt"; echo '```'; } >> "$GITHUB_STEP_SUMMARY"
  fi
fi

if [ "$TEST_RC" -ne 0 ]; then
  echo "Unit tests FAILED"
  exit "$TEST_RC"
fi
if [ "$COV_RC" -ne 0 ]; then
  if [ "$COVERAGE_MIN" -gt 0 ]; then
    echo "Coverage gate FAILED (minimum ${COVERAGE_MIN}% line coverage)"
    exit "$COV_RC"
  fi
  echo "::warning::gcovr failed (exit $COV_RC) - tests passed, coverage report may be missing"
fi
