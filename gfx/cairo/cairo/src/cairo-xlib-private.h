



































#ifndef CAIRO_XLIB_PRIVATE_H
#define CAIRO_XLIB_PRIVATE_H

#include "cairo-xlib.h"
#include "cairo-xlib-xrender-private.h"

#include "cairo-compiler-private.h"
#include "cairo-device-private.h"
#include "cairo-freelist-type-private.h"
#include "cairo-list-private.h"
#include "cairo-reference-count-private.h"
#include "cairo-types-private.h"

typedef struct _cairo_xlib_display cairo_xlib_display_t;
typedef struct _cairo_xlib_screen cairo_xlib_screen_t;

typedef struct _cairo_xlib_hook cairo_xlib_hook_t;
typedef struct _cairo_xlib_job cairo_xlib_job_t;
typedef void (*cairo_xlib_notify_func) (Display *, void *);
typedef void (*cairo_xlib_notify_resource_func) (Display *, XID);

struct _cairo_xlib_hook {
    cairo_xlib_hook_t *prev, *next; 
    void (*func) (cairo_xlib_display_t *display, void *data);
};


#define CUBE_SIZE 6

#define RAMP_SIZE 16

struct _cairo_xlib_display {
    cairo_device_t base;

    cairo_xlib_display_t *next;

    Display *display;
    cairo_list_t screens;

    int render_major;
    int render_minor;
    XRenderPictFormat *cached_xrender_formats[CAIRO_FORMAT_RGB16_565 + 1];

    cairo_xlib_job_t *workqueue;
    cairo_freelist_t wq_freelist;

    cairo_xlib_hook_t *close_display_hooks;
    unsigned int buggy_gradients :1;
    unsigned int buggy_pad_reflect :1;
    unsigned int buggy_repeat :1;
    unsigned int closed :1;
};

typedef struct _cairo_xlib_visual_info {
    cairo_list_t link;
    VisualID visualid;
    struct { uint8_t a, r, g, b; } colors[256];
    uint8_t cube_to_pseudocolor[CUBE_SIZE][CUBE_SIZE][CUBE_SIZE];
    uint8_t field8_to_cube[256];
    int8_t  dither8_to_cube[256];
    uint8_t gray8_to_pseudocolor[256];
} cairo_xlib_visual_info_t;

struct _cairo_xlib_screen {
    cairo_list_t link;

    cairo_device_t *device;
    Screen *screen;

    cairo_bool_t has_font_options;
    cairo_font_options_t font_options;

    GC gc[4];
    cairo_atomic_int_t gc_depths; 

    cairo_list_t visuals;
};

cairo_private cairo_device_t *
_cairo_xlib_device_create (Display *display);

cairo_private cairo_xlib_screen_t *
_cairo_xlib_display_get_screen (cairo_xlib_display_t *display,
				Screen *screen);

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
cairo_private cairo_status_t
_cairo_xlib_display_acquire (cairo_device_t *device,
                             cairo_xlib_display_t **display);

cairo_private void
_cairo_xlib_display_get_xrender_version (cairo_xlib_display_t *display,
					 int *major, int *minor);

cairo_private cairo_bool_t
_cairo_xlib_display_has_repeat (cairo_device_t *device);

cairo_private cairo_bool_t
_cairo_xlib_display_has_reflect (cairo_device_t *device);

cairo_private cairo_bool_t
_cairo_xlib_display_has_gradients (cairo_device_t *device);

cairo_private XRenderPictFormat *
_cairo_xlib_display_get_xrender_format (cairo_xlib_display_t	*display,
	                                cairo_format_t		 format);

cairo_private cairo_status_t
_cairo_xlib_screen_get (Display *dpy,
			Screen *screen,
			cairo_xlib_screen_t **out);

cairo_private void
_cairo_xlib_screen_destroy (cairo_xlib_screen_t *info);

cairo_private void
_cairo_xlib_screen_close_display (cairo_xlib_display_t *display,
                                  cairo_xlib_screen_t *info);

cairo_private GC
_cairo_xlib_screen_get_gc (cairo_xlib_display_t *display,
                           cairo_xlib_screen_t *info,
			   int depth,
			   Drawable drawable);

cairo_private void
_cairo_xlib_screen_put_gc (cairo_xlib_display_t *display,
                           cairo_xlib_screen_t *info,
			   int depth,
			   GC gc);

cairo_private cairo_font_options_t *
_cairo_xlib_screen_get_font_options (cairo_xlib_screen_t *info);

cairo_private cairo_status_t
_cairo_xlib_screen_get_visual_info (cairo_xlib_display_t *display,
                                    cairo_xlib_screen_t *info,
				    Visual *visual,
				    cairo_xlib_visual_info_t **out);

cairo_private cairo_status_t
_cairo_xlib_visual_info_create (Display *dpy,
	                        int screen,
				VisualID visualid,
				cairo_xlib_visual_info_t **out);

cairo_private void
_cairo_xlib_visual_info_destroy (cairo_xlib_visual_info_t *info);

#endif 
