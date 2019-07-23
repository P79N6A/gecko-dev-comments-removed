



































#ifndef CAIRO_XLIB_PRIVATE_H
#define CAIRO_XLIB_PRIVATE_H

#include "cairo-xlib.h"
#include "cairo-xlib-xrender-private.h"

#include "cairo-compiler-private.h"
#include "cairo-freelist-private.h"
#include "cairo-mutex-private.h"
#include "cairo-reference-count-private.h"

typedef struct _cairo_xlib_display cairo_xlib_display_t;
typedef struct _cairo_xlib_hook cairo_xlib_hook_t;
typedef struct _cairo_xlib_job cairo_xlib_job_t;
typedef void (*cairo_xlib_notify_func) (Display *, void *);
typedef void (*cairo_xlib_notify_resource_func) (Display *, XID);

struct _cairo_xlib_hook {
    cairo_xlib_hook_t *prev, *next; 
    void (*func) (cairo_xlib_display_t *display, void *data);
};

struct _cairo_xlib_display {
    cairo_xlib_display_t *next;
    cairo_reference_count_t ref_count;
    cairo_mutex_t mutex;

    Display *display;
    cairo_xlib_screen_info_t *screens;

    int render_major;
    int render_minor;
    XRenderPictFormat *cached_xrender_formats[CAIRO_FORMAT_A1 + 1];

    cairo_xlib_job_t *workqueue;
    cairo_freelist_t wq_freelist;

    cairo_xlib_hook_t *close_display_hooks;
    unsigned int buggy_repeat :1;
    unsigned int buggy_pad_reflect :1;
    unsigned int closed :1;
};


#define CUBE_SIZE 6

#define RAMP_SIZE 16

typedef struct _cairo_xlib_visual_info {
    VisualID visualid;
    struct { uint8_t a, r, g, b; } colors[256];
    uint8_t cube_to_pseudocolor[CUBE_SIZE][CUBE_SIZE][CUBE_SIZE];
    uint8_t field8_to_cube[256];
    int8_t  dither8_to_cube[256];
    uint8_t gray8_to_pseudocolor[256];
} cairo_xlib_visual_info_t;

struct _cairo_xlib_screen_info {
    cairo_xlib_screen_info_t *next;
    cairo_reference_count_t ref_count;
    cairo_mutex_t mutex;

    cairo_xlib_display_t *display;
    Screen *screen;
    cairo_bool_t has_render;

    cairo_bool_t has_font_options;
    cairo_font_options_t font_options;

    GC gc[4];
    int gc_depths; 

    cairo_array_t visuals;
};

cairo_private cairo_status_t
_cairo_xlib_display_get (Display *display, cairo_xlib_display_t **out);

cairo_private cairo_xlib_display_t *
_cairo_xlib_display_reference (cairo_xlib_display_t *info);
cairo_private void
_cairo_xlib_display_destroy (cairo_xlib_display_t *info);

cairo_private void
_cairo_xlib_add_close_display_hook (cairo_xlib_display_t *display, cairo_xlib_hook_t *hook);

cairo_private void
_cairo_xlib_remove_close_display_hook (cairo_xlib_display_t *display, cairo_xlib_hook_t *hook);

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

cairo_private XRenderPictFormat *
_cairo_xlib_display_get_xrender_format (cairo_xlib_display_t	*display,
	                                cairo_format_t		 format);

cairo_private cairo_status_t
_cairo_xlib_screen_info_get (cairo_xlib_display_t *display,
			     Screen *screen,
			     cairo_xlib_screen_info_t **out);

cairo_private cairo_xlib_screen_info_t *
_cairo_xlib_screen_info_reference (cairo_xlib_screen_info_t *info);
cairo_private void
_cairo_xlib_screen_info_destroy (cairo_xlib_screen_info_t *info);

cairo_private void
_cairo_xlib_screen_info_close_display (cairo_xlib_screen_info_t *info);

cairo_private GC
_cairo_xlib_screen_get_gc (cairo_xlib_screen_info_t *info,
			   unsigned int depth,
			   Drawable drawable,
			   unsigned int *need_reset);

cairo_private void
_cairo_xlib_screen_put_gc (cairo_xlib_screen_info_t *info,
			   unsigned int depth,
			   GC gc,
			   cairo_bool_t reset_clip);

cairo_private cairo_font_options_t *
_cairo_xlib_screen_get_font_options (cairo_xlib_screen_info_t *info);

cairo_private cairo_status_t
_cairo_xlib_screen_get_visual_info (cairo_xlib_screen_info_t *info,
				    Visual *visual,
				    cairo_xlib_visual_info_t **out);

cairo_private cairo_status_t
_cairo_xlib_visual_info_create (Display *dpy,
	                        int screen,
				VisualID visualid,
				cairo_xlib_visual_info_t **out);

cairo_private void
_cairo_xlib_visual_info_destroy (Display *dpy, cairo_xlib_visual_info_t *info);

#endif 
