





















































#include "cairoint.h"

#include "cairo-xlib-private.h"

#include <fontconfig/fontconfig.h>

#include <X11/extensions/Xrender.h>

static int
parse_boolean (const char *v)
{
    char c0, c1;

    c0 = *v;
    if (c0 == 't' || c0 == 'T' || c0 == 'y' || c0 == 'Y' || c0 == '1')
	return 1;
    if (c0 == 'f' || c0 == 'F' || c0 == 'n' || c0 == 'N' || c0 == '0')
	return 0;
    if (c0 == 'o')
    {
	c1 = v[1];
	if (c1 == 'n' || c1 == 'N')
	    return 1;
	if (c1 == 'f' || c1 == 'F')
	    return 0;
    }

    return -1;
}

static cairo_bool_t
get_boolean_default (Display       *dpy,
		     const char    *option,
		     cairo_bool_t  *value)
{
    char *v;
    int i;

    v = XGetDefault (dpy, "Xft", option);
    if (v) {
	i = parse_boolean (v);
	if (i >= 0) {
	    *value = i;
	    return TRUE;
	}
    }

    return FALSE;
}

static cairo_bool_t
get_integer_default (Display    *dpy,
		     const char *option,
		     int        *value)
{
    int i;
    char *v, *e;

    v = XGetDefault (dpy, "Xft", option);
    if (v) {
	if (FcNameConstant ((FcChar8 *) v, value))
	    return TRUE;

	i = strtol (v, &e, 0);
	if (e != v)
	    return TRUE;
    }

    return FALSE;
}


#ifndef FC_HINT_NONE
#define FC_HINT_NONE        0
#define FC_HINT_SLIGHT      1
#define FC_HINT_MEDIUM      2
#define FC_HINT_FULL        3
#endif

static void
_cairo_xlib_init_screen_font_options (Display *dpy, cairo_xlib_screen_info_t *info)
{
    cairo_bool_t xft_hinting;
    cairo_bool_t xft_antialias;
    int xft_hintstyle;
    int xft_rgba;
    cairo_antialias_t antialias;
    cairo_subpixel_order_t subpixel_order;
    cairo_hint_style_t hint_style;

    if (!get_boolean_default (dpy, "antialias", &xft_antialias))
	xft_antialias = TRUE;

    if (!get_boolean_default (dpy, "hinting", &xft_hinting))
	xft_hinting = TRUE;

    if (!get_integer_default (dpy, "hintstyle", &xft_hintstyle))
	xft_hintstyle = FC_HINT_FULL;

    if (!get_integer_default (dpy, "rgba", &xft_rgba))
    {
	xft_rgba = FC_RGBA_UNKNOWN;

#if RENDER_MAJOR > 0 || RENDER_MINOR >= 6
	if (info->has_render)
	{
	    int render_order = XRenderQuerySubpixelOrder (dpy,
							  XScreenNumberOfScreen (info->screen));

	    switch (render_order)
	    {
	    default:
	    case SubPixelUnknown:
		xft_rgba = FC_RGBA_UNKNOWN;
		break;
	    case SubPixelHorizontalRGB:
		xft_rgba = FC_RGBA_RGB;
		break;
	    case SubPixelHorizontalBGR:
		xft_rgba = FC_RGBA_BGR;
		break;
	    case SubPixelVerticalRGB:
		xft_rgba = FC_RGBA_VRGB;
		break;
	    case SubPixelVerticalBGR:
		xft_rgba = FC_RGBA_VBGR;
		break;
	    case SubPixelNone:
		xft_rgba = FC_RGBA_NONE;
		break;
	    }
	}
#endif
    }

    if (xft_hinting) {
	switch (xft_hintstyle) {
	case FC_HINT_NONE:
	    hint_style = CAIRO_HINT_STYLE_NONE;
	    break;
	case FC_HINT_SLIGHT:
	    hint_style = CAIRO_HINT_STYLE_SLIGHT;
	    break;
	case FC_HINT_MEDIUM:
	    hint_style = CAIRO_HINT_STYLE_MEDIUM;
	    break;
	case FC_HINT_FULL:
	    hint_style = CAIRO_HINT_STYLE_FULL;
	    break;
	default:
	    hint_style = CAIRO_HINT_STYLE_DEFAULT;
	}
    } else {
	hint_style = CAIRO_HINT_STYLE_NONE;
    }

    switch (xft_rgba) {
    case FC_RGBA_RGB:
	subpixel_order = CAIRO_SUBPIXEL_ORDER_RGB;
	break;
    case FC_RGBA_BGR:
	subpixel_order = CAIRO_SUBPIXEL_ORDER_BGR;
	break;
    case FC_RGBA_VRGB:
	subpixel_order = CAIRO_SUBPIXEL_ORDER_VRGB;
	break;
    case FC_RGBA_VBGR:
	subpixel_order = CAIRO_SUBPIXEL_ORDER_VBGR;
	break;
    case FC_RGBA_UNKNOWN:
    case FC_RGBA_NONE:
    default:
	subpixel_order = CAIRO_SUBPIXEL_ORDER_DEFAULT;
    }

    if (xft_antialias) {
	if (subpixel_order == CAIRO_SUBPIXEL_ORDER_DEFAULT)
	    antialias = CAIRO_ANTIALIAS_GRAY;
	else
	    antialias = CAIRO_ANTIALIAS_SUBPIXEL;
    } else {
	antialias = CAIRO_ANTIALIAS_NONE;
    }

    cairo_font_options_set_hint_style (&info->font_options, hint_style);
    cairo_font_options_set_antialias (&info->font_options, antialias);
    cairo_font_options_set_subpixel_order (&info->font_options, subpixel_order);
    cairo_font_options_set_hint_metrics (&info->font_options, CAIRO_HINT_METRICS_ON);
}

