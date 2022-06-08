#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "wrappedlibs.h"

#include "debug.h"
#include "wrapper.h"
#include "bridge.h"
#include "callback.h"
#include "box64context.h"
#include "librarian.h"
#include "gtkclass.h"
#include "library.h"
#include "custommem.h"
#include "khash.h"

static bridge_t*        my_bridge           = NULL;
static const char* (*g_type_name)(size_t)   = NULL;
#define GTKCLASS(A) static size_t my_##A    = (size_t)-1;
GTKCLASSES()
#undef GTKCLASS

KHASH_SET_INIT_INT(signalmap)
static kh_signalmap_t *my_signalmap = NULL;

typedef struct sigoffset_s {
    uint32_t offset;
    int     n;
} sigoffset_t;
typedef struct sigoffset_array_s {
    sigoffset_t *a;
    int     cap;
    int     sz;
} sigoffset_array_t;

KHASH_MAP_INIT_INT64(sigoffset, sigoffset_array_t)
static kh_sigoffset_t *my_sigoffset = NULL;

// ---- Defining the multiple functions now -----
#define SUPER() \
GO(0)   \
GO(1)   \
GO(2)   \
GO(3)   \
GO(4)   \
GO(5)   \
GO(6)   \
GO(7)

#define WRAPPED(A, NAME, RET, DEF, N, ...)  \
static uintptr_t my_##NAME##_fct_##A = 0;   \
static RET my_##NAME##_##A DEF              \
{                                           \
    printf_log(LOG_DEBUG, "Calling " #NAME "_" #A " wrapper\n");             \
    return (RET)RunFunction(my_context, my_##NAME##_fct_##A, N, __VA_ARGS__);\
}

#define FIND(A, NAME) \
static void* find_##NAME##_##A(void* fct)                           \
{                                                                   \
    if(!fct) return fct;                                            \
    void* tmp = GetNativeFnc((uintptr_t)fct);                       \
    if(tmp) return tmp;                                             \
    if(my_##NAME##_##A##_fct_0 == (uintptr_t)fct) return my_##NAME##_##A##_0;   \
    if(my_##NAME##_##A##_fct_1 == (uintptr_t)fct) return my_##NAME##_##A##_1;   \
    if(my_##NAME##_##A##_fct_2 == (uintptr_t)fct) return my_##NAME##_##A##_2;   \
    if(my_##NAME##_##A##_fct_3 == (uintptr_t)fct) return my_##NAME##_##A##_3;   \
    if(my_##NAME##_##A##_fct_4 == (uintptr_t)fct) return my_##NAME##_##A##_4;   \
    if(my_##NAME##_##A##_fct_5 == (uintptr_t)fct) return my_##NAME##_##A##_5;   \
    if(my_##NAME##_##A##_fct_6 == (uintptr_t)fct) return my_##NAME##_##A##_6;   \
    if(my_##NAME##_##A##_fct_7 == (uintptr_t)fct) return my_##NAME##_##A##_7;   \
    if(my_##NAME##_##A##_fct_0 == 0) {my_##NAME##_##A##_fct_0 = (uintptr_t)fct; return my_##NAME##_##A##_0; } \
    if(my_##NAME##_##A##_fct_1 == 0) {my_##NAME##_##A##_fct_1 = (uintptr_t)fct; return my_##NAME##_##A##_1; } \
    if(my_##NAME##_##A##_fct_2 == 0) {my_##NAME##_##A##_fct_2 = (uintptr_t)fct; return my_##NAME##_##A##_2; } \
    if(my_##NAME##_##A##_fct_3 == 0) {my_##NAME##_##A##_fct_3 = (uintptr_t)fct; return my_##NAME##_##A##_3; } \
    if(my_##NAME##_##A##_fct_4 == 0) {my_##NAME##_##A##_fct_4 = (uintptr_t)fct; return my_##NAME##_##A##_4; } \
    if(my_##NAME##_##A##_fct_5 == 0) {my_##NAME##_##A##_fct_5 = (uintptr_t)fct; return my_##NAME##_##A##_5; } \
    if(my_##NAME##_##A##_fct_6 == 0) {my_##NAME##_##A##_fct_6 = (uintptr_t)fct; return my_##NAME##_##A##_6; } \
    if(my_##NAME##_##A##_fct_7 == 0) {my_##NAME##_##A##_fct_7 = (uintptr_t)fct; return my_##NAME##_##A##_7; } \
    printf_log(LOG_NONE, "Warning, no more slot for " #NAME " gtkclass callback\n");    \
    return NULL;    \
}

#define REVERSE(A, NAME)   \
static void* reverse_##NAME##_##A(wrapper_t W, void* fct)                       \
{                                                                               \
    if(!fct) return fct;                                                        \
    if((void*)my_##NAME##_##A##_0 == fct) return (void*)my_##NAME##_##A##_fct_0;\
    if((void*)my_##NAME##_##A##_1 == fct) return (void*)my_##NAME##_##A##_fct_1;\
    if((void*)my_##NAME##_##A##_2 == fct) return (void*)my_##NAME##_##A##_fct_2;\
    if((void*)my_##NAME##_##A##_3 == fct) return (void*)my_##NAME##_##A##_fct_3;\
    if((void*)my_##NAME##_##A##_4 == fct) return (void*)my_##NAME##_##A##_fct_4;\
    if((void*)my_##NAME##_##A##_5 == fct) return (void*)my_##NAME##_##A##_fct_5;\
    if((void*)my_##NAME##_##A##_6 == fct) return (void*)my_##NAME##_##A##_fct_6;\
    if((void*)my_##NAME##_##A##_7 == fct) return (void*)my_##NAME##_##A##_fct_7;\
    Dl_info info;                                                               \
    if(dladdr(fct, &info))                                                      \
        return (void*)AddCheckBridge(my_bridge, W, fct, 0, NULL);               \
    return fct;                                                                 \
}

#define AUTOBRIDGE(A, NAME)   \
static void autobridge_##NAME##_##A(wrapper_t W, void* fct)         \
{                                                                   \
    if(!fct)                                                        \
        return;                                                     \
    Dl_info info;                                                   \
    if(dladdr(fct, &info))                                          \
        AddAutomaticBridge(thread_get_emu(), my_bridge, W, fct, 0); \
}

#define WRAPPER(A, NAME, RET, DEF, N, ...)          \
WRAPPED(0, NAME##_##A, RET, DEF, N, __VA_ARGS__)    \
WRAPPED(1, NAME##_##A, RET, DEF, N, __VA_ARGS__)    \
WRAPPED(2, NAME##_##A, RET, DEF, N, __VA_ARGS__)    \
WRAPPED(3, NAME##_##A, RET, DEF, N, __VA_ARGS__)    \
WRAPPED(4, NAME##_##A, RET, DEF, N, __VA_ARGS__)    \
WRAPPED(5, NAME##_##A, RET, DEF, N, __VA_ARGS__)    \
WRAPPED(6, NAME##_##A, RET, DEF, N, __VA_ARGS__)    \
WRAPPED(7, NAME##_##A, RET, DEF, N, __VA_ARGS__)    \
FIND(A, NAME)                                       \
REVERSE(A, NAME)                                    \
AUTOBRIDGE(A, NAME)

// ----- GObjectClass ------
// wrapper x64 -> natives of callbacks
WRAPPER(GObject, constructor, void*, (size_t type, uint32_t n_construct_properties, void* construct_properties), 3, type, n_construct_properties, construct_properties);
WRAPPER(GObject, set_property, void, (void* object, uint32_t property_id, void* value, void* pspec), 4, object, property_id, value, pspec);
WRAPPER(GObject, get_property, void, (void* object, uint32_t property_id, void* value, void* pspec), 4, object, property_id, value, pspec);
WRAPPER(GObject, dispose, void, (void* object), 1, object);
WRAPPER(GObject, finalize, void, (void* object), 1, object);
WRAPPER(GObject, dispatch_properties_changed, void*, (int type, uint32_t n_pspecs, void* pspecs), 3, type, n_pspecs, pspecs);
WRAPPER(GObject, notify, void*, (int type, void* pspecs), 2, type, pspecs);
WRAPPER(GObject, constructed, void, (void* object), 1, object);

#define SUPERGO() \
    GO(constructor, pFLup);                 \
    GO(set_property, vFpupp);               \
    GO(get_property, vFpupp);               \
    GO(dispose, vFp);                       \
    GO(finalize, vFp);                      \
    GO(dispatch_properties_changed, vFpup); \
    GO(notify, vFpp);                       \
    GO(constructed, vFp);

// wrap (so bridge all calls, just in case)
static void wrapGObjectClass(my_GObjectClass_t* class)
{
    #define GO(A, W) class->A = reverse_##A##_GObject (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGObjectClass(my_GObjectClass_t* class)
{   
    #define GO(A, W)   class->A = find_##A##_GObject (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGObjectClass(my_GObjectClass_t* class)
{
    #define GO(A, W) autobridge_##A##_GObject (W, class->A)
    SUPERGO()
    #undef GO
}
#undef SUPERGO

// ----- GtkObjectClass ------
// wrapper x64 -> natives of callbacks
WRAPPER(GtkObject, set_arg, void, (void* object, void* arg, uint32_t arg_id), 3, object, arg, arg_id);
WRAPPER(GtkObject, get_arg, void, (void* object, void* arg, uint32_t arg_id), 3, object, arg, arg_id);
WRAPPER(GtkObject, destroy, void, (void* object), 1, object);

#define SUPERGO() \
    GO(set_arg, vFppu); \
    GO(get_arg, vFppu); \
    GO(destroy, vFp);
// wrap (so bridge all calls, just in case)
static void wrapGtkObjectClass(my_GtkObjectClass_t* class)
{
    wrapGObjectClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkObject (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkObjectClass(my_GtkObjectClass_t* class)
{   
    unwrapGObjectClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkObject (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkObjectClass(my_GtkObjectClass_t* class)
{
    bridgeGObjectClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkObject (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// ----- GtkWidgetClass ------
// wrapper x64 -> natives of callbacks
WRAPPER(GtkWidget, dispatch_child_properties_changed, void, (void* widget, uint32_t n_pspecs, void* pspecs), 3, widget, n_pspecs, pspecs);
WRAPPER(GtkWidget, show,              void, (void* widget), 1, widget);
WRAPPER(GtkWidget, show_all,          void, (void* widget), 1, widget);
WRAPPER(GtkWidget, hide,              void, (void* widget), 1, widget);
WRAPPER(GtkWidget, hide_all,          void, (void* widget), 1, widget);
WRAPPER(GtkWidget, map,               void, (void* widget), 1, widget);
WRAPPER(GtkWidget, unmap,             void, (void* widget), 1, widget);
WRAPPER(GtkWidget, realize,           void, (void* widget), 1, widget);
WRAPPER(GtkWidget, unrealize,         void, (void* widget), 1, widget);
WRAPPER(GtkWidget, size_request,      void, (void* widget, void* requisition), 2, widget, requisition);
WRAPPER(GtkWidget, size_allocate,     void, (void* widget, void* allocation), 2, widget, allocation);
WRAPPER(GtkWidget, state_changed,     void, (void* widget, int previous_state), 2, widget, previous_state);
WRAPPER(GtkWidget, parent_set,        void, (void* widget, void* previous_parent), 2, widget, previous_parent);
WRAPPER(GtkWidget, hierarchy_changed, void, (void* widget, void* previous_toplevel), 2, widget, previous_toplevel);
WRAPPER(GtkWidget, style_set,         void, (void* widget, void* previous_style), 2, widget, previous_style);
WRAPPER(GtkWidget, direction_changed, void, (void* widget, int previous_direction), 2, widget, previous_direction);
WRAPPER(GtkWidget, grab_notify,       void, (void* widget, int was_grabbed), 2, widget, was_grabbed);
WRAPPER(GtkWidget, child_notify,      void, (void* widget, void* pspec), 2, widget, pspec);
WRAPPER(GtkWidget, mnemonic_activate, int, (void* widget, int group_cycling), 2, widget, group_cycling);
WRAPPER(GtkWidget, grab_focus,        void, (void* widget), 1, widget);
WRAPPER(GtkWidget, focus,             int, (void* widget, int direction), 2, widget, direction);
WRAPPER(GtkWidget, event,             int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, button_press_event,int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, button_release_event, int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, scroll_event,      int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, motion_notify_event, int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, delete_event,       int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, destroy_event,      int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, expose_event,       int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, key_press_event,    int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, key_release_event,  int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, enter_notify_event, int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, leave_notify_event, int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, configure_event,    int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, focus_in_event,     int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, focus_out_event,    int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, map_event,          int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, unmap_event,        int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, property_notify_event,  int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, selection_clear_event,  int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, selection_request_event,int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, selection_notify_event, int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, proximity_in_event,  int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, proximity_out_event, int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, visibility_notify_event, int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, client_event,        int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, no_expose_event,     int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, window_state_event,  int, (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget, selection_get,       void, (void* widget, void* selection_data, uint32_t info, uint32_t time_), 4, widget, selection_data, info, time_);
WRAPPER(GtkWidget, selection_received,  void, (void* widget, void* selection_data, uint32_t time_), 3, widget, selection_data, time_);
WRAPPER(GtkWidget, drag_begin,          void, (void* widget, void* context), 2, widget, context);
WRAPPER(GtkWidget, drag_end,            void, (void* widget, void* context), 2, widget, context);
WRAPPER(GtkWidget, drag_data_get,       void, (void* widget, void* context, void* selection_data, uint32_t info, uint32_t time_), 5, widget, context, selection_data, info, time_);
WRAPPER(GtkWidget, drag_data_delete,    void, (void* widget, void* context), 2, widget, context);
WRAPPER(GtkWidget, drag_leave,          void, (void* widget, void* context, uint32_t time_), 3, widget, context, time_);
WRAPPER(GtkWidget, drag_motion,         int, (void* widget, void* context, int32_t x, int32_t y, uint32_t time_), 5, widget, context, x, y, time_);
WRAPPER(GtkWidget, drag_drop,           int, (void* widget, void* context, int32_t x, int32_t y, uint32_t time_), 5, widget, context, x, y, time_);
WRAPPER(GtkWidget, drag_data_received,  void, (void* widget, void* context, int32_t x, int32_t y, void* selection_data, uint32_t info, uint32_t time_), 7, widget, context, x, y, selection_data, info, time_);
WRAPPER(GtkWidget,  popup_menu,         int  , (void* widget), 1, widget);
WRAPPER(GtkWidget,  show_help,          int  , (void* widget, int help_type), 2, widget, help_type);
WRAPPER(GtkWidget, get_accessible,      void*, (void* widget), 1, widget);
WRAPPER(GtkWidget, screen_changed,      void , (void* widget, void* previous_screen), 2, widget, previous_screen);
WRAPPER(GtkWidget, can_activate_accel,  int  , (void* widget, uint32_t signal_id), 2, widget, signal_id);
WRAPPER(GtkWidget, grab_broken_event,   int  , (void* widget, void* event), 2, widget, event);
WRAPPER(GtkWidget,  composited_changed, void , (void* widget), 1, widget);
WRAPPER(GtkWidget,  query_tooltip,      int  , (void* widget, int32_t x, int32_t y, int keyboard_tooltip, void* tooltip), 5, widget, x, y, keyboard_tooltip, tooltip);

#define SUPERGO() \
    GO(dispatch_child_properties_changed, vFpup);   \
    GO(show, vFp);                                  \
    GO(show_all, vFp);                              \
    GO(hide, vFp);                                  \
    GO(hide_all, vFp);                              \
    GO(map, vFp);                                   \
    GO(unmap, vFp);                                 \
    GO(realize, vFp);                               \
    GO(unrealize, vFp);                             \
    GO(size_request, vFpp);                         \
    GO(size_allocate, vFpp);                        \
    GO(state_changed, vFpi);                        \
    GO(parent_set, vFpp);                           \
    GO(hierarchy_changed, vFpp);                    \
    GO(style_set, vFpp);                            \
    GO(direction_changed, vFpi);                    \
    GO(grab_notify, vFpi);                          \
    GO(child_notify, vFpp);                         \
    GO(mnemonic_activate, iFpi);                    \
    GO(grab_focus, vFp);                            \
    GO(focus, iFpi);                                \
    GO(event, iFpp);                                \
    GO(button_press_event, iFpp);                   \
    GO(button_release_event, iFpp);                 \
    GO(scroll_event, iFpp);                         \
    GO(motion_notify_event, iFpp);                  \
    GO(delete_event, iFpp);                         \
    GO(destroy_event, iFpp);                        \
    GO(expose_event, iFpp);                         \
    GO(key_press_event, iFpp);                      \
    GO(key_release_event, iFpp);                    \
    GO(enter_notify_event, iFpp);                   \
    GO(leave_notify_event, iFpp);                   \
    GO(configure_event, iFpp);                      \
    GO(focus_in_event, iFpp);                       \
    GO(focus_out_event, iFpp);                      \
    GO(map_event, iFpp);                            \
    GO(unmap_event, iFpp);                          \
    GO(property_notify_event, iFpp);                \
    GO(selection_clear_event, iFpp);                \
    GO(selection_request_event, iFpp);              \
    GO(selection_notify_event, iFpp);               \
    GO(proximity_in_event, iFpp);                   \
    GO(proximity_out_event, iFpp);                  \
    GO(visibility_notify_event, iFpp);              \
    GO(client_event, iFpp);                         \
    GO(no_expose_event, iFpp);                      \
    GO(window_state_event, iFpp);                   \
    GO(selection_get, vFppuu);                      \
    GO(selection_received, vFppu);                  \
    GO(drag_begin, vFpp);                           \
    GO(drag_end, vFpp);                             \
    GO(drag_data_get, vFpppuu);                     \
    GO(drag_data_delete, vFpp);                     \
    GO(drag_leave, vFppu);                          \
    GO(drag_motion, iFppiiu);                       \
    GO(drag_drop, iFppiiu);                         \
    GO(drag_data_received, vFppiipuu);              \
    GO(popup_menu, iFp);                            \
    GO(show_help, iFpi);                            \
    GO(get_accessible, pFp);                        \
    GO(screen_changed, vFpp);                       \
    GO(can_activate_accel, iFpu);                   \
    GO(grab_broken_event, iFpp);                    \
    GO(composited_changed, vFp);                    \
    GO(query_tooltip, iFpiiip);

// wrap (so bridge all calls, just in case)
static void wrapGtkWidgetClass(my_GtkWidgetClass_t* class)
{
    wrapGtkObjectClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkWidget (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkWidgetClass(my_GtkWidgetClass_t* class)
{   
    unwrapGtkObjectClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkWidget (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkWidgetClass(my_GtkWidgetClass_t* class)
{
    bridgeGtkObjectClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkWidget (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// ----- GtkContainerClass ------
// wrapper x64 -> natives of callbacks
WRAPPER(GtkContainer, add, void, (void* container, void* widget), 2, container, widget);
WRAPPER(GtkContainer, remove, void, (void* container, void* widget), 2, container, widget);
WRAPPER(GtkContainer, check_resize, void, (void* container), 1, container);
WRAPPER(GtkContainer, forall, void, (void* container, int include_internals, void* callback, void* callback_data), 4, container, include_internals, AddCheckBridge(my_bridge, vFpp, callback, 0, NULL), callback_data);
WRAPPER(GtkContainer, set_focus_child, void, (void* container, void* widget), 2, container, widget);
WRAPPER(GtkContainer, child_type, int, (void* container), 1, container);
WRAPPER(GtkContainer, composite_name, void*, (void* container, void* child), 2, container, child);
WRAPPER(GtkContainer, set_child_property, void, (void* container, void* child, uint32_t property_id, void* value, void* pspec), 5, container, child, property_id, value, pspec);
WRAPPER(GtkContainer, get_child_property, void, (void* container, void* child, uint32_t property_id, void* value, void* pspec), 5, container, child, property_id, value, pspec);

#define SUPERGO() \
    GO(add, vFpp);                  \
    GO(remove, vFpp);               \
    GO(check_resize, vFp);          \
    GO(forall, vFpipp);             \
    GO(set_focus_child, vFpp);      \
    GO(child_type, iFp);            \
    GO(composite_name, pFpp);       \
    GO(set_child_property, vFppupp);\
    GO(get_child_property, vFppupp);

// wrap (so bridge all calls, just in case)
static void wrapGtkContainerClass(my_GtkContainerClass_t* class)
{
    wrapGtkWidgetClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkContainer (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkContainerClass(my_GtkContainerClass_t* class)
{   
    unwrapGtkWidgetClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkContainer (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkContainerClass(my_GtkContainerClass_t* class)
{
    bridgeGtkWidgetClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkContainer (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// ----- GtkActionClass ------
// wrapper x64 -> natives of callbacks
WRAPPER(GtkAction, activate, void, (void* action), 1, action);
WRAPPER(GtkAction, create_menu_item, void*, (void* action), 1, action);
WRAPPER(GtkAction, create_tool_item, void*, (void* action), 1, action);
WRAPPER(GtkAction, connect_proxy, void , (void* action, void* proxy), 2, action, proxy);
WRAPPER(GtkAction, disconnect_proxy, void , (void* action, void* proxy), 2, action, proxy);
WRAPPER(GtkAction, create_menu, void*, (void* action), 1, action);

#define SUPERGO() \
    GO(activate, vFp);          \
    GO(create_menu_item, pFp);  \
    GO(create_tool_item, pFp);  \
    GO(connect_proxy, vFpp);    \
    GO(disconnect_proxy, vFpp); \
    GO(create_menu, pFp);       \

// wrap (so bridge all calls, just in case)
static void wrapGtkActionClass(my_GtkActionClass_t* class)
{
    wrapGObjectClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkAction (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkActionClass(my_GtkActionClass_t* class)
{   
    unwrapGObjectClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkAction (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkActionClass(my_GtkActionClass_t* class)
{
    bridgeGObjectClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkAction (W, class->A)
    SUPERGO()
    #undef GO
}
#undef SUPERGO

// ----- GtkMiscClass ------

// wrap (so bridge all calls, just in case)
static void wrapGtkMiscClass(my_GtkMiscClass_t* class)
{
    wrapGtkWidgetClass(&class->parent_class);
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkMiscClass(my_GtkMiscClass_t* class)
{   
    unwrapGtkWidgetClass(&class->parent_class);
}
// autobridge
static void bridgeGtkMiscClass(my_GtkMiscClass_t* class)
{
    bridgeGtkWidgetClass(&class->parent_class);
}


// ----- GtkLabelClass ------
// wrapper x64 -> natives of callbacks
WRAPPER(GtkMisc, move_cursor, void, (void* label, int step, int count, int extend_selection), 4, label, step, count, extend_selection);
WRAPPER(GtkMisc, copy_clipboard, void, (void* label), 1, label);
WRAPPER(GtkMisc, populate_popup, void, (void* label, void* menu), 2, label, menu);
WRAPPER(GtkMisc, activate_link, int, (void* label, void* uri), 2, label, uri);

#define SUPERGO() \
    GO(move_cursor, vFpiii);    \
    GO(copy_clipboard, vFp);    \
    GO(populate_popup, vFpp);   \
    GO(activate_link, iFpp);    \

// wrap (so bridge all calls, just in case)
static void wrapGtkLabelClass(my_GtkLabelClass_t* class)
{
    wrapGtkMiscClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkMisc (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkLabelClass(my_GtkLabelClass_t* class)
{   
    unwrapGtkMiscClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkMisc (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkLabelClass(my_GtkLabelClass_t* class)
{
    bridgeGtkMiscClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkMisc (W, class->A)
    SUPERGO()
    #undef GO
}
#undef SUPERGO

// ----- GtkTreeViewClass ------
// wrapper x64 -> natives of callbacks
WRAPPER(GtkTreeView, set_scroll_adjustments, void, (void* tree_view, void* hadjustment, void* vadjustment), 3, tree_view, hadjustment, vadjustment);
WRAPPER(GtkTreeView, row_activated, void, (void* tree_view, void* path, void* column), 3, tree_view, path, column);
WRAPPER(GtkTreeView, test_expand_row, int, (void* tree_view, void* iter, void* path), 3, tree_view, iter, path);
WRAPPER(GtkTreeView, test_collapse_row, int, (void* tree_view, void* iter, void* path), 3, tree_view, iter, path);
WRAPPER(GtkTreeView, row_expanded, void, (void* tree_view, void* iter, void* path), 3, tree_view, iter, path);
WRAPPER(GtkTreeView, row_collapsed, void, (void* tree_view, void* iter, void* path), 3, tree_view, iter, path);
WRAPPER(GtkTreeView, columns_changed, void, (void* tree_view), 1, tree_view);
WRAPPER(GtkTreeView, cursor_changed, void, (void* tree_view), 1, tree_view);
WRAPPER(GtkTreeView, move_cursor, int, (void* tree_view, int step, int count), 3, tree_view, step, count);
WRAPPER(GtkTreeView, select_all, int, (void* tree_view), 1, tree_view);
WRAPPER(GtkTreeView, unselect_all, int, (void* tree_view), 1, tree_view);
WRAPPER(GtkTreeView, select_cursor_row, int, (void* tree_view, int start_editing), 2, tree_view, start_editing);
WRAPPER(GtkTreeView, toggle_cursor_row, int, (void* tree_view), 1, tree_view);
WRAPPER(GtkTreeView, expand_collapse_cursor_row, int, (void* tree_view, int logical, int expand, int open_all), 4, tree_view, logical, expand, open_all);
WRAPPER(GtkTreeView, select_cursor_parent, int, (void* tree_view), 1, tree_view);
WRAPPER(GtkTreeView, start_interactive_search, int, (void* tree_view), 1, tree_view);

#define SUPERGO() \
    GO(set_scroll_adjustments, vFppp);      \
    GO(row_activated, vFppp);               \
    GO(test_expand_row, iFppp);             \
    GO(test_collapse_row, iFppp);           \
    GO(row_expanded, vFppp);                \
    GO(row_collapsed, vFppp);               \
    GO(columns_changed, vFp);               \
    GO(cursor_changed, vFp);                \
    GO(move_cursor, iFppp);                 \
    GO(select_all, iFp);                    \
    GO(unselect_all, iFp);                  \
    GO(select_cursor_row, iFpi);            \
    GO(toggle_cursor_row, iFp);             \
    GO(expand_collapse_cursor_row, iFpiii); \
    GO(select_cursor_parent, iFp);          \
    GO(start_interactive_search, iFp);      \

// wrap (so bridge all calls, just in case)
static void wrapGtkTreeViewClass(my_GtkTreeViewClass_t* class)
{
    wrapGtkContainerClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkTreeView (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkTreeViewClass(my_GtkTreeViewClass_t* class)
{   
    unwrapGtkContainerClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkTreeView (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkTreeViewClass(my_GtkTreeViewClass_t* class)
{
    bridgeGtkContainerClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkTreeView (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// ----- GtkBinClass ------

// wrap (so bridge all calls, just in case)
static void wrapGtkBinClass(my_GtkBinClass_t* class)
{
    wrapGtkContainerClass(&class->parent_class);
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkBinClass(my_GtkBinClass_t* class)
{   
    unwrapGtkContainerClass(&class->parent_class);
}
// autobridge
static void bridgeGtkBinClass(my_GtkBinClass_t* class)
{
    bridgeGtkContainerClass(&class->parent_class);
}

// ----- GtkWindowClass ------
// wrapper x64 -> natives of callbacks
WRAPPER(GtkWindow, set_focus, void, (void* window, void* focus), 2, window, focus);
WRAPPER(GtkWindow, frame_event, int, (void* window, void* event), 2, window, event);
WRAPPER(GtkWindow, activate_focus, void, (void* window), 1, window);
WRAPPER(GtkWindow, activate_default, void, (void* window), 1, window);
WRAPPER(GtkWindow, move_focus, void, (void* window, int direction), 2, window, direction);
WRAPPER(GtkWindow, keys_changed, void, (void* window), 1, window);

#define SUPERGO()               \
    GO(set_focus, vFpp);        \
    GO(frame_event, iFpp);      \
    GO(activate_focus, vFp);    \
    GO(activate_default, vFp);  \
    GO(move_focus, vFpi);       \
    GO(keys_changed, vFp);      \


// wrap (so bridge all calls, just in case)
static void wrapGtkWindowClass(my_GtkWindowClass_t* class)
{
    wrapGtkBinClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkWindow (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkWindowClass(my_GtkWindowClass_t* class)
{   
    unwrapGtkBinClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkWindow (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkWindowClass(my_GtkWindowClass_t* class)
{
    bridgeGtkBinClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkWindow (W, class->A)
    SUPERGO()
    #undef GO
}

// ----- GtkTableClass ------

// wrap (so bridge all calls, just in case)
static void wrapGtkTableClass(my_GtkTableClass_t* class)
{
    wrapGtkContainerClass(&class->parent_class);
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkTableClass(my_GtkTableClass_t* class)
{   
    unwrapGtkContainerClass(&class->parent_class);
}
// autobridge
static void bridgeGtkTableClass(my_GtkTableClass_t* class)
{
    bridgeGtkContainerClass(&class->parent_class);
}

// ----- GtkFixedClass ------

// wrap (so bridge all calls, just in case)
static void wrapGtkFixedClass(my_GtkFixedClass_t* class)
{
    wrapGtkContainerClass(&class->parent_class);
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkFixedClass(my_GtkFixedClass_t* class)
{   
    unwrapGtkContainerClass(&class->parent_class);
}
// autobridge
static void bridgeGtkFixedClass(my_GtkFixedClass_t* class)
{
    bridgeGtkContainerClass(&class->parent_class);
}

// ----- MetaFramesClass ------

// wrap (so bridge all calls, just in case)
static void wrapMetaFramesClass(my_MetaFramesClass_t* class)
{
    wrapGtkWindowClass(&class->parent_class);
}
// unwrap (and use callback if not a native call anymore)
static void unwrapMetaFramesClass(my_MetaFramesClass_t* class)
{   
    unwrapGtkWindowClass(&class->parent_class);
}
// autobridge
static void bridgeMetaFramesClass(my_MetaFramesClass_t* class)
{
    bridgeGtkWindowClass(&class->parent_class);
}

#undef SUPERGO

// ----- GtkButtonClass ------
// wrapper x86 -> natives of callbacks
WRAPPER(GtkButton, pressed, void,  (void* button), 1, button);
WRAPPER(GtkButton, released, void, (void* button), 1, button);
WRAPPER(GtkButton, clicked, void,  (void* button), 1, button);
WRAPPER(GtkButton, enter, void,    (void* button), 1, button);
WRAPPER(GtkButton, leave, void,    (void* button), 1, button);
WRAPPER(GtkButton, activate, void, (void* button), 1, button);

#define SUPERGO()               \
    GO(pressed, vFp);           \
    GO(released, vFp);          \
    GO(clicked, vFp);           \
    GO(enter, vFp);             \
    GO(leave, vFp);             \
    GO(activate, vFp);          \


// wrap (so bridge all calls, just in case)
static void wrapGtkButtonClass(my_GtkButtonClass_t* class)
{
    wrapGtkBinClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkButton (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkButtonClass(my_GtkButtonClass_t* class)
{   
    unwrapGtkBinClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkButton (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkButtonClass(my_GtkButtonClass_t* class)
{
    bridgeGtkBinClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkButton (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// ----- GtkComboBoxClass ------
// wrapper x86 -> natives of callbacks
WRAPPER(GtkComboBox, changed, void, (void* combo_box), 1, combo_box);
WRAPPER(GtkComboBox, get_active_text, void*, (void* combo_box), 1, combo_box);

#define SUPERGO()               \
    GO(changed, vFp);           \
    GO(get_active_text, pFp);   \


// wrap (so bridge all calls, just in case)
static void wrapGtkComboBoxClass(my_GtkComboBoxClass_t* class)
{
    wrapGtkBinClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkComboBox (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkComboBoxClass(my_GtkComboBoxClass_t* class)
{   
    unwrapGtkBinClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkComboBox (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkComboBoxClass(my_GtkComboBoxClass_t* class)
{
    bridgeGtkBinClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkComboBox (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// ----- GtkToggleButtonClass ------
// wrapper x86 -> natives of callbacks
WRAPPER(GtkToggleButton, toggled, void, (void* toggle_button), 1, toggle_button);

#define SUPERGO()               \
    GO(toggled, vFp);           \


// wrap (so bridge all calls, just in case)
static void wrapGtkToggleButtonClass(my_GtkToggleButtonClass_t* class)
{
    wrapGtkButtonClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkToggleButton (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkToggleButtonClass(my_GtkToggleButtonClass_t* class)
{   
    unwrapGtkButtonClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkToggleButton (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkToggleButtonClass(my_GtkToggleButtonClass_t* class)
{
    bridgeGtkButtonClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkToggleButton (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// ----- GtkCheckButtonClass ------
// wrapper x86 -> natives of callbacks
WRAPPER(GtkCheckButton, draw_indicator, void, (void* check_button, void* area), 2, check_button, area);

#define SUPERGO()               \
    GO(draw_indicator, vFpp);   \


// wrap (so bridge all calls, just in case)
static void wrapGtkCheckButtonClass(my_GtkCheckButtonClass_t* class)
{
    wrapGtkToggleButtonClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkCheckButton (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkCheckButtonClass(my_GtkCheckButtonClass_t* class)
{   
    unwrapGtkToggleButtonClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkCheckButton (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkCheckButtonClass(my_GtkCheckButtonClass_t* class)
{
    bridgeGtkToggleButtonClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkCheckButton (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// ----- GtkEntryClass ------
// wrapper x86 -> natives of callbacks
WRAPPER(GtkEntry, populate_popup, void,     (void* entry, void* menu), 2, entry, menu);
WRAPPER(GtkEntry, activate, void,           (void* entry), 1, entry);
WRAPPER(GtkEntry, move_cursor, void,        (void* entry, int step, int count, int extend_selection), 4, entry, step, count, extend_selection);
WRAPPER(GtkEntry, insert_at_cursor, void,   (void* entry, void* str), 2, entry, str);
WRAPPER(GtkEntry, delete_from_cursor, void, (void* entry, int type, int count), 3, entry, type, count);
WRAPPER(GtkEntry, backspace, void,          (void* entry), 1, entry);
WRAPPER(GtkEntry, cut_clipboard, void,      (void* entry), 1, entry);
WRAPPER(GtkEntry, copy_clipboard, void,     (void* entry), 1, entry);
WRAPPER(GtkEntry, paste_clipboard, void,    (void* entry), 1, entry);
WRAPPER(GtkEntry, toggle_overwrite, void,   (void* entry), 1, entry);
WRAPPER(GtkEntry, get_text_area_size, void, (void* entry, void* x, void* y, void* width, void* height), 5, entry, x, y, width, height);

#define SUPERGO()                   \
    GO(populate_popup, vFpp);       \
    GO(activate, vFp);              \
    GO(move_cursor, vFpiii);        \
    GO(insert_at_cursor, vFp);      \
    GO(delete_from_cursor, vFpii);  \
    GO(backspace, vFp);             \
    GO(cut_clipboard, vFp);         \
    GO(copy_clipboard, vFp);        \
    GO(paste_clipboard, vFp);       \
    GO(toggle_overwrite, vFp);      \
    GO(get_text_area_size, vFppppp);\

// wrap (so bridge all calls, just in case)
static void wrapGtkEntryClass(my_GtkEntryClass_t* class)
{
    wrapGtkWidgetClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkEntry (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkEntryClass(my_GtkEntryClass_t* class)
{   
    unwrapGtkWidgetClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkEntry (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkEntryClass(my_GtkEntryClass_t* class)
{
    bridgeGtkWidgetClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkEntry (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// ----- GtkSpinButtonClass ------
// wrapper x86 -> natives of callbacks
WRAPPER(GtkSpinButton, input, int,  (void* spin_button, void* new_value), 2, spin_button, new_value);
WRAPPER(GtkSpinButton, output, int, (void* spin_button), 1, spin_button);
WRAPPER(GtkSpinButton, value_changed, void, (void* spin_button), 1, spin_button);
WRAPPER(GtkSpinButton, change_value, void, (void* spin_button, int scroll), 2, spin_button, scroll);
WRAPPER(GtkSpinButton, wrapped, void, (void* spin_button), 1, spin_button);

#define SUPERGO()           \
    GO(input, iFpp);        \
    GO(output, iFp);        \
    GO(value_changed, vFp); \
    GO(change_value, vFpi); \
    GO(wrapped, vFp);       \

// wrap (so bridge all calls, just in case)
static void wrapGtkSpinButtonClass(my_GtkSpinButtonClass_t* class)
{
    wrapGtkEntryClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkSpinButton (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkSpinButtonClass(my_GtkSpinButtonClass_t* class)
{   
    unwrapGtkEntryClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkSpinButton (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkSpinButtonClass(my_GtkSpinButtonClass_t* class)
{
    bridgeGtkEntryClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkSpinButton (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// ----- GtkProgressClass ------
// wrapper x86 -> natives of callbacks
WRAPPER(GtkProgress, paint, void,          (void* progress), 1, progress);
WRAPPER(GtkProgress, update, void,         (void* progress), 1, progress);
WRAPPER(GtkProgress, act_mode_enter, void, (void* progress), 1, progress);

#define SUPERGO()           \
    GO(paint, vFp);         \
    GO(update, vFp);        \
    GO(act_mode_enter, vFp);\

// wrap (so bridge all calls, just in case)
static void wrapGtkProgressClass(my_GtkProgressClass_t* class)
{
    wrapGtkWidgetClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkProgress (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkProgressClass(my_GtkProgressClass_t* class)
{   
    unwrapGtkWidgetClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkProgress (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkProgressClass(my_GtkProgressClass_t* class)
{
    bridgeGtkWidgetClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkProgress (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// ----- GtkProgressBarClass ------
// no wrapper x86 -> natives of callbacks

#define SUPERGO()           \

// wrap (so bridge all calls, just in case)
static void wrapGtkProgressBarClass(my_GtkProgressBarClass_t* class)
{
    wrapGtkProgressClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkProgressBar (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkProgressBarClass(my_GtkProgressBarClass_t* class)
{   
    unwrapGtkProgressClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkProgressBar (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkProgressBarClass(my_GtkProgressBarClass_t* class)
{
    bridgeGtkProgressClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkProgressBar (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// ----- GtkFrameClass ------
// wrapper x86 -> natives of callbacks
WRAPPER(GtkFrame, compute_child_allocation, void, (void* frame, void* allocation), 2, frame, allocation);

#define SUPERGO()                       \
    GO(compute_child_allocation, vFpp); \

// wrap (so bridge all calls, just in case)
static void wrapGtkFrameClass(my_GtkFrameClass_t* class)
{
    wrapGtkBinClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkFrame (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkFrameClass(my_GtkFrameClass_t* class)
{   
    unwrapGtkBinClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkFrame (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkFrameClass(my_GtkFrameClass_t* class)
{
    bridgeGtkBinClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkFrame (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// ----- GtkMenuShellClass ------
// wrapper x86 -> natives of callbacks
WRAPPER(GtkMenuShell,deactivate, void,      (void* menu_shell), 1, menu_shell);
WRAPPER(GtkMenuShell,selection_done, void,  (void* menu_shell), 1, menu_shell);
WRAPPER(GtkMenuShell,move_current, void,    (void* menu_shell, int direction), 2, menu_shell, direction);
WRAPPER(GtkMenuShell,activate_current, void,(void* menu_shell, int force_hide), 2, menu_shell, force_hide);
WRAPPER(GtkMenuShell,cancel, void,          (void* menu_shell), 1, menu_shell);
WRAPPER(GtkMenuShell,select_item, void,     (void* menu_shell, void* menu_item), 2, menu_shell, menu_item);
WRAPPER(GtkMenuShell,insert, void,          (void* menu_shell, void* child, int position), 3, menu_shell, child, position);
WRAPPER(GtkMenuShell,get_popup_delay, int,  (void* menu_shell), 1, menu_shell);
WRAPPER(GtkMenuShell,move_selected, int,    (void* menu_shell, int distance), 2, menu_shell, distance);

#define SUPERGO()               \
    GO(deactivate, vFp);        \
    GO(selection_done, vFp);    \
    GO(move_current, vFpi);     \
    GO(activate_current, vFpi); \
    GO(cancel, vFp);            \
    GO(select_item, vFpp);      \
    GO(insert, vFppi);          \
    GO(get_popup_delay, iFp);   \
    GO(move_selected, iFpi);    \

// wrap (so bridge all calls, just in case)
static void wrapGtkMenuShellClass(my_GtkMenuShellClass_t* class)
{
    wrapGtkContainerClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkMenuShell (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkMenuShellClass(my_GtkMenuShellClass_t* class)
{   
    unwrapGtkContainerClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkMenuShell (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkMenuShellClass(my_GtkMenuShellClass_t* class)
{
    bridgeGtkContainerClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkMenuShell (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// ----- GtkMenuBarClass ------
// no wrapper x86 -> natives of callbacks

#define SUPERGO()           \

// wrap (so bridge all calls, just in case)
static void wrapGtkMenuBarClass(my_GtkMenuBarClass_t* class)
{
    wrapGtkMenuShellClass(&class->parent_class);
    #define GO(A, W) class->A = reverse_##A##_GtkMenuBar (W, class->A)
    SUPERGO()
    #undef GO
}
// unwrap (and use callback if not a native call anymore)
static void unwrapGtkMenuBarClass(my_GtkMenuBarClass_t* class)
{   
    unwrapGtkMenuShellClass(&class->parent_class);
    #define GO(A, W)   class->A = find_##A##_GtkMenuBar (class->A)
    SUPERGO()
    #undef GO
}
// autobridge
static void bridgeGtkMenuBarClass(my_GtkMenuBarClass_t* class)
{
    bridgeGtkMenuShellClass(&class->parent_class);
    #define GO(A, W) autobridge_##A##_GtkMenuBar (W, class->A)
    SUPERGO()
    #undef GO
}

#undef SUPERGO

// No more wrap/unwrap
#undef WRAPPER
#undef FIND
#undef REVERSE
#undef WRAPPED

// g_type_class_peek_parent
static void wrapGTKClass(void* cl, size_t type)
{
    #define GTKCLASS(A)                             \
    if(type==my_##A)                                \
        wrap##A##Class((my_##A##Class_t*)cl);       \
    else 

    GTKCLASSES()
    {
        if(my_MetaFrames==-1 && !strcmp(g_type_name(type), "MetaFrames")) {
            my_MetaFrames = type;
            wrapMetaFramesClass((my_MetaFramesClass_t*)cl);
        } else
            printf_log(LOG_NONE, "Warning, Custom Class initializer with unknown class type %zu (%s)\n", type, g_type_name(type));
    }
    #undef GTKCLASS
}

static void unwrapGTKClass(void* cl, size_t type)
{
    #define GTKCLASS(A)                             \
    if(type==my_##A)                                \
        unwrap##A##Class((my_##A##Class_t*)cl);     \
    else 

    GTKCLASSES()
    {
        if(my_MetaFrames==-1 && !strcmp(g_type_name(type), "MetaFrames")) {
            my_MetaFrames = type;
            unwrapMetaFramesClass((my_MetaFramesClass_t*)cl);
        } else
            printf_log(LOG_NONE, "Warning, Custom Class initializer with unknown class type %zu (%s)\n", type, g_type_name(type));
    }
    #undef GTKCLASS
}

static void bridgeGTKClass(void* cl, size_t type)
{
    #define GTKCLASS(A)                             \
    if(type==my_##A)                                \
        bridge##A##Class((my_##A##Class_t*)cl);     \
    else 

    GTKCLASSES()
    {
        printf_log(LOG_NONE, "Warning, AutoBridge GTK Class with unknown class type %zu (%s)\n", type, g_type_name(type));
    }
    #undef GTKCLASS
}


typedef union my_GClassAll_s {
    #define GTKCLASS(A) my_##A##Class_t A;
    GTKCLASSES()
    #undef GTKCLASS
} my_GClassAll_t;

#define GO(A) \
static void* my_gclassall_ref_##A = NULL;   \
static my_GClassAll_t my_gclassall_##A;

SUPER()
#undef GO

void* unwrapCopyGTKClass(void* klass, size_t type)
{
    if(!klass) return klass;
    #define GO(A) if(klass == my_gclassall_ref_##A) return &my_gclassall_##A;
    SUPER()
    #undef GO
    // check if class is the exact type we know
    size_t sz = 0;
    #define GTKCLASS(A) if(type==my_##A) sz = sizeof(my_##A##Class_t); else
    GTKCLASSES()
    {
        printf_log(LOG_NONE, "Warning, unwrapCopyGTKClass called with unknown class type %zu (%s)\n", type, g_type_name(type));
        return klass;
    }
    #undef GTKCLASS
    my_GClassAll_t *newklass = NULL;
    #define GO(A) if(!newklass && !my_gclassall_ref_##A) {my_gclassall_ref_##A = klass; newklass = &my_gclassall_##A;}
    SUPER()
    #undef GO
    if(!newklass) {
        printf_log(LOG_NONE, "Warning: no more slot for unwrapCopyGTKClass\n");
        return klass;
    }
    memcpy(newklass, klass, sz);
    unwrapGTKClass(newklass, type);
    return newklass;
}

// gtk_type_class

#define GO(A) \
static void* my_gclassallu_ref_##A = NULL;   \
static my_GClassAll_t my_gclassallu_##A;

SUPER()
#undef GO
void* wrapCopyGTKClass(void* klass, size_t type)
{
    if(!klass) return klass;
    #define GO(A) if(klass == my_gclassallu_ref_##A) return &my_gclassallu_##A;
    SUPER()
    #undef GO
    // check if class is the exact type we know
    size_t sz = 0;
    #define GTKCLASS(A) if(type==my_##A) sz = sizeof(my_##A##Class_t); else
    GTKCLASSES()
    {
        if(my_MetaFrames==(size_t)-1 && !strcmp(g_type_name(type), "MetaFrames")) {
            my_MetaFrames = type;
            sz = sizeof(my_MetaFramesClass_t);
        }
        if(!sz) {
            printf_log(LOG_NONE, "Warning, wrapCopyGTKClass called with unknown class type %p (%s)\n", (void*)type, g_type_name(type));
            return klass;
        }
    }
    #undef GTKCLASS
    my_GClassAll_t *newklass = NULL;
    #define GO(A) if(!newklass && !my_gclassallu_ref_##A) {my_gclassallu_ref_##A = klass; newklass = &my_gclassallu_##A;}
    SUPER()
    #undef GO
    if(!newklass) {
        printf_log(LOG_NONE, "Warning: no more slot for wrapCopyGTKClass\n");
        return klass;
    }
    memcpy(newklass, klass, sz);
    wrapGTKClass(newklass, type);
    return newklass;
}

// ---- GTypeValueTable ----

// First the structure GTypeInfo statics, with paired x64 source pointer
#define GO(A) \
static my_GTypeValueTable_t     my_gtypevaluetable_##A = {0};   \
static my_GTypeValueTable_t   *ref_gtypevaluetable_##A = NULL;
SUPER()
#undef GO
// Then the static functions callback that may be used with the structure
// value_init ...
#define GO(A)   \
static uintptr_t my_value_init_fct_##A = 0;                 \
static void my_value_init_##A(void* a)                      \
{                                                           \
    RunFunction(my_context, my_value_init_fct_##A, 1, a);   \
}
SUPER()
#undef GO
static void* find_value_init_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_value_init_fct_##A == (uintptr_t)fct) return my_value_init_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_value_init_fct_##A == 0) {my_value_init_fct_##A = (uintptr_t)fct; return my_value_init_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo value_init callback\n");
    return NULL;
}
// value_free ...
#define GO(A)   \
static uintptr_t my_value_free_fct_##A = 0;                 \
static void my_value_free_##A(void* a)                      \
{                                                           \
    RunFunction(my_context, my_value_free_fct_##A, 1, a);   \
}
SUPER()
#undef GO
static void* find_value_free_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_value_free_fct_##A == (uintptr_t)fct) return my_value_free_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_value_free_fct_##A == 0) {my_value_free_fct_##A = (uintptr_t)fct; return my_value_free_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo value_free callback\n");
    return NULL;
}
// value_copy ...
#define GO(A)   \
static uintptr_t my_value_copy_fct_##A = 0;                 \
static void my_value_copy_##A(void* a, void* b)             \
{                                                           \
    RunFunction(my_context, my_value_copy_fct_##A, 2, a, b);\
}
SUPER()
#undef GO
static void* find_value_copy_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_value_copy_fct_##A == (uintptr_t)fct) return my_value_copy_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_value_copy_fct_##A == 0) {my_value_copy_fct_##A = (uintptr_t)fct; return my_value_copy_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo value_copy callback\n");
    return NULL;
}
// value_peek_pointer ...
#define GO(A)   \
static uintptr_t my_value_peek_pointer_fct_##A = 0;                             \
static void* my_value_peek_pointer_##A(void* a)                                 \
{                                                                               \
    return (void*)RunFunction(my_context, my_value_peek_pointer_fct_##A, 1, a); \
}
SUPER()
#undef GO
static void* find_value_peek_pointer_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_value_peek_pointer_fct_##A == (uintptr_t)fct) return my_value_peek_pointer_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_value_peek_pointer_fct_##A == 0) {my_value_peek_pointer_fct_##A = (uintptr_t)fct; return my_value_peek_pointer_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo value_peek_pointer callback\n");
    return NULL;
}
// collect_value ...
#define GO(A)   \
static uintptr_t my_collect_value_fct_##A = 0;                                      \
static void* my_collect_value_##A(void* a, uint32_t b, void* c, uint32_t d)         \
{                                                                                   \
    return (void*)RunFunction(my_context, my_collect_value_fct_##A, 4, a, b, c, d); \
}
SUPER()
#undef GO
static void* find_collect_value_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_collect_value_fct_##A == (uintptr_t)fct) return my_collect_value_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_collect_value_fct_##A == 0) {my_collect_value_fct_##A = (uintptr_t)fct; return my_collect_value_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo collect_value callback\n");
    return NULL;
}
// lcopy_value ...
#define GO(A)   \
static uintptr_t my_lcopy_value_fct_##A = 0;                                      \
static void* my_lcopy_value_##A(void* a, uint32_t b, void* c, uint32_t d)         \
{                                                                                   \
    return (void*)RunFunction(my_context, my_lcopy_value_fct_##A, 4, a, b, c, d); \
}
SUPER()
#undef GO
static void* find_lcopy_value_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_lcopy_value_fct_##A == (uintptr_t)fct) return my_lcopy_value_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_lcopy_value_fct_##A == 0) {my_lcopy_value_fct_##A = (uintptr_t)fct; return my_lcopy_value_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo lcopy_value callback\n");
    return NULL;
}
// And now the get slot / assign... Taking into account that the desired callback may already be a wrapped one (so unwrapping it)
my_GTypeValueTable_t* findFreeGTypeValueTable(my_GTypeValueTable_t* fcts)
{
    if(!fcts) return fcts;
    #define GO(A) if(ref_gtypevaluetable_##A == fcts) return &my_gtypevaluetable_##A;
    SUPER()
    #undef GO
    #define GO(A) if(ref_gtypevaluetable_##A == 0) {                                                        \
        ref_gtypevaluetable_##A = fcts;                                                                     \
        my_gtypevaluetable_##A.value_init = find_value_init_Fct(fcts->value_init);                          \
        my_gtypevaluetable_##A.value_free = find_value_free_Fct(fcts->value_free);                          \
        my_gtypevaluetable_##A.value_copy = find_value_copy_Fct(fcts->value_copy);                          \
        my_gtypevaluetable_##A.value_peek_pointer = find_value_peek_pointer_Fct(fcts->value_peek_pointer);  \
        my_gtypevaluetable_##A.collect_format = fcts->collect_format;                                       \
        my_gtypevaluetable_##A.collect_value = find_collect_value_Fct(fcts->collect_value);                 \
        my_gtypevaluetable_##A.lcopy_format = fcts->lcopy_format;                                           \
        my_gtypevaluetable_##A.lcopy_value = find_lcopy_value_Fct(fcts->lcopy_value);                       \
        return &my_gtypevaluetable_##A;                                                                     \
    }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeValueTable callback\n");
    return NULL;
}

// signal2 ...
#define GO(A)   \
static uintptr_t my_signal2_fct_##A = 0;                                \
static void* my_signal2_##A(void* a, void* b)                           \
{                                                                       \
    return (void*)RunFunction(my_context, my_signal2_fct_##A, 2, a, b); \
}
SUPER()
#undef GO
static void* find_signal2_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_signal2_fct_##A == (uintptr_t)fct) return my_signal2_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_signal2_fct_##A == 0) {my_signal2_fct_##A = (uintptr_t)fct; return my_signal2_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo signal2 callback\n");
    return NULL;
}
// signal3 ...
#define GO(A)   \
static uintptr_t my_signal3_fct_##A = 0;                                    \
static void* my_signal3_##A(void* a, void* b, void* c)                      \
{                                                                           \
    return (void*)RunFunction(my_context, my_signal3_fct_##A, 3, a, b, c);  \
}
SUPER()
#undef GO
static void* find_signal3_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_signal3_fct_##A == (uintptr_t)fct) return my_signal3_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_signal3_fct_##A == 0) {my_signal3_fct_##A = (uintptr_t)fct; return my_signal3_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo signal3 callback\n");
    return NULL;
}
// signal4 ...
#define GO(A)   \
static uintptr_t my_signal4_fct_##A = 0;                                        \
static void* my_signal4_##A(void* a, void* b, void* c, void* d)                 \
{                                                                               \
    return (void*)RunFunction(my_context, my_signal4_fct_##A, 4, a, b, c, d);   \
}
SUPER()
#undef GO
static void* find_signal4_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_signal4_fct_##A == (uintptr_t)fct) return my_signal4_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_signal4_fct_##A == 0) {my_signal4_fct_##A = (uintptr_t)fct; return my_signal4_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo signal4 callback\n");
    return NULL;
}
// signal5 ...
#define GO(A)   \
static uintptr_t my_signal5_fct_##A = 0;                                        \
static void* my_signal5_##A(void* a, void* b, void* c, void* d, void* e)        \
{                                                                               \
    return (void*)RunFunction(my_context, my_signal5_fct_##A, 5, a, b, c, d, e);\
}
SUPER()
#undef GO
static void* find_signal5_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_signal5_fct_##A == (uintptr_t)fct) return my_signal5_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_signal5_fct_##A == 0) {my_signal5_fct_##A = (uintptr_t)fct; return my_signal5_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo signal5 callback\n");
    return NULL;
}
// signal6 ...
#define GO(A)   \
static uintptr_t my_signal6_fct_##A = 0;                                            \
static void* my_signal6_##A(void* a, void* b, void* c, void* d, void* e, void* f)   \
{                                                                                   \
    return (void*)RunFunction(my_context, my_signal6_fct_##A, 6, a, b, c, d, e, f); \
}
SUPER()
#undef GO
static void* find_signal6_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_signal6_fct_##A == (uintptr_t)fct) return my_signal6_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_signal6_fct_##A == 0) {my_signal6_fct_##A = (uintptr_t)fct; return my_signal6_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo signal6 callback\n");
    return NULL;
}
// signal7 ...
#define GO(A)   \
static uintptr_t my_signal7_fct_##A = 0;                                            \
static void* my_signal7_##A(void* a, void* b, void* c, void* d, void* e, void* f, void* g)  \
{                                                                                           \
    return (void*)RunFunction(my_context, my_signal7_fct_##A, 7, a, b, c, d, e, f, g);      \
}
SUPER()
#undef GO
static void* find_signal7_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_signal7_fct_##A == (uintptr_t)fct) return my_signal7_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_signal7_fct_##A == 0) {my_signal7_fct_##A = (uintptr_t)fct; return my_signal7_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo signal7 callback\n");
    return NULL;
}
static const wrapper_t wrappers[] = {pFpp, pFppp, pFpppp, pFpppp, pFppppp, pFpppppp};
typedef void* (*finder_t)(void*);
static const finder_t finders[] = {find_signal2_Fct, find_signal3_Fct, find_signal4_Fct, find_signal5_Fct, find_signal6_Fct, find_signal7_Fct};
#define MAX_SIGNAL_N 7

// ---- GTypeInfo ----
// let's handle signal with offset, that are used to wrap custom signal function
void my_unwrap_signal_offset(void* klass);
void my_add_signal_offset(size_t itype, uint32_t offset, int n)
{
    printf_log(LOG_DEBUG, "my_add_signal_offset(0x%zx, %d, %d)\n", itype, offset, n);
    if(!offset || !itype) // no offset means no overload...
        return;
    if(n<0 || n>MAX_SIGNAL_N) {
        printf_log(LOG_NONE, "Warning, signal with too many args (%d) in my_add_signal_offset\n", n);
        return;
    }
    int ret;
    khint_t k = kh_put(sigoffset, my_sigoffset, itype, &ret);
    sigoffset_array_t *p = &kh_value(my_sigoffset, k);
    if(ret) {
        p->a = NULL; p->cap = 0; p->sz = 0;
    }
    // check if offset already there
    for(int i=0; i<p->sz; ++i)
        if(p->a[i].offset == offset) {
            printf_log(LOG_INFO, "Offset already there... Bye\n");
            return; // already there, bye
        }
    if(p->sz==p->cap) {
        p->cap+=4;
        p->a = (sigoffset_t*)realloc(p->a, sizeof(sigoffset_t)*p->cap);
    }
    p->a[p->sz].offset = offset;
    p->a[p->sz++].n = n;
}
void my_unwrap_signal_offset(void* klass)
{
    if(!klass)
        return;
    size_t itype = *(size_t*)klass;
    khint_t k = kh_get(sigoffset, my_sigoffset, itype);
    if(k==kh_end(my_sigoffset))
        return;
    sigoffset_array_t *p = &kh_value(my_sigoffset, k);
    printf_log(LOG_DEBUG, "my_unwrap_signal_offset(%p) type=0x%zx with %d signals with offset\n", klass, itype, p->sz);
    for(int i=0; i<p->sz; ++i) {
        void** f = (void**)((uintptr_t)klass + p->a[i].offset);
        if(!GetNativeFnc((uintptr_t)*f)) {
            // Not a native function: autobridge it
            void* new_f = finders[p->a[i].n-2](f);
            printf_log(LOG_DEBUG, "Unwrapping %p -> %p (with alternate)\n", *f, new_f);
            if(!hasAlternate(new_f))
                addAlternate(new_f, *f);
            *f = new_f;
        }
    }
}

// First the structure my_GTypeInfo_t statics, with paired x64 source pointer
#define GO(A) \
static my_GTypeInfo_t     my_gtypeinfo_##A = {0};   \
static my_GTypeInfo_t    ref_gtypeinfo_##A = {0};   \
static int              used_gtypeinfo_##A = 0;
SUPER()
#undef GO
// Then the static functions callback that may be used with the structure
// base_init ...
#define GO(A)   \
static uintptr_t my_base_init_fct_##A = 0;                          \
static int my_base_init_##A(void* a)                                \
{                                                                   \
    return RunFunction(my_context, my_base_init_fct_##A, 1, a);     \
}
SUPER()
#undef GO
static void* find_base_init_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_base_init_fct_##A == (uintptr_t)fct) return my_base_init_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_base_init_fct_##A == 0) {my_base_init_fct_##A = (uintptr_t)fct; return my_base_init_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo base_init callback\n");
    return NULL;
}
// base_finalize ...
#define GO(A)   \
static uintptr_t my_base_finalize_fct_##A = 0;                      \
static int my_base_finalize_##A(void* a)                            \
{                                                                   \
    return RunFunction(my_context, my_base_finalize_fct_##A, 1, a); \
}
SUPER()
#undef GO
static void* find_base_finalize_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_base_finalize_fct_##A == (uintptr_t)fct) return my_base_finalize_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_base_finalize_fct_##A == 0) {my_base_finalize_fct_##A = (uintptr_t)fct; return my_base_finalize_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo base_finalize callback\n");
    return NULL;
}
// class_init ...
#define GO(A)   \
static uintptr_t my_class_init_fct_##A = 0;                             \
static size_t parent_class_init_##A = 0;                                \
static int my_class_init_##A(void* a, void* b)                          \
{                                                                       \
    printf_log(LOG_DEBUG, "Custom Class init %d for class %p (parent=%p)\n", A, a, (void*)parent_class_init_##A);\
    int ret = RunFunction(my_context, my_class_init_fct_##A, 2, a, b);  \
    unwrapGTKClass(a, parent_class_init_##A);                           \
    bridgeGTKClass(a, parent_class_init_##A);                           \
    my_unwrap_signal_offset(a);                                         \
    return ret;                                                         \
}
SUPER()
#undef GO
static void* find_class_init_Fct(void* fct, size_t parent)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_class_init_fct_##A == (uintptr_t)fct && parent_class_init_##A==parent) return my_class_init_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_class_init_fct_##A == 0) {my_class_init_fct_##A = (uintptr_t)fct; parent_class_init_##A=parent; return my_class_init_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo class_init callback\n");
    return NULL;
}
// class_finalize ...
#define GO(A)   \
static uintptr_t my_class_finalize_fct_##A = 0;                         \
static int my_class_finalize_##A(void* a, void* b)                      \
{                                                                       \
    return RunFunction(my_context, my_class_finalize_fct_##A, 2, a, b); \
}
SUPER()
#undef GO
static void* find_class_finalize_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_class_finalize_fct_##A == (uintptr_t)fct) return my_class_finalize_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_class_finalize_fct_##A == 0) {my_class_finalize_fct_##A = (uintptr_t)fct; return my_class_finalize_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo class_finalize callback\n");
    return NULL;
}
// instance_init ...
#define GO(A)   \
static uintptr_t my_instance_init_fct_##A = 0;                         \
static int my_instance_init_##A(void* a, void* b)                      \
{                                                                      \
    return RunFunction(my_context, my_instance_init_fct_##A, 2, a, b); \
}
SUPER()
#undef GO
static void* find_instance_init_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_instance_init_fct_##A == (uintptr_t)fct) return my_instance_init_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_instance_init_fct_##A == 0) {my_instance_init_fct_##A = (uintptr_t)fct; return my_instance_init_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo instance_init callback\n");
    return NULL;
}
// And now the get slot / assign... Taking into account that the desired callback may already be a wrapped one (so unwrapping it)
my_GTypeInfo_t* findFreeGTypeInfo(my_GTypeInfo_t* fcts, size_t parent)
{
    if(!fcts) return NULL;
    #define GO(A) if(used_gtypeinfo_##A && memcmp(&ref_gtypeinfo_##A, fcts, sizeof(my_GTypeInfo_t))==0) return &my_gtypeinfo_##A;
    SUPER()
    #undef GO
    #define GO(A) if(used_gtypeinfo_##A == 0) {                                         \
        used_gtypeinfo_##A = 1;                                                         \
        memcpy(&ref_gtypeinfo_##A, fcts, sizeof(my_GTypeInfo_t));                       \
        my_gtypeinfo_##A.class_size = fcts->class_size;                                 \
        my_gtypeinfo_##A.base_init = find_base_init_Fct(fcts->base_init);               \
        my_gtypeinfo_##A.base_finalize = find_base_finalize_Fct(fcts->base_finalize);   \
        my_gtypeinfo_##A.class_init = find_class_init_Fct(fcts->class_init, parent);    \
        my_gtypeinfo_##A.class_finalize = find_class_finalize_Fct(fcts->class_finalize);\
        my_gtypeinfo_##A.class_data = fcts->class_data;                                 \
        my_gtypeinfo_##A.instance_size = fcts->instance_size;                           \
        my_gtypeinfo_##A.n_preallocs = fcts->n_preallocs;                               \
        my_gtypeinfo_##A.instance_init = find_instance_init_Fct(fcts->instance_init);   \
        my_gtypeinfo_##A.value_table = findFreeGTypeValueTable(fcts->value_table);      \
        return &my_gtypeinfo_##A;                                                       \
    }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GTypeInfo callback\n");
    return NULL;
}

// ---- GtkTypeInfo ----

// First the structure my_GtkTypeInfo_t statics, with paired x64 source pointer
#define GO(A) \
static my_GtkTypeInfo_t     my_gtktypeinfo_##A = {0};   \
static my_GtkTypeInfo_t    ref_gtktypeinfo_##A = {0};  \
static int                used_gtktypeinfo_##A = 0;
SUPER()
#undef GO
// Then the static functions callback that may be used with the structure
#define GO(A)   \
static int fct_gtk_parent_##A = 0 ;                 \
static uintptr_t fct_gtk_class_init_##A = 0;        \
static int my_gtk_class_init_##A(void* g_class) {   \
    printf_log(LOG_DEBUG, "Calling fct_gtk_class_init_" #A " wrapper\n");           \
    int ret = (int)RunFunction(my_context, fct_gtk_class_init_##A, 1, g_class);     \
    unwrapGTKClass(g_class, fct_gtk_parent_##A);                                    \
    bridgeGTKClass(g_class, fct_gtk_parent_##A);                                    \
    return ret;                                                                     \
}   \
static uintptr_t fct_gtk_object_init_##A = 0;  \
static int my_gtk_object_init_##A(void* object, void* data) {   \
    printf_log(LOG_DEBUG, "Calling fct_gtk_object_init_" #A " wrapper\n");             \
    return (int)RunFunction(my_context, fct_gtk_object_init_##A, 2, object, data);    \
}   \
static uintptr_t fct_gtk_base_class_init_##A = 0;  \
static int my_gtk_base_class_init_##A(void* instance, void* data) {   \
    printf_log(LOG_DEBUG, "Calling fct_gtk_base_class_init_" #A " wrapper\n");             \
    return (int)RunFunction(my_context, fct_gtk_base_class_init_##A, 2, instance, data);    \
}

SUPER()
#undef GO
// And now the get slot / assign... Taking into account that the desired callback may already be a wrapped one (so unwrapping it)
my_GtkTypeInfo_t* findFreeGtkTypeInfo(my_GtkTypeInfo_t* fcts, size_t parent)
{
    if(!fcts) return NULL;
    #define GO(A) if(used_gtktypeinfo_##A && memcmp(&ref_gtktypeinfo_##A, fcts, sizeof(my_GtkTypeInfo_t))==0) return &my_gtktypeinfo_##A;
    SUPER()
    #undef GO
    #define GO(A) if(used_gtktypeinfo_##A == 0) {          \
        memcpy(&ref_gtktypeinfo_##A, fcts, sizeof(my_GtkTypeInfo_t));        \
        fct_gtk_parent_##A = parent;                        \
        my_gtktypeinfo_##A.type_name = fcts->type_name; \
        my_gtktypeinfo_##A.object_size = fcts->object_size; \
        my_gtktypeinfo_##A.class_size = fcts->class_size; \
        my_gtktypeinfo_##A.class_init_func = (fcts->class_init_func)?((GetNativeFnc((uintptr_t)fcts->class_init_func))?GetNativeFnc((uintptr_t)fcts->class_init_func):my_gtk_class_init_##A):NULL;    \
        fct_gtk_class_init_##A = (uintptr_t)fcts->class_init_func;           \
        my_gtktypeinfo_##A.object_init_func = (fcts->object_init_func)?((GetNativeFnc((uintptr_t)fcts->object_init_func))?GetNativeFnc((uintptr_t)fcts->object_init_func):my_gtk_object_init_##A):NULL;    \
        fct_gtk_object_init_##A = (uintptr_t)fcts->object_init_func;         \
        my_gtktypeinfo_##A.reserved_1 = fcts->reserved_1;                 \
        my_gtktypeinfo_##A.reserved_2 = fcts->reserved_2;                 \
        my_gtktypeinfo_##A.base_class_init_func = (fcts->base_class_init_func)?((GetNativeFnc((uintptr_t)fcts->base_class_init_func))?GetNativeFnc((uintptr_t)fcts->base_class_init_func):my_gtk_base_class_init_##A):NULL;    \
        fct_gtk_base_class_init_##A = (uintptr_t)fcts->base_class_init_func;   \
        return &my_gtktypeinfo_##A;                       \
    }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for GtkTypeInfo callback\n");
    return NULL;
}

#undef SUPER

void InitGTKClass(bridge_t *bridge)
{
    my_bridge  = bridge;
    my_signalmap = kh_init(signalmap);
    my_sigoffset = kh_init(sigoffset);
}

void FiniGTKClass()
{
    if(my_signalmap) {
        /*khint_t k;
        kh_foreach_key(my_signalmap, k,
            my_signal_t* p = (my_signal_t*)(uintptr_t)k;
            free(p);
        );*/ // lets assume all signals data is freed by gtk already
        kh_destroy(signalmap, my_signalmap);
        my_signalmap = NULL;
    }
    if(my_sigoffset) {
        sigoffset_array_t* p;
        kh_foreach_value_ref(my_sigoffset, p,
            free(p->a);
        );
        kh_destroy(sigoffset, my_sigoffset);
        my_sigoffset = NULL;
    }
}

#define GTKCLASS(A)             \
void Set##A##ID(size_t id)      \
{                               \
    my_##A = id;                \
}
GTKCLASSES()
#undef GTKCLASS

void AutoBridgeGtk(void*(*ref)(size_t), void(*unref)(void*))
{
    void* p;
    #define GTKCLASS(A)             \
    if(my_##A && my_##A!=-1) {      \
        p = ref(my_##A);            \
        bridgeGTKClass(p, my_##A);  \
        unref(p);                   \
    }
    GTKCLASSES()
    #undef GTKCLASS
}

void SetGTypeName(void* f)
{
    g_type_name = f;
}

my_signal_t* new_mysignal(void* f, void* data, void* destroy)
{
    my_signal_t* sig = (my_signal_t*)calloc(1, sizeof(my_signal_t));
    sig->sign = SIGN;
    sig->c_handler = (uintptr_t)f;
    sig->destroy = (uintptr_t)destroy;
    sig->data = data;
    int ret;
    kh_put(signalmap, my_signalmap, (uintptr_t)sig, &ret);
    return sig;
}
void my_signal_delete(my_signal_t* sig)
{
    khint_t k = kh_get(signalmap, my_signalmap, (uintptr_t)sig);
    if(k!=kh_end(my_signalmap)) {
        kh_del(signalmap, my_signalmap, k);
    } else {
        printf_log(LOG_NONE, "Warning, my_signal_delete called with an unrefereced signal!\n");
    }
    uintptr_t d = sig->destroy;
    if(d) {
        RunFunction(my_context, d, 1, sig->data);
    }
    printf_log(LOG_DEBUG, "gtk Data deleted, sig=%p, data=%p, destroy=%p\n", sig, sig->data, (void*)d);
    free(sig);
}
int my_signal_is_valid(void* sig)
{
    khint_t k = kh_get(signalmap, my_signalmap, (uintptr_t)sig);
    if(k!=kh_end(my_signalmap)) {
        /*if(((my_signal_t*)c)->sign == SIGN)
            return 1;
        else
            printf_log(LOG_NONE, "Warning, incohrent my_signal_t structure referenced\n");*/
        return 1;
    }
    return 0;
}

int my_signal_cb(void* a, void* b, void* c, void* d)
{
    // signal can have many signature... so first job is to find the data!
    // hopefully, no callback have more than 4 arguments...
    my_signal_t* sig = NULL;
    int i = 0;
    if(my_signal_is_valid(a)) {
        sig = (my_signal_t*)a;
        i = 1;
    }
    if(!sig && my_signal_is_valid(b)) {
        sig = (my_signal_t*)b;
        i = 2;
    }
    if(!sig && my_signal_is_valid(c)) {
        sig = (my_signal_t*)c;
        i = 3;
    }
    if(!sig && my_signal_is_valid(d)) {
        sig = (my_signal_t*)d;
        i = 4;
    }
    printf_log(LOG_DEBUG, "gtk Signal called, sig=%p, NArgs=%d\n", sig, i);
    switch(i) {
        case 1: return (int)RunFunction(my_context, sig->c_handler, 1, sig->data);
        case 2: return (int)RunFunction(my_context, sig->c_handler, 2, a, sig->data);
        case 3: return (int)RunFunction(my_context, sig->c_handler, 3, a, b, sig->data);
        case 4: return (int)RunFunction(my_context, sig->c_handler, 4, a, b, c, sig->data);
    }
    printf_log(LOG_NONE, "Warning, Gtk signal callback but no data found!");
    return 0;
}
