#!/usr/bin/env python3
"""Turn raw cppcheck.xml + clang-tidy.txt into readable reports.
Usage: static_report.py OUTDIR  -> writes OUTDIR/summary.md, OUTDIR/findings.txt
Prints '<cppcheck_n> <tidy_n>' (unique findings) on stdout."""
import re, sys, collections, xml.etree.ElementTree as ET
out = sys.argv[1]
rx = re.compile(r'^(?P<file>[^\s:][^:]*):(?P<line>\d+):(?P<col>\d+): (?P<sev>warning|error): (?P<msg>.*) \[(?P<chk>[\w.,-]+)\]$')

def rel(p):
    i = p.find('IndRover_MotorECU/')
    return p[i:] if i >= 0 else p

tidy = {}
try:
    for l in open(f'{out}/clang-tidy.txt', errors='replace'):
        m = rx.match(l.rstrip())
        if m:
            d = m.groupdict()
            tidy[(rel(d['file']), d['line'], d['chk'])] = (d['sev'], d['msg'], d['col'])
except FileNotFoundError:
    pass

cpp = {}
try:
    for e in ET.parse(f'{out}/cppcheck.xml').getroot().iter('error'):
        loc = e.find('location')
        f, ln = (rel(loc.get('file')), loc.get('line')) if loc is not None else ('-', '0')
        cpp[(f, ln, e.get('id'))] = (e.get('severity'), e.get('msg'))
except Exception:
    pass

lines = []
for (f, ln, c), (s, m) in sorted(cpp.items()):
    lines.append(f'[cppcheck  ] {f}:{ln}  {s}  ({c})\n              {m}')
for (f, ln, c), (s, m, col) in sorted(tidy.items()):
    lines.append(f'[clang-tidy] {f}:{ln}:{col}  {s}  ({c})\n              {m}')
open(f'{out}/findings.txt', 'w').write('\n'.join(lines) + ('\n' if lines else ''))

def table(title, items):
    cnt = collections.Counter(k[2] for k in items)
    r = [f'### {title}: {len(items)}', '']
    if not items:
        return r + ['None :tada:', '']
    r += ['| Rule | Count |', '|---|---|'] + [f'| `{k}` | {v} |' for k, v in cnt.most_common()]
    r += ['', '| Location | Rule | Message |', '|---|---|---|']
    for k in sorted(items)[:100]:
        v = items[k]
        msg = (v[1] if len(v) > 1 else '').replace('|', '\\|')
        r.append(f'| `{k[0]}:{k[1]}` | `{k[2]}` | {msg} |')
    return r + ['']

md = ['## Static analysis', ''] + table('cppcheck', cpp) + table('clang-tidy', tidy)
open(f'{out}/summary.md', 'w').write('\n'.join(md))
print(len(cpp), len(tidy))
