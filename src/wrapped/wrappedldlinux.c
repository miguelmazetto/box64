#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <dlfcn.h>

#include "wrappedlibs.h"

#include "debug.h"
#include "wrapper.h"
#include "bridge.h"
#include "librarian/library_private.h"
#include "x64emu.h"
#include "emu/x64emu_private.h"
#include "elfloader.h"
#include "box64context.h"

typedef struct my_tls_s {
    unsigned long int   i;
    unsigned long int   o;
} my_tls_t;

EXPORT void* my___tls_get_addr(x64emu_t* emu, void* p)
{
    my_tls_t *t = (my_tls_t*)p;
    void* ret = GetDTatOffset(emu->context, t->i, t->o);
    return ret;
}

// don't try to load the actual ld-linux (because name is variable), just use box64 itself, as it's linked to ld-linux
const char* ldlinuxName = "ld-linux.so.2";
#define LIBNAME ldlinux

#define PRE_INIT\
    if(1)                                                           \
        lib->priv.w.lib = dlopen(NULL, RTLD_LAZY | RTLD_GLOBAL);    \
    else

// define all standard library functions
#include "wrappedlib_init.h"

