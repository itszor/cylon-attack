Cylon Attack: a hybrid build
============================

Cylon Attack (Julian Brown, 1999-2000; GPL) on libroger, built from its
source: the C compiled natively, the assembler translated from the
original !RunImage by SuperSpider and run on libroger. The game's C and
C3Dlib's C run as host code. The asm (the sprite scalers, rectangles,
smooth points and console, and C3Dlib's texture mappers, depth-sort
renderer, fixed-point maths and screen/OS utilities) runs as translated
ARM, as do the author's FlexiSound module and the C start-up.

Building
--------

Needs libroger (its Makefile builds libroger.a; SDL3 via pkg-config) and
gcc. The translation is included (hybrid/gen), so SuperSpider is only
needed to make it again.

    cd hybrid && make ROGER=/path/to/libroger && ./cylon

    make regen      analyse and translate the images again (../tools/images.py;
                    SS=/path/to/superspider, which needs --range repeatable)

ROGER and SS default to ~/stuff/libroger and ~/stuff/superspider. Run
./cylon from hybrid/: the game's files are found at ../cylon.

Layout
------

    source/              the source as released (casrc.zip; not modified)
    cylon/!Cylon/        the game as released: !RunImage, FlexiSound, data
    tools/images.py      the translated images and their ranges: analyse, gen
    tools/asmsyms.py     finds each EXPORTed asm routine of source/ in !RunImage
    tools/funcmap.py     !RunImage's functions with the names Acorn C embeds
    analysis/            SuperSpider's analyses
    hybrid/              the build: Makefile, game.c (roger_config),
                         shims.c and hyrt.c (the boundary), hybrid.h, inc/
                         (swis.h, kernel.h), cylon.patch, gen/ (translated)

How it fits together
--------------------

The program starts as the original does: libroger loads !RunImage at &8000
and runs its AIF header and the Shared C Library stubs (translated), whose
start-up initialises libroger's C library and calls main. libroger's
`call_addr` hook (game.c) sends that call to the native main instead.

- C → asm: shims.c defines each asm routine the C calls, with the header's
  signature, as a call of the routine's translation at its address in
  !RunImage (found by asmsyms.py; byte-identical routines placed by source
  order). Arguments go in R0-R3 and the result comes back in R0.
- asm → C: the one C function the asm calls (texture2glue, for 24bpp
  polygons) is dispatched through the same hook to the native function.
  The asm's calls to malloc, exit and division go through the stub vectors
  to libroger's C library, as in the original.
- Memory: the asm reads and writes what the C allocates, so there is one
  heap, the C library's in emulated memory. hybrid.h sends the native C's
  malloc/calloc/realloc/free there through the image's stub vector, so a C
  pointer to heap data is a host pointer into MEM, passed to the asm as its
  address there. Small structures the C passes from its stack or static
  data (point, point2d, matrix) are copied onto the ARM stack for the call,
  and copied back where the asm may write them.
- Structures holding pointers: only C3Dlib's `decor` is read by the asm
  with pointers in it, and the c3d_render shim rebuilds it with 32-bit
  addresses. `bpolygon` and `blist` are allocated by the C but linked and
  read by the asm; the fields the C writes are all words below the `next`
  pointer, so they're at the same offsets on a 64-bit host.
- Static data the asm reads from the C: smoothpoint's cuberoottab and
  smoothpttab (at &17D88/&17E88 in the image). smoothptinit is wrapped
  to copy the native tables there once they're computed.
- The OS: `_swi`/`_swix` (inc/swis.h) call libroger's SWIs, passing
  pointers as above. printf/puts/putchar go to the VDU, and fopen maps
  RISC OS names (Cylon:objects.mother) through libroger.
- The C's names are linked into one object that exports only hy_native,
  so they don't clash with libroger's (both have a `bank`).

Changes to the C (hybrid/cylon.patch, applied to a copy): the sprite
loaders' pointer arithmetic through `(int)` becomes `(intptr_t)`; and
insertscore no longer shifts the hiscore table into `hiscores[24]`, one
past its end. On RISC OS those 8 bytes were the next heap block's header;
libroger's heap has no headers in emulated memory, so they were the next
block, the sceneinfo, and its viewbuf pointer (a crash after entering
initials).
hybrid.h also gives a null string RISC OS's page-zero semantics for atol,
atoi and strcmp, because C3Dlib's object loader runs strtok past the end
of its `normals` lines and calls atol(NULL). On RISC OS that reads 0.

SuperSpider's `--range` is repeatable for this, so one image holds just
the asm ranges and the C between them stays untranslated.

Notes
-----

- Headless runs (`--limit`) are deterministic. Emulated time advances only
  with ARM instructions, so the native C takes no emulated time, except
  that a timed key read (OS_Byte 129) that times out lets its time limit
  pass (hyrt.c), or the hiscore entry would never see a scripted key.
  That loop reads the keyboard buffer: script Return as `type:13`.
- initglue/freeglue (s.preemptglue) are unused (PREEMPTIVE_UPDATE isn't
  defined) and aren't in !RunImage. c/sound (DataVox) isn't linked by the
  original either.
- The game has no quit key: close the window.
