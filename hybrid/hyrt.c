/* The boundary between the game's C, compiled natively, and its assembler,
 * translated from !RunImage and run on libroger (hyrt.h), and the C
 * library calls hybrid.h sends to the emulated machine. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <roger.h>
#include "hyrt.h"
#include "inc/swis.h"

#define MEMSIZE 0x4000000u

/* the image's Shared C Library stub vector (chunk 2, at &11D24): the
 * entries libroger's C library serves, in the library's order */
#define CLIB_CALLOC  0x11E2Cu
#define CLIB_FREE    0x11E30u
#define CLIB_MALLOC  0x11E34u
#define CLIB_REALLOC 0x11E38u
#define CLIB_EXIT    0x11E44u

uint32_t hy_arm(const void *p)
{
    if (!p) return 0;
    const uint8_t *q = p;
    if (q < MEM || q >= MEM + MEMSIZE) {
        fprintf(stderr, "hybrid: %p is not in the emulated memory\n", p);
        roger_finish("native pointer passed to the assembler");
    }
    return (uint32_t)(q - MEM);
}

void *hy_host(uint32_t a) { return a ? MEM + (a & (MEMSIZE - 1)) : NULL; }
uint32_t hy_reg(int k) { return R[k]; }

static int in_mem(const void *p) { return (const uint8_t *)p >= MEM && (const uint8_t *)p < MEM + MEMSIZE; }

void hy_enter(struct hy_frame *f)
{
    memcpy(f->save, R, sizeof f->save);
    f->flags = NF << 3 | ZF << 2 | CF << 1 | VF;
    f->nback = 0;
}

static uint32_t push(const void *p, uint32_t size)
{
    R[13] = (R[13] - size) & ~7u;
    memcpy(MEM + R[13], p, size);
    return R[13];
}

uint32_t hy_in(struct hy_frame *f, const void *p, uint32_t size)
{
    (void)f;
    if (!p) return 0;
    return in_mem(p) ? hy_arm(p) : push(p, size);
}

uint32_t hy_io(struct hy_frame *f, void *p, uint32_t size)
{
    if (!p) return 0;
    if (in_mem(p)) return hy_arm(p);
    uint32_t a = push(p, size);
    if (f->nback == (int)(sizeof f->back / sizeof f->back[0])) roger_finish("hybrid: too many arguments copied back");
    f->back[f->nback].host = p;
    f->back[f->nback].arm = a;
    f->back[f->nback].size = size;
    f->nback++;
    return a;
}

uint32_t hy_str(struct hy_frame *f, const char *s) { return hy_in(f, s, s ? strlen(s) + 1 : 0); }

static void leave(struct hy_frame *f)
{
    for (int k = 0; k < f->nback; k++)
        if (memcmp(f->back[k].host, MEM + f->back[k].arm, f->back[k].size))
            memcpy(f->back[k].host, MEM + f->back[k].arm, f->back[k].size);
    memcpy(R, f->save, sizeof f->save);
    NF = f->flags >> 3 & 1; ZF = f->flags >> 2 & 1; CF = f->flags >> 1 & 1; VF = f->flags & 1;
}

uint32_t hy_run(struct hy_frame *f, uint32_t addr, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
    R[0] = r0; R[1] = r1; R[2] = r2; R[3] = r3;
    roger_call_program(addr, ROGER_HANDLER_RET);
    uint32_t r = R[0];
    leave(f);
    return r;
}

/* ------------------------------------------------------------ the heap */
static uint32_t clib(uint32_t entry, uint32_t a0, uint32_t a1)
{
    struct hy_frame f;
    hy_enter(&f);
    return hy_run(&f, entry, a0, a1, 0, 0);
}

void *hy_malloc(size_t n) { return hy_host(clib(CLIB_MALLOC, (uint32_t)n, 0)); }
void *hy_calloc(size_t n, size_t size) { return hy_host(clib(CLIB_CALLOC, (uint32_t)n, (uint32_t)size)); }
void *hy_realloc(void *p, size_t n) { return hy_host(clib(CLIB_REALLOC, hy_arm(p), (uint32_t)n)); }
void hy_free(void *p) { if (p) clib(CLIB_FREE, hy_arm(p), 0); }

/* ------------------------------------------------------------ exit */
static void (*exit_fns[32])(void);
static int nexit;

