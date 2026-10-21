/* Cylon Attack (Julian Brown, 1999-2000): a hybrid build. The game's C is
 * compiled natively from its source (../source, patched for a 64-bit host
 * by cylon.patch); its assembler - the game's and C3Dlib's s files - is
 * translated from the !RunImage the source built (../tools/images.py),
 * and runs on libroger, which also runs the author's FlexiSound module,
 * translated likewise. shims.c and hyrt.c join the two.
 *
 * !Run: Set Cylon$Dir <Obey$Dir> ; Set Cylon$Path <Cylon$Dir>. ;
 * Set C3Dres$Path <Cylon$Dir>.c3dres. ; RMEnsure FlexiSound 1.11 RMLoad
 * <Cylon$Dir>.FlexiSound ; WimpSlot -min 256K -max 256K ; Run
 * <Cylon$Dir>.!RunImage
 *
 * The program starts as the original does: the AIF header, then the
 * Shared C Library stubs, whose start-up initialises libroger's C library
 * (the heap the native C allocates from too: hybrid.h) and calls main -
 * which is the native main, as is the one C function the assembler calls
 * (hy_native). */
#include <stdio.h>
#include <string.h>
#include <roger.h>
#include "hyrt.h"

extern const struct roger_image runimage_image, flexisound_image;

static const struct roger_module modules[] = {
    {"../cylon/!Cylon/FlexiSound", &flexisound_image, 0, NULL},
    {NULL, NULL, 0, NULL},
};

static const char *const variables[] = {
    "Cylon$Dir", "!Cylon",
    "Cylon$Path", "<Cylon$Dir>.",
    "C3Dres$Path", "<Cylon$Dir>.c3dres.",
    NULL,
};

/* A dispatch to native code: run it as the ARM function it replaces (APCS:
 * R0 the result, R4-R11 and R13 as they were - the native code's own calls
 * into the assembler leave them so - the return to R14 with the caller's
 * flags) */
static int native_call(uint32_t a)
{
    uint32_t r0, save[16];
    memcpy(save, R, sizeof save);
    if (!hy_native(a, &r0)) return 0;
    memcpy(R, save, sizeof save);
    R[0] = r0;
    SETPC(R[14], 1);
    return 1;
}

static const struct roger_config config = {
    .title = "Cylon Attack (hybrid)",
    .image_path = "../cylon/!Cylon/!RunImage",
    .image = &runimage_image,
    .modules = modules,
    .appdir = "../cylon",
    .variables = variables,
    .command_line = "<Cylon$Dir>.!RunImage",
    .initial_mode = 12,
    /* the original's 256K, as !Run gives it: the C library extends the
     * slot for the heap (Wimp_SlotSize) as it would on the desktop */
    .wimpslot = 256 * 1024,
    .call_addr = native_call,
};

int main(int argc, char **argv) { roger_main(argc, argv, &config); }