cairo_xlib_screen_info_t *
_cairo_xlib_screen_info_reference (cairo_xlib_screen_info_t *info)
{
    if (info == NULL)
	return NULL;

    assert (info->ref_count > 0);
    info->ref_count++;

    return info;
}

void
_cairo_xlib_screen_info_close_display (cairo_xlib_screen_info_t *info)
{
    int i;

    for (i = 0; i < ARRAY_LENGTH (info->gc); i++) {
	if (info->gc[i] != NULL) {
	    XFreeGC (info->display->display, info->gc[i]);
	    info->gc[i] = NULL;
	}
    }
}

void
_cairo_xlib_screen_info_destroy (cairo_xlib_screen_info_t *info)
{
    cairo_xlib_screen_info_t **prev;
    cairo_xlib_screen_info_t *list;

    if (info == NULL)
	return;

    assert (info->ref_count > 0);
    if (--info->ref_count)
	return;

    CAIRO_MUTEX_LOCK (info->display->mutex);
    for (prev = &info->display->screens; (list = *prev); prev = &list->next) {
	if (list == info) {
	    *prev = info->next;
	    break;
	}
    }
    CAIRO_MUTEX_UNLOCK (info->display->mutex);

    _cairo_xlib_screen_info_close_display (info);

    _cairo_xlib_display_destroy (info->display);

    free (info);
}

cairo_xlib_screen_info_t *
_cairo_xlib_screen_info_get (Display *dpy, Screen *screen)
{
    cairo_xlib_display_t *display;
    cairo_xlib_screen_info_t *info = NULL, **prev;

    display = _cairo_xlib_display_get (dpy);
    if (display == NULL)
	return NULL;

    CAIRO_MUTEX_LOCK (display->mutex);
    if (display->closed) {
	CAIRO_MUTEX_UNLOCK (display->mutex);
	goto DONE;
    }

    for (prev = &display->screens; (info = *prev); prev = &(*prev)->next) {
	if (info->screen == screen) {
	    


	    if (prev != &display->screens) {
		*prev = info->next;
		info->next = display->screens;
		display->screens = info;
	    }
	    break;
	}
    }
    CAIRO_MUTEX_UNLOCK (display->mutex);

    if (info != NULL) {
	info = _cairo_xlib_screen_info_reference (info);
    } else {
	info = malloc (sizeof (cairo_xlib_screen_info_t));
	if (info != NULL) {
	    info->ref_count = 2; 
	    info->display = _cairo_xlib_display_reference (display);
	    info->screen = screen;
	    info->has_render = FALSE;
	    _cairo_font_options_init_default (&info->font_options);
	    memset (info->gc, 0, sizeof (info->gc));
	    info->gc_needs_clip_reset = 0;

	    if (screen) {
		int event_base, error_base;
		info->has_render = (XRenderQueryExtension (dpy, &event_base, &error_base) &&
			(XRenderFindVisualFormat (dpy, DefaultVisual (dpy, DefaultScreen (dpy))) != 0));
		_cairo_xlib_init_screen_font_options (dpy, info);
	    }

	    CAIRO_MUTEX_LOCK (display->mutex);
	    info->next = display->screens;
	    display->screens = info;
	    CAIRO_MUTEX_UNLOCK (display->mutex);
	}
    }

DONE:
    _cairo_xlib_display_destroy (display);

    return info;
}

static int
depth_to_index (int depth)
{
    switch(depth){
	case 1:  return 1;
	case 8:  return 2;
	case 12: return 3;
	case 15: return 4;
	case 16: return 5;
	case 24: return 6;
	case 30: return 7;
	case 32: return 8;
    }
    return 0;
}

GC
_cairo_xlib_screen_get_gc (cairo_xlib_screen_info_t *info, int depth)
{
    GC gc;

    depth = depth_to_index (depth);

    gc = info->gc[depth];
    info->gc[depth] = NULL;

    if (info->gc_needs_clip_reset & (1 << depth)) {
	XSetClipMask(info->display->display, gc, None);
	info->gc_needs_clip_reset &= ~(1 << depth);
    }

    return gc;
}

cairo_status_t
_cairo_xlib_screen_put_gc (cairo_xlib_screen_info_t *info, int depth, GC gc, cairo_bool_t reset_clip)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;

    depth = depth_to_index (depth);

    if (info->gc[depth] != NULL) {
	status = _cairo_xlib_display_queue_work (info->display,
		                               (cairo_xlib_notify_func) XFreeGC,
					       info->gc[depth],
					       NULL);
    }

    info->gc[depth] = gc;
    if (reset_clip)
	info->gc_needs_clip_reset |= 1 << depth;
    else
	info->gc_needs_clip_reset &= ~(1 << depth);

    return status;
}
