#!/usr/bin/env bash
# Host-side unit tests (GoogleTest + CTest) with gcov/gcovr coverage.
set -euo pipefail
cd "$(dirname "$0")/.."

cmake -S test -B build/ut -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="--coverage -O0"
cmake --build build/ut

mkdir -p build/ut/junit build/ut/coverage
ctest --test-dir build/ut --output-on-failure \
      --output-junit "$PWD/build/ut/junit/results.xml"

gcovr -r . --exclude 'test/' --exclude 'third_party/' \
      --xml build/ut/coverage/coverage.xml \
      --html-details build/ut/coverage/index.html \
      --print-summary
