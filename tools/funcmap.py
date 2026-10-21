#!/usr/bin/env python3
"""List the functions SuperSpider found in !RunImage with the names Acorn C
embeds before each C function (a name, then &FF0000nn: its padded length).
Functions without one are the assembler's (or the stubs')."""
import json, struct, sys

img = open(sys.argv[1], 'rb').read()
an = json.load(open(sys.argv[2]))
base = an['base']

def word(a):
    return struct.unpack_from('<I', img, a - base)[0]

def cname(a):
    if a - 4 < base:
        return None
    w = word(a - 4)
    if w & 0xFFFFFF00 != 0xFF000000:
        return None
    n = w & 0xFF
    s = img[a - 4 - n - base:a - 4 - base]
    return s.split(b'\0')[0].decode('latin-1')

for f in an['funcs']:
    print('%05x %s' % (f, cname(f) or '-'))
