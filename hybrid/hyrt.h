/* The native C's side of the boundary with the translated assembler
 * (hyrt.c). Kept apart from roger.h, so that the game's headers and
 * libroger's don't meet.
 *
 * A call into the assembler (shims.c) opens a frame, passes its pointer
 * arguments, calls, and returns R0:
 *
 *     struct hy_frame f;
 *     hy_enter(&f);
 *     return hy_run(&f, ADDR, hy_in(&f, pos, sizeof *pos), n, 0, 0);
 *
 * The emulated machine's memory (MEM) is the one heap (hybrid.h), so a
 * pointer into it is passed as its address there. A pointer to anything
 * else (the native stack, the C's static data) is copied onto the ARM
 * stack for the call: hy_in for what the assembler only reads, hy_io for
 * what it may also write (copied back afterwards). The frame keeps the
 * registers as they were, and the ARM stack pointer, and puts them back. */
#ifndef HYRT_H
#define HYRT_H
#include <stdint.h>

struct hy_frame {
    uint32_t save[16], flags;
    int nback;
    struct { void *host; uint32_t arm, size; } back[8];
};

void hy_enter(struct hy_frame *f);
uint32_t hy_in(struct hy_frame *f, const void *p, uint32_t size);
uint32_t hy_io(struct hy_frame *f, void *p, uint32_t size);
uint32_t hy_str(struct hy_frame *f, const char *s);
uint32_t hy_run(struct hy_frame *f, uint32_t addr, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3);

uint32_t hy_arm(const void *p);         /* a pointer into MEM: its address there (NULL: 0) */
void *hy_host(uint32_t a);              /* ...and back (0: NULL) */
uint32_t hy_reg(int k);                 /* a register: arguments of a call into native code */

/* native code the ARM code calls (shims.c): 1 if a is one, which has been
 * run with its arguments from R0-R3, and *r0 its result */
int hy_native(uint32_t a, uint32_t *r0);

#endif
