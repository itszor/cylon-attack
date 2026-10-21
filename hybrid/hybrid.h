/* Included before every file of the game's C (gcc -include): the C library
 * calls that must reach the emulated machine rather than the host's.
 *
 * - the heap: the assembler reads and writes what the C allocates (sprites,
 *   polygon buffers, depth-sort buckets) and allocates some itself
 *   (c3d_loadfile, which the C frees), so there is one heap, the Shared C
 *   Library's in emulated memory (libroger's), reached through the image's
 *   own stub vector. A pointer into it is a host pointer into MEM.
 * - exit and atexit: the game's exit handlers run while the machine still
 *   does, then the library's exit ends the program as RISC OS would.
 * - the console (printf, puts, putchar): the VDU, as on RISC OS.
 * - fopen: RISC OS names (Cylon:objects.mother) through libroger's mapping.
 * - a null string read as RISC OS reads one: page zero, whose first bytes
 *   (the hardware vectors' branches) are no digits and no text, so atol
 *   gives 0 and strcmp a mismatch. C3Dlib's object loader relies on it
 *   (strtok runs out on the objects' normals lines).
 *
 * Everything else (strings, rand, sprintf, maths...) is the host's. */
#ifndef HYBRID_H
#define HYBRID_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

void *hy_malloc(size_t n);
void *hy_calloc(size_t n, size_t size);
void *hy_realloc(void *p, size_t n);
void hy_free(void *p);
void hy_exit(int status) __attribute__((noreturn));
int hy_atexit(void (*fn)(void));
int hy_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int hy_puts(const char *s);
int hy_putchar(int c);
FILE *hy_fopen(const char *name, const char *mode);

static inline long hy_atol(const char *s) { return s ? atol(s) : 0; }
static inline int hy_atoi(const char *s) { return s ? atoi(s) : 0; }
static inline int hy_strcmp(const char *a, const char *b)
{
    static const char page0[] = "\x18\xF0\x9F\xE5";   /* ldr pc,[pc,#&18]: the reset vector */
    return strcmp(a ? a : page0, b ? b : page0);
}

#define malloc(n) hy_malloc(n)
#define calloc(n, s) hy_calloc(n, s)
#define realloc(p, n) hy_realloc(p, n)
#define free(p) hy_free(p)
#define exit(s) hy_exit(s)
#define atexit(f) hy_atexit(f)
#define printf hy_printf
#define puts(s) hy_puts(s)
#define putchar(c) hy_putchar(c)
#define fopen(n, m) hy_fopen(n, m)
#define atol(s) hy_atol(s)
#define atoi(s) hy_atoi(s)
#define strcmp(a, b) hy_strcmp(a, b)

#endif
