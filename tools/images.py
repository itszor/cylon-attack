#!/usr/bin/env python3
"""The translated images of Cylon Attack and the steps that build them.

    images.py analyse   SuperSpider (superspider analyse --full) on each image:
                        analysis/<name>.json (analysis/<name>.roots, if there,
                        adds entry points)
    images.py gen       superspider gen for each image -> hybrid/gen/<name>/

The game is a hybrid: its C is compiled natively (hybrid/), and only the
assembler parts of !RunImage are translated, with the C start-up they need
(the AIF header and the Shared C Library stubs, whose vectors libroger's
C library fills). Each range below is code the source's s/ files (the
game's and C3Dlib's) assembled to, found in the image by asmsyms.py; the C
between them is not translated, so a call from the assembler into it (to
texture2glue) is dispatched, and hybrid/game.c sends it to the native
function.

Images:
  runimage    !Cylon.!RunImage at &8000: the ranges below only
  flexisound  !Cylon.FlexiSound (the author's own sound module) at &1800000
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.dirname(HERE)
APP = os.path.join(TOP, 'cylon', '!Cylon')
SS = os.environ.get('SUPERSPIDER', os.path.expanduser('~/stuff/superspider'))

RANGES = [
    (0x08000, 0x080e4),    # AIF header, zero-initialisation; the stubs' C$$code
                           # (the language's initialisation, the call of _main)
    (0x080e4, 0x08260),    # s.smoothpoint: smoothpoint, smoothpoint2, blob
    (0x0a094, 0x0a0cc),    # s.fmath: fsqrt
    (0x0b1a0, 0x0ba74),    # s.scale: the sprite scalers and plotters
    (0x0d84c, 0x0da70),    # s.rectangle: rectangle ... scroller
    (0x0dc34, 0x0dca0),    # s.console: plotconsole
    (0x11a68, 0x11ab4),    # C3Dlib s.perspect: c3d_perspect
    (0x11ab4, 0x12008),    # the stubs: start-up, library vectors
    (0x12008, 0x17d88),    # C3Dlib's other s files: texture mappers, bucket,
                           # fmath, utils; s.fsndcode (to the end of the image)
]

IMAGES = [
    # name, file (in !Cylon), base, module
    ('runimage', '!RunImage', 0x8000, False),
    ('flexisound', 'FlexiSound', 0x1800000, True),
]


def run(cmd):
    print('+', ' '.join(cmd), flush=True)
    subprocess.run(cmd, check=True)


def exe():
    subprocess.run(['dune', 'build'], cwd=SS, check=True)
    return os.path.join(SS, '_build/default/bin/main.exe')


def analyse():
    ss = exe()
    an = os.path.join(TOP, 'analysis')
    os.makedirs(an, exist_ok=True)
    for name, path, base, module in IMAGES:
        cmd = [ss, 'analyse', os.path.join(APP, path), '%x' % base, '--full',
               '-o', os.path.join(an, name + '.json')]
        rf = os.path.join(an, name + '.roots')
        if os.path.exists(rf):
            cmd += ['--roots', rf]
        if module:
            cmd += ['--module']
        run(cmd)


def gen():
    ss = exe()
    for name, path, base, module in IMAGES:
        out = os.path.join(TOP, 'hybrid', 'gen', name)
        os.makedirs(out, exist_ok=True)
        for f in os.listdir(out):
            os.remove(os.path.join(out, f))
        cmd = [ss, 'gen', os.path.join(APP, path), '%x' % base,
               os.path.join(TOP, 'analysis', name + '.json'), '--c', out,
               '--prefix', name[0].upper() + name[1:], '--tabprefix', name, '--files', '4']
        if name == 'runimage':
            for lo, hi in RANGES:
                cmd += ['--range', '%x-%x' % (lo, hi)]
        run(cmd)


if __name__ == '__main__':
    what = sys.argv[1] if len(sys.argv) > 1 else ''
    if what == 'analyse':
        analyse()
    elif what == 'gen':
        gen()
    else:
        sys.exit(__doc__)
