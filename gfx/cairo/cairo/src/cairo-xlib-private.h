































#ifndef CAIRO_XLIB_PRIVATE_H
#define CAIRO_XLIB_PRIVATE_H

#include "cairoint.h"
#include "cairo-xlib.h"
#include "cairo-freelist-private.h"

typedef struct _cairo_xlib_display cairo_xlib_display_t;
typedef struct _cairo_xlib_hook cairo_xlib_hook_t;
typedef struct _cairo_xlib_job cairo_xlib_job_t;
typedef void (*cairo_xlib_notify_func) (Display *, void *);
typedef void (*cairo_xlib_notify_resource_func) (Display *, XID);

struct _cairo_xlib_hook {
    cairo_xlib_hook_t *next;
    void (*func) (Display *display, void *data);
    void *data;
    const void *key;
};

struct _cairo_xlib_display {
    cairo_xlib_display_t *next;
    unsigned int ref_count;
    cairo_mutex_t mutex;

    Display *display;
    cairo_xlib_screen_info_t *screens;

    cairo_xlib_job_t *workqueue;
    cairo_freelist_t wq_freelist;

    cairo_freelist_t hook_freelist;
    cairo_xlib_hook_t *close_display_hooks;
    unsigned int closed :1;
};

struct _cairo_xlib_screen_info {
    cairo_xlib_screen_info_t *next;
    unsigned int ref_count;

    cairo_xlib_display_t *display;
    Screen *screen;
    cairo_bool_t has_render;

    cairo_font_options_t font_options;

    GC gc[9];
    unsigned int gc_needs_clip_reset;
};

cairo_private cairo_xlib_display_t *
_cairo_xlib_display_get (Display *display);

cairo_private cairo_xlib_display_t *
_cairo_xlib_display_reference (cairo_xlib_display_t *info);
cairo_private void
_cairo_xlib_display_destroy (cairo_xlib_display_t *info);

cairo_private cairo_bool_t
_cairo_xlib_add_close_display_hook (Display *display, void (*func) (Display *, void *), void *data, const void *key);
cairo_private void
_cairo_xlib_remove_close_display_hooks (Display *display, const void *key);

cairo_private cairo_status_t
_cairo_xlib_display_queue_work (cairo_xlib_display_t *display,
	                        cairo_xlib_notify_func notify,
				void *data,
				void (*destroy)(void *));
cairo_private cairo_status_t
_cairo_xlib_display_queue_resource (cairo_xlib_display_t *display,
	                           cairo_xlib_notify_resource_func notify,
				   XID resource);
cairo_private void
_cairo_xlib_display_notify (cairo_xlib_display_t *display);

cairo_private cairo_xlib_screen_info_t *
_cairo_xlib_screen_info_get (Display *display, Screen *screen);

cairo_private cairo_xlib_screen_info_t *
_cairo_xlib_screen_info_reference (cairo_xlib_screen_info_t *info);
cairo_private void
_cairo_xlib_screen_info_destroy (cairo_xlib_screen_info_t *info);

cairo_private void
_cairo_xlib_screen_info_close_display (cairo_xlib_screen_info_t *info);

cairo_private GC
_cairo_xlib_screen_get_gc (cairo_xlib_screen_info_t *info, int depth);
cairo_private cairo_status_t
_cairo_xlib_screen_put_gc (cairo_xlib_screen_info_t *info, int depth, GC gc, cairo_bool_t reset_clip);

#if 1 

#include "cairo-xlib-xrender.h"
slim_hidden_proto (cairo_xlib_surface_create_with_xrender_format);

#endif

#endif 
