#!/usr/bin/env python3
"""Markdown coverage tables from gcovr JSON.
Usage: coverage_summary.py coverage_summary.json [coverage_full.json]"""
import json, sys

def pct(c, t, p=None):
    if not t:
        return "n/a"
    return f"{(p if p is not None else 100.0 * c / t):.1f}%"

def cell(d, k):
    t, c = d.get(f"{k}_total", 0), d.get(f"{k}_covered", 0)
    return f"{pct(c, t, d.get(f'{k}_percent'))} ({c}/{t})"

def ranges(nums):
    out, nums = [], sorted(set(nums))
    i = 0
    while i < len(nums):
        j = i
        while j + 1 < len(nums) and nums[j + 1] == nums[j] + 1:
            j += 1
        out.append(str(nums[i]) if i == j else f"{nums[i]}-{nums[j]}")
        i = j + 1
    return ", ".join(out)

s = json.load(open(sys.argv[1]))
print("### Code coverage (Application/)\n")
print("| | Lines | Functions | Branches |\n|---|---|---|---|")
print(f"| **Total** | **{cell(s,'line')}** | **{cell(s,'function')}** | **{cell(s,'branch')}** |\n")
print("<sub>Loop coverage is not reported by gcov; loop bodies and exits count as lines and branches. "
      "'n/a' = nothing to measure.</sub>\n")
print("#### Per file\n")
print("| File | Lines | Functions | Branches |\n|---|---|---|---|")
for f in sorted(s.get("files", []), key=lambda x: x["filename"]):
    name = f["filename"].replace("IndRover_MotorECU/Application/", "")
    print(f"| `{name}` | {cell(f,'line')} | {cell(f,'function')} | {cell(f,'branch')} |")
print()
if len(sys.argv) > 2:
    try:
        full = json.load(open(sys.argv[2]))
        rows = []
        for f in sorted(full.get("files", []), key=lambda x: x["filename"]):
            miss = [l["line_number"] for l in f.get("lines", []) if l.get("count", 1) == 0]
            if miss:
                rows.append(f"| `{f['filename'].replace('IndRover_MotorECU/Application/','')}` | {ranges(miss)} |")
        if rows:
            print("#### Uncovered lines\n\n| File | Lines |\n|---|---|")
            print("\n".join(rows))
            print()
    except Exception as e:
        print(f"<sub>uncovered-line list unavailable: {e}</sub>")
