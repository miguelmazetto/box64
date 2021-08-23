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
#include "librarian.h"
#include "box64context.h"
#include "emu/x64emu_private.h"
#include "myalign.h"

const char* atkName = "libatk-1.0.so.0";
#define LIBNAME atk
static library_t* my_lib = NULL;

#include "generated/wrappedatktypes.h"

typedef struct atk_my_s {
    // functions
    #define GO(A, B)    B   A;
    SUPER()
    #undef GO
} atk_my_t;

void* getAtkMy(library_t* lib)
{
    my_lib = lib;
    atk_my_t* my = (atk_my_t*)calloc(1, sizeof(atk_my_t));
    #define GO(A, W) my->A = (W)dlsym(lib->priv.w.lib, #A);
    SUPER()
    #undef GO
    return my;
}
#undef SUPER

void freeAtkMy(void* lib)
{
    my_lib = NULL;
    (void)lib;
    //atk_my_t *my = (atk_my_t *)lib;
}

#define SUPER() \
GO(0)   \
GO(1)   \
GO(2)   \
GO(3)   \
GO(4)

// AtkEventListenerInit ...
#define GO(A)   \
static uintptr_t my_AtkEventListenerInit_fct_##A = 0;               \
static void my_AtkEventListenerInit_##A()                           \
{                                                                   \
    RunFunction(my_context, my_AtkEventListenerInit_fct_##A, 0);    \
}
SUPER()
#undef GO
static void* find_AtkEventListenerInit_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_AtkEventListenerInit_fct_##A == (uintptr_t)fct) return my_AtkEventListenerInit_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_AtkEventListenerInit_fct_##A == 0) {my_AtkEventListenerInit_fct_##A = (uintptr_t)fct; return my_AtkEventListenerInit_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for atk AtkEventListenerInit callback\n");
    return NULL;
}
// AtkEventListener ...
#define GO(A)   \
static uintptr_t my_AtkEventListener_fct_##A = 0;               \
static void my_AtkEventListener_##A(void* a)                    \
{                                                               \
    RunFunction(my_context, my_AtkEventListener_fct_##A, 1, a); \
}
SUPER()
#undef GO
static void* find_AtkEventListener_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_AtkEventListener_fct_##A == (uintptr_t)fct) return my_AtkEventListener_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_AtkEventListener_fct_##A == 0) {my_AtkEventListener_fct_##A = (uintptr_t)fct; return my_AtkEventListener_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for atk AtkEventListener callback\n");
    return NULL;
}
// AtkKeySnoopFunc ...
#define GO(A)   \
static uintptr_t my_AtkKeySnoopFunc_fct_##A = 0;                                \
static int my_AtkKeySnoopFunc_##A(void* a, void* b)                             \
{                                                                               \
    return (int)RunFunction(my_context, my_AtkKeySnoopFunc_fct_##A, 2, a, b);   \
}
SUPER()
#undef GO
static void* find_AtkKeySnoopFunc_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_AtkKeySnoopFunc_fct_##A == (uintptr_t)fct) return my_AtkKeySnoopFunc_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_AtkKeySnoopFunc_fct_##A == 0) {my_AtkKeySnoopFunc_fct_##A = (uintptr_t)fct; return my_AtkKeySnoopFunc_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for atk AtkKeySnoopFunc callback\n");
    return NULL;
}

#undef SUPER

EXPORT void my_atk_focus_tracker_init(x64emu_t* emu, void* f)
{
    atk_my_t *my = (atk_my_t*)my_lib->priv.w.p2;

    my->atk_focus_tracker_init(find_AtkEventListenerInit_Fct(f));
}

EXPORT uint32_t my_atk_add_focus_tracker(x64emu_t* emu, void* f)
{
    atk_my_t *my = (atk_my_t*)my_lib->priv.w.p2;

    return my->atk_add_focus_tracker(find_AtkEventListener_Fct(f));
}

EXPORT uint32_t my_atk_add_key_event_listener(x64emu_t* emu, void* f, void* p)
{
    atk_my_t *my = (atk_my_t*)my_lib->priv.w.p2;

    return my->atk_add_key_event_listener(find_AtkEventListener_Fct(f), p);
}

#define PRE_INIT    \
    if(box64_nogtk) \
        return -1;

#define CUSTOM_INIT \
    lib->priv.w.p2 = getAtkMy(lib); \
    my_lib = lib;

#define CUSTOM_FINI \
    freeAtkMy(lib->priv.w.p2);  \
    free(lib->priv.w.p2);       \

#include "wrappedlib_init.h"
