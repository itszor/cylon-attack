/* The game's assembler routines, as the native C sees them: each calls the
 * translation of the routine in !RunImage (at its address there, found by
 * ../tools/asmsyms.py) with its arguments in R0-R3 (hyrt.h), and returns
 * R0. And the other way: the C function the assembler calls
 * (texture2glue), and main, which the C start-up calls.
 *
 * Pointer arguments are to the emulated memory (the heap, the screen) or,
 * for the small structures the C passes from its stack or static data
 * (point2d, point, matrix), copied there. The one structure of pointers
 * the assembler reads, C3Dlib's decor, is rebuilt with 32-bit addresses
 * for c3d_render. bpolygon and blist the C only allocates, and writes the
 * fields of that are words (type ... depth), which are where the assembler
 * expects them on a 64-bit host too. */
#include <string.h>
#include "c3d.h"
#include "scale.h"
#include "rectangle.h"
#include "smoothpoint.h"
#include "console.h"
#include "fmath.h"
#include "sound.h"
#include "hyrt.h"

#define A(p) hy_arm(p)
#define IN(p) hy_in(&f, (p), sizeof *(p))
#define IO(p) hy_io(&f, (p), sizeof *(p))
#define CALL(addr, a0, a1, a2, a3) hy_run(&f, (addr), (uint32_t)(a0), (uint32_t)(a1), (uint32_t)(a2), (uint32_t)(a3))
#define SHIM(...) struct hy_frame f; hy_enter(&f); __VA_ARGS__

/* ------------------------------------------------------------ s.smoothpoint */
/* The assembler reads its tables from the C's static data in the image,
 * which the native smoothptinit (compiled as native_smoothptinit) doesn't
 * fill: copy them there once they are made. */
#define CUBEROOTTAB 0x17D88u
#define SMOOTHPTTAB 0x17E88u
extern char cuberoottab[256];
extern unsigned int smoothpttab[16][16];
void native_smoothptinit(void);
void smoothptinit(void)
{
    native_smoothptinit();
    memcpy(hy_host(CUBEROOTTAB), cuberoottab, sizeof cuberoottab);
    memcpy(hy_host(SMOOTHPTTAB), smoothpttab, sizeof smoothpttab);
}

void smoothpoint(int *scr, int x, int y, int i) { SHIM(CALL(0x080E4, A(scr), x, y, i);) }
void smoothpoint2(int *scr, int x, int y, int i) { SHIM(CALL(0x0818C, A(scr), x, y, i);) }
void blob(int *scr, int x, int y, int i) { SHIM(CALL(0x0822C, A(scr), x, y, i);) }

/* ------------------------------------------------------------ s.fmath */
int fsqrt(int n) { SHIM(return (int)CALL(0x0A094, n, 0, 0, 0);) }

/* ------------------------------------------------------------ s.scale */
void sm_64x64(int *scr, int *bmp, point2d *pos, int s) { SHIM(CALL(0x0B1A0, A(scr), A(bmp), IO(pos), s);) }
void smw_64x64(int *scr, int *bmp, point2d *pos, int s) { SHIM(CALL(0x0B31C, A(scr), A(bmp), IO(pos), s);) }
void sm_32x16(int *scr, int *bmp, point2d *pos, int s) { SHIM(CALL(0x0B3B8, A(scr), A(bmp), IO(pos), s);) }
void m_32x16(int *scr, int *bmp, point2d *pos) { SHIM(CALL(0x0B45C, A(scr), A(bmp), IO(pos), 0);) }
void mw_32x16(int *scr, int *bmp, point2d *pos) { SHIM(CALL(0x0B508, A(scr), A(bmp), IO(pos), 0);) }
void m_XxY(int *scr, int *bmp, const point2d *pos, const point2d *size)
{
    SHIM(CALL(0x0B634, A(scr), A(bmp), IN(pos), IN(size));)
}
void om_XxY(int *scr, int *bmp, const point2d *pos, const point2d *size)
{
    SHIM(CALL(0x0B880, A(scr), A(bmp), IN(pos), IN(size));)
}

/* ------------------------------------------------------------ s.rectangle */
void rectangle(int *scr, int col, const point2d *tl, const point2d *br)
{
    SHIM(CALL(0x0D84C, A(scr), col, IN(tl), IN(br));)
}
void shadedrect(int *scr, const point2d *tl, const point2d *br, int col)
{
    SHIM(CALL(0x0D8D4, A(scr), IN(tl), IN(br), col);)
}
void fillmem(int *start, int word) { SHIM(CALL(0x0D938, A(start), word, 0, 0);) }
void ormem(int *start, int word) { SHIM(CALL(0x0D968, A(start), word, 0, 0);) }
void blinds(int *dest, int *src) { SHIM(CALL(0x0D9A4, A(dest), A(src), 0, 0);) }
void scroller(int *disp, int *other, int startrow, int numrows)
{
    SHIM(CALL(0x0DA14, A(disp), A(other), startrow, numrows);)
}

/* ------------------------------------------------------------ s.console */
void plotconsole(int *scr, int *data) { SHIM(CALL(0x0DC34, A(scr), A(data), 0, 0);) }

/* ------------------------------------------------------------ s.fsndcode */
int flexisound_add(char *name, int looped) { SHIM(return (int)CALL(0x132AC, hy_str(&f, name), looped, 0, 0);) }
void flexisound_remove(int n) { SHIM(CALL(0x132B4, n, 0, 0, 0);) }
void flexisound_play(int n, int ch, int vol, int pitch) { SHIM(CALL(0x132BC, n, ch, vol, pitch);) }
int readtimer1(int n) { SHIM(return (int)CALL(0x132C4, n, 0, 0, 0);) }

