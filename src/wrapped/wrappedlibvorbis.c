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
#include "callback.h"
#include "box64context.h"
#include "librarian.h"
#include "myalign.h"


const char* libvorbisName = "libvorbis.so.0";
#define LIBNAME libvorbis

#define CUSTOM_INIT \
    box64->vorbis = lib;

#define CUSTOM_FINI \
    lib->context->vorbis = NULL;

#include "wrappedlib_init.h"
