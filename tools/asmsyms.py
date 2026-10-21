#!/usr/bin/env python3
"""Find each EXPORTed assembler routine of the source in !RunImage.

The source's instructions after each exported label are matched against
SuperSpider's disassembly of the image (mnemonic and destination register,
conditions and the objasm/disassembler spelling differences normalised).
Prints `address name` for each export found exactly once; ambiguous or
missing ones are reported on stderr."""
import re, subprocess, sys

SS = sys.argv[1]
IMAGE = sys.argv[2]
SOURCES = sys.argv[3:]
N = 8                                    # instructions compared

CONDS = 'eq|ne|cs|cc|hs|lo|mi|pl|vs|vc|hi|ls|ge|lt|gt|le|al|nv'
DP = 'and|eor|sub|rsb|add|adc|sbc|rsc|tst|teq|cmp|cmn|orr|mov|bic|mvn'

def norm(mn, ops):
    """(mnemonic, first operand) in a common spelling."""
    mn = mn.lower()
    ops = ops.lower().replace('r13', 'sp').replace('r14', 'lr').replace('r15', 'pc') \
        .replace('r10', 'r10').replace('sl', 'r10').replace('fp', 'r11').replace('ip', 'r12')
    mb = re.match(r'^b(%s)$' % CONDS, mn)
    if mn == 'b' or mb:
        return 'b', ''
    if re.match(r'^bl(%s)?$' % CONDS, mn):
        return 'bl', ''
    m = re.match(r'^(ldm|stm)(%s)?(fd|ed|fa|ea|ia|ib|da|db)?(%s)?$' % (CONDS, CONDS), mn)
    if m:
        mn = m.group(1)
    else:
        m = re.match(r'^(%s|mul|mla|ldr|str|swi|svc|bl|b|adr|adrl|swp)(%s)?(s|b|t|bt)?(%s)?(s|b)?$'
                     % (DP + '|', CONDS, CONDS), mn)
        if m:
            mn = m.group(1)
    if mn == 'svc':
        mn = 'swi'
    if mn in ('adr', 'adrl'):
        mn = 'add'                       # disassembled as add/sub rX, pc
    first = ops.split(',')[0].strip().rstrip('!')
    if mn in ('b', 'bl', 'swi'):
        first = ''
    if mn in ('cmp', 'cmn', 'tst', 'teq'):
        first = ''
    return mn, first

def source_exports(path):
    lines = open(path, encoding='latin-1').read().split('\n')
    exports = set()
    for l in lines:
        m = re.match(r'\s+EXPORT\s+(\S+)', l)
        if m:
            exports.add(m.group(1))
    out = {}
    for i, l in enumerate(lines):
        m = re.match(r'^(\w+)\s*(;.*)?$', l)
        if not m or m.group(1) not in exports or m.group(1) in out:
            continue
        seq = []
        for l2 in lines[i + 1:]:
            l2 = l2.split(';')[0].rstrip()
            if not l2.strip():
                continue
            if not l2[0].isspace():      # a label: may carry an instruction
                parts = l2.split(None, 1)
                if len(parts) < 2:
                    continue
                l2 = ' ' + parts[1]
            p = l2.split(None, 1)
            mn = p[0]
            if mn.upper() in ('EXPORT', 'IMPORT', 'ALIGN', 'LTORG', 'ENTRY'):
                continue
            if mn.isupper() or mn in ('%',):
                break                    # a directive: data, macros...
            seq.append(norm(mn, p[1] if len(p) > 1 else ''))
            if mn.lower().startswith('adrl'):
                seq.append(seq[-1])
            if len(seq) >= N:
                del seq[N:]
                break
        out[m.group(1)] = seq
    return out

dis = subprocess.run([SS, 'disasm', IMAGE, '0x8000'], capture_output=True, text=True).stdout
insns = []
for l in dis.split('\n'):
    p = l.split(None, 3)
    if len(p) < 3:
        continue
    a = int(p[0], 16)
    rest = p[2] + (' ' + p[3] if len(p) > 3 else '')
    q = rest.split(None, 1)
    insns.append((a, norm(q[0], q[1] if len(q) > 1 else '')))

found = []
for src in SOURCES:
    cands = []
    for name, seq in source_exports(src).items():
        if not seq:
            print('%s: %s has no instructions' % (src, name), file=sys.stderr)
            continue
        hits = [insns[i][0] for i in range(len(insns) - len(seq))
                if all(insns[i + k][1] == seq[k] for k in range(len(seq)))]
        cands.append((name, hits))
    # exports of one file lie in source order: drop hits out of order with
    # the unique ones around them
    for _ in range(3):
        for i, (name, hits) in enumerate(cands):
            lo = max([h[0] for _, h in cands[:i] if len(h) == 1] or [0])
            hi = min([h[0] for _, h in cands[i + 1:] if len(h) == 1] or [1 << 30])
            cands[i] = (name, [h for h in hits if lo < h < hi] if len(hits) > 1 else hits)
    for name, hits in cands:
        if len(hits) == 1:
            found.append((hits[0], name))
        else:
            print('%s: %s: %s' % (src, name, ' '.join('%x' % h for h in hits) or 'not found'),
                  file=sys.stderr)
for a, n in sorted(found):
    print('%05x %s' % (a, n))