/* ------------------------------------------------------------ C3Dlib s.perspect */
int c3d_perspect(point *out, point *in) { SHIM(return (int)CALL(0x11A68, IO(out), IO(in), 0, 0);) }

/* ------------------------------------------------------------ C3Dlib s.bucket */
void c3d_render(blist *in, bpolygon *poly, decor *dec, int *screenbase)
{
    uint32_t d[10] = {
        A(dec->textures), A(dec->textures24bpp), A(dec->patterns), A(dec->gouraudshades),
        A(dec->sprites), A(dec->cloud), A(dec->tables[0]), A(dec->tables[1]), A(dec->tables[2]),
        A(dec->tables[3])};
    SHIM(CALL(0x1306C, A(in), A(poly), hy_in(&f, d, sizeof d), A(screenbase));)
}
void c3d_initlist(blist *in) { SHIM(CALL(0x13120, A(in), 0, 0, 0);) }
bpolygon *c3d_addtoview(blist *in, bpolygon *poly, int depth)
{
    SHIM(return hy_host(CALL(0x13164, A(in), A(poly), depth, 0));)
}
int c3d_onscreen(point *p0, point *p1, point *p2, point *p3)
{
    SHIM(return (int)CALL(0x131BC, IN(p0), IN(p1), IN(p2), IN(p3));)
}
int c3d_checkorder(point *p0, point *p1, point *p2, point *p3)
{
    SHIM(return (int)CALL(0x1321C, IN(p0), IN(p1), IN(p2), IN(p3));)
}
int c3d_checkorder3(point *p0, point *p1, point *p2)
{
    SHIM(return (int)CALL(0x1326C, IN(p0), IN(p1), IN(p2), 0);)
}

/* ------------------------------------------------------------ C3Dlib s.fmath */
int c3d_fsqrt(int n) { SHIM(return (int)CALL(0x132CC, n, 0, 0, 0);) }
void c3d_addvec(point *d, point *a, point *b) { SHIM(CALL(0x132F8, IO(d), IN(a), IN(b), 0);) }
void c3d_subvec(point *d, point *a, point *b) { SHIM(CALL(0x13318, IO(d), IN(a), IN(b), 0);) }
int c3d_dotprod8(point *a, point *b) { SHIM(return (int)CALL(0x13338, IN(a), IN(b), 0, 0);) }
int c3d_dotprod12(point *a, point *b) { SHIM(return (int)CALL(0x13358, IN(a), IN(b), 0, 0);) }
void c3d_crossprod8(point *d, point *a, point *b) { SHIM(CALL(0x13378, IO(d), IN(a), IN(b), 0);) }
void c3d_crossprod12(point *d, point *a, point *b) { SHIM(CALL(0x133BC, IO(d), IN(a), IN(b), 0);) }
void c3d_transformpt12(point *d, point *s, matrix *m, point *off)
{
    SHIM(CALL(0x13400, IO(d), IN(s), IN(m), IN(off));)
}
void c3d_rtransformpt12(point *d, point *s, matrix *m, point *off)
{
    SHIM(CALL(0x1346C, IO(d), IN(s), IN(m), IN(off));)
}
void c3d_normalize12(point *p) { SHIM(CALL(0x134C8, IO(p), 0, 0, 0);) }
void c3d_normalize(point *p) { SHIM(CALL(0x13548, IO(p), 0, 0, 0);) }

/* ------------------------------------------------------------ C3Dlib s.utils */
int *c3d_loadfile(char *path) { SHIM(return hy_host(CALL(0x135C8, hy_str(&f, path), 0, 0, 0));) }
void c3d_screenmode(int mode) { SHIM(CALL(0x13634, mode, 0, 0, 0);) }
void c3d_removecursors(void) { SHIM(CALL(0x13648, 0, 0, 0, 0);) }
int *c3d_getscreenbase(void) { SHIM(return hy_host(CALL(0x13650, 0, 0, 0, 0));) }
void c3d_setscreenbanks(int display, int writeto) { SHIM(CALL(0x13674, display, writeto, 0, 0);) }
int c3d_allocatescreenbanks(int n) { SHIM(return (int)CALL(0x136A4, n, 0, 0, 0);) }
void c3d_resetscreen(int *buffer) { SHIM(CALL(0x136F4, A(buffer), 0, 0, 0);) }
void c3d_clearbank(int *buffer) { SHIM(CALL(0x13728, A(buffer), 0, 0, 0);) }
void c3d_getmouse(int *x, int *y, int *b) { SHIM(CALL(0x13760, IO(x), IO(y), IO(b), 0);) }
int c3d_keydown(int key) { SHIM(return (int)CALL(0x13784, key, 0, 0, 0);) }
int c3d_gettime(void) { SHIM(return (int)CALL(0x137A4, 0, 0, 0, 0);) }

/* ------------------------------------------------------------ into native code */
#define ARM_MAIN        0x08550u
#define ARM_TEXTURE2GLUE 0x10D60u

int cylon_main(void);

int hy_native(uint32_t a, uint32_t *r0)
{
    switch (a) {
    case ARM_MAIN:                        /* from the library's _main */
        *r0 = (uint32_t)cylon_main();
        return 1;
    case ARM_TEXTURE2GLUE:                /* from c3d_render, for 24bpp polygons */
        texture2glue(hy_host(hy_reg(0)), hy_host(hy_reg(1)), (int)hy_reg(2), (int)hy_reg(3));
        *r0 = 0;
        return 1;
    }
    return 0;
}