int hy_atexit(void (*fn)(void))
{
    if (nexit == 32) return -1;
    exit_fns[nexit++] = fn;
    return 0;
}

void hy_exit(int status)
{
    while (nexit) exit_fns[--nexit]();
    clib(CLIB_EXIT, (uint32_t)status, 0);
    roger_finish("exit");                 /* (the library's exit doesn't return) */
}

/* ------------------------------------------------------------ the console */
static void vdu(const char *s, int n)
{
    uint32_t save[16];
    memcpy(save, R, sizeof save);
    for (int k = 0; k < n; k++) {
        R[0] = (uint8_t)s[k];
        roger_swi(OS_WriteC);
        if (s[k] == '\n') { R[0] = '\r'; roger_swi(OS_WriteC); }
    }
    memcpy(R, save, sizeof save);
}

int hy_printf(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    vdu(buf, n < (int)sizeof buf ? n : (int)sizeof buf - 1);
    return n;
}

int hy_puts(const char *s) { vdu(s, (int)strlen(s)); vdu("\n", 1); return 0; }
int hy_putchar(int c) { char ch = (char)c; vdu(&ch, 1); return c; }

FILE *hy_fopen(const char *name, const char *mode)
{
    char path[2048];
    if (!roger_hostpath(name, path)) return NULL;
    return fopen(path, mode);
}

/* ------------------------------------------------------------ _swi, _swix */
static _kernel_oserror last_error;

static _kernel_oserror *swix(int n, unsigned mask, const hy_swiarg *a, uint32_t *ret)
{
    struct hy_frame f;
    hy_enter(&f);
    for (int r = 0; r < 10; r++) {
        if (!(mask & _IN(r))) continue;
        uint32_t v;
        if (!a->ptr) v = (uint32_t)a->v;
        else if (a->size != sizeof(void *)) v = hy_io(&f, (void *)(uintptr_t)a->v, (uint32_t)a->size);   /* an array */
        else if (in_mem((void *)(uintptr_t)a->v) || !a->v) v = hy_arm((void *)(uintptr_t)a->v);
        else v = hy_str(&f, (const char *)(uintptr_t)a->v);
        R[r] = v;
        a++;
    }
    uint32_t inkey_cs = n == OS_Byte && R[0] == 129 && R[2] < 0x80 ? (R[2] << 8 | R[1]) : 0;
    roger_swi((uint32_t)n | 0x20000u);
    if (inkey_cs && R[2] == 0xFF && !roger_interactive()) {
        /* a key read that timed out: in a headless run, where time is
         * instructions, native code waiting takes none, so let the time
         * limit pass (as the interactive INKEY waits it out) */
        uint32_t r1 = R[1], r2 = R[2];
        roger_swi(OS_ReadMonotonicTime);
        uint32_t until = R[0] + inkey_cs;
        do { roger_idle(1000); roger_swi(OS_ReadMonotonicTime); } while ((int32_t)(R[0] - until) < 0);
        R[1] = r1; R[2] = r2;
    }
    _kernel_oserror *err = NULL;
    if (VF) {
        last_error.errnum = (int)RD32(R[0]);
        snprintf(last_error.errmess, sizeof last_error.errmess, "%s", (char *)MEM + R[0] + 4);
        err = &last_error;
    }
    uint32_t out[16];
    memcpy(out, R, sizeof out);
    uint32_t flags = PSR() >> 28;
    leave(&f);
    if (ret) *ret = out[(mask >> 16) & 15];
    for (int r = 0; r < 10; r++)
        if (mask & _OUT(r)) { *(int *)(uintptr_t)a->v = (int)out[r]; a++; }
    if (mask & _FLAGS) *(int *)(uintptr_t)a->v = (int)(flags << 28);
    return err;
}

_kernel_oserror *hy_swix(int n, unsigned mask, const hy_swiarg *args) { return swix(n, mask, args, NULL); }

int hy_swi(int n, unsigned mask, const hy_swiarg *args)
{
    uint32_t r;
    _kernel_oserror *e = swix(n, mask, args, &r);
    if (e) {
        fprintf(stderr, "SWI &%X: error &%X %s\n", n, e->errnum, e->errmess);
        roger_finish("SWI error");
    }
    return (int)r;
}
