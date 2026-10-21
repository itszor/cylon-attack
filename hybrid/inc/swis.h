/* swis.h for the native build: Acorn C's _swi and _swix, on libroger.
 *
 * The register masks are Acorn's: inputs _IN/_INR, outputs _OUT/_OUTR,
 * _RETURN (which register _swi returns), _FLAGS. The arguments follow the
 * mask as on RISC OS: the inputs in register order, then a pointer for each
 * output, then one for the flags. Each argument's type and size are kept
 * (hy_swiarg), so that a pointer input can be passed as an address in the
 * emulated memory: a pointer into it as its address there, anything else
 * (a string literal, a block on the native stack) copied onto the ARM stack
 * for the call - an array or literal its whole size, a plain pointer as a
 * string - and copied back if the SWI changed it. */
#ifndef HY_SWIS_H
#define HY_SWIS_H
#include <stddef.h>
#include <stdint.h>
#include "kernel.h"

#define _IN(i)       (1u << (i))
#define _INR(i, j)   (~0u << (i) ^ ~0u << ((j) + 1))
#define _OUT(i)      ((1u << 31) >> (i))
#define _OUTR(i, j)  (~0u >> (i) ^ ~0u >> ((j) + 1))
#define _FLAGS       (1u << 21)
#define _RETURN(i)   ((unsigned)(i) << 16)

#define OS_WriteC            0x00
#define OS_Byte              0x06
#define OS_Word              0x07
#define OS_File              0x08
#define OS_Mouse             0x1C
#define OS_SpriteOp          0x2E
#define OS_ReadMonotonicTime 0x42
#define OS_ScreenMode        0x65
#define Sound_Stereo         0x40142

typedef struct { uint64_t v; size_t size; int ptr; } hy_swiarg;
#define HY_ISPTR(x) _Generic((x), char *: 1, const char *: 1, int *: 1, const int *: 1, \
                             unsigned *: 1, void *: 1, const void *: 1, default: 0)
#define HY_SA(x) ((hy_swiarg){(uint64_t)(uintptr_t)(x), sizeof(x), HY_ISPTR(x)})

#define HY_M1(f, a) f(a)
#define HY_M2(f, a, ...) f(a), HY_M1(f, __VA_ARGS__)
#define HY_M3(f, a, ...) f(a), HY_M2(f, __VA_ARGS__)
#define HY_M4(f, a, ...) f(a), HY_M3(f, __VA_ARGS__)
#define HY_M5(f, a, ...) f(a), HY_M4(f, __VA_ARGS__)
#define HY_M6(f, a, ...) f(a), HY_M5(f, __VA_ARGS__)
#define HY_M7(f, a, ...) f(a), HY_M6(f, __VA_ARGS__)
#define HY_M8(f, a, ...) f(a), HY_M7(f, __VA_ARGS__)
#define HY_M9(f, a, ...) f(a), HY_M8(f, __VA_ARGS__)
#define HY_M10(f, a, ...) f(a), HY_M9(f, __VA_ARGS__)
#define HY_M11(f, a, ...) f(a), HY_M10(f, __VA_ARGS__)
#define HY_M12(f, a, ...) f(a), HY_M11(f, __VA_ARGS__)
#define HY_MN(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, N, ...) N
#define HY_MAP(f, ...) HY_MN(__VA_ARGS__, HY_M12, HY_M11, HY_M10, HY_M9, HY_M8, HY_M7, \
                             HY_M6, HY_M5, HY_M4, HY_M3, HY_M2, HY_M1, 0)(f, __VA_ARGS__)

/* the arguments after a sentinel, so that there may be none */
#define HY_SWIARGS(...) ((const hy_swiarg[]){{0, 0, 0} __VA_OPT__(, HY_MAP(HY_SA, __VA_ARGS__))} + 1)

_kernel_oserror *hy_swix(int n, unsigned mask, const hy_swiarg *args);
int hy_swi(int n, unsigned mask, const hy_swiarg *args);
#define _swix(n, mask, ...) hy_swix((n), (mask), HY_SWIARGS(__VA_ARGS__))
#define _swi(n, mask, ...) hy_swi((n), (mask), HY_SWIARGS(__VA_ARGS__))

#endif
