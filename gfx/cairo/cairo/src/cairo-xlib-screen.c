





















































#include "cairoint.h"

#include "cairo-xlib-private.h"
#include "cairo-xlib-xrender-private.h"

#include "cairo-xlib-surface-private.h"

#include <fontconfig/fontconfig.h>

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
    char *v, *e;

    v = XGetDefault (dpy, "Xft", option);
    if (v) {
#if CAIRO_HAS_FC_FONT
	if (FcNameConstant ((FcChar8 *) v, value))
	    return TRUE;
#endif

	*value = strtol (v, &e, 0);
	if (e != v)
	    return TRUE;
    }

    return FALSE;
}

#ifndef FC_RGBA_UNKNOWN
#define FC_RGBA_UNKNOWN	    0
#define FC_RGBA_RGB	    1
#define FC_RGBA_BGR	    2
#define FC_RGBA_VRGB	    3
#define FC_RGBA_VBGR	    4
#define FC_RGBA_NONE	    5
#endif

#ifndef FC_HINT_NONE
#define FC_HINT_NONE        0
#define FC_HINT_SLIGHT      1
#define FC_HINT_MEDIUM      2
#define FC_HINT_FULL        3
#endif


static void
_cairo_xlib_init_screen_font_options (Display *dpy,
				      cairo_xlib_screen_info_t *info)
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
    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&info->ref_count));

    _cairo_reference_count_inc (&info->ref_count);

    return info;
}

void
_cairo_xlib_screen_info_close_display (cairo_xlib_screen_info_t *info)
{
    cairo_xlib_visual_info_t **visuals;
    int i;

    CAIRO_MUTEX_LOCK (info->mutex);
    for (i = 0; i < ARRAY_LENGTH (info->gc); i++) {
	if (info->gc[i] != NULL) {
	    XFreeGC (info->display->display, info->gc[i]);
	    info->gc[i] = NULL;
	}
    }

    visuals = _cairo_array_index (&info->visuals, 0);
    for (i = 0; i < _cairo_array_num_elements (&info->visuals); i++)
	_cairo_xlib_visual_info_destroy (info->display->display, visuals[i]);
    _cairo_array_truncate (&info->visuals, 0);

    CAIRO_MUTEX_UNLOCK (info->mutex);
}

void
_cairo_xlib_screen_info_destroy (cairo_xlib_screen_info_t *info)
{
    cairo_xlib_screen_info_t **prev;
    cairo_xlib_screen_info_t *list;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&info->ref_count));

    if (! _cairo_reference_count_dec_and_test (&info->ref_count))
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

    _cairo_array_fini (&info->visuals);

    CAIRO_MUTEX_FINI (info->mutex);

    free (info);
}

cairo_status_t
_cairo_xlib_screen_info_get (cairo_xlib_display_t *display,
			     Screen *screen,
			     cairo_xlib_screen_info_t **out)
{
    cairo_xlib_screen_info_t *info = NULL, **prev;

    CAIRO_MUTEX_LOCK (display->mutex);
    if (display->closed) {
	CAIRO_MUTEX_UNLOCK (display->mutex);
	return _cairo_error (CAIRO_STATUS_SURFACE_FINISHED);
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
	if (unlikely (info == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	CAIRO_REFERENCE_COUNT_INIT (&info->ref_count, 2); 
	CAIRO_MUTEX_INIT (info->mutex);
	info->display = _cairo_xlib_display_reference (display);
	info->screen = screen;
	info->has_render = FALSE;
	info->has_font_options = FALSE;
	memset (info->gc, 0, sizeof (info->gc));
	info->gc_needs_clip_reset = 0;

	_cairo_array_init (&info->visuals,
			   sizeof (cairo_xlib_visual_info_t*));

	if (screen) {
	    Display *dpy = display->display;
	    int event_base, error_base;

	    info->has_render = (XRenderQueryExtension (dpy, &event_base, &error_base) &&
		    (XRenderFindVisualFormat (dpy, DefaultVisual (dpy, DefaultScreen (dpy))) != 0));
	}

	



	CAIRO_MUTEX_LOCK (display->mutex);
	info->next = display->screens;
	display->screens = info;
	CAIRO_MUTEX_UNLOCK (display->mutex);
    }

    *out = info;
    return CAIRO_STATUS_SUCCESS;
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
_cairo_xlib_screen_get_gc (cairo_xlib_screen_info_t *info,
			   int depth,
			   unsigned int *dirty)
{
    GC gc;
    cairo_bool_t needs_reset;

    depth = depth_to_index (depth);

    CAIRO_MUTEX_LOCK (info->mutex);
    gc = info->gc[depth];
    info->gc[depth] = NULL;
    needs_reset = info->gc_needs_clip_reset & (1 << depth);
    info->gc_needs_clip_reset &= ~(1 << depth);
    CAIRO_MUTEX_UNLOCK (info->mutex);

    if (needs_reset)
	*dirty |= CAIRO_XLIB_SURFACE_CLIP_DIRTY_GC;

    return gc;
}

cairo_status_t
_cairo_xlib_screen_put_gc (cairo_xlib_screen_info_t *info, int depth, GC gc, cairo_bool_t reset_clip)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    GC oldgc;

    depth = depth_to_index (depth);

    CAIRO_MUTEX_LOCK (info->mutex);
    oldgc = info->gc[depth];
    info->gc[depth] = gc;
    if (reset_clip)
	info->gc_needs_clip_reset |= 1 << depth;
    else
	info->gc_needs_clip_reset &= ~(1 << depth);
    CAIRO_MUTEX_UNLOCK (info->mutex);

    if (oldgc != NULL) {
	status = _cairo_xlib_display_queue_work (info->display,
		                               (cairo_xlib_notify_func) XFreeGC,
					       oldgc,
					       NULL);
    }

    return status;
}

cairo_status_t
_cairo_xlib_screen_get_visual_info (cairo_xlib_screen_info_t *info,
				    Visual *visual,
				    cairo_xlib_visual_info_t **out)
{
    Display *dpy = info->display->display;
    cairo_xlib_visual_info_t **visuals, *ret = NULL;
    cairo_status_t status;
    int i, n_visuals;

    CAIRO_MUTEX_LOCK (info->mutex);
    visuals = _cairo_array_index (&info->visuals, 0);
    n_visuals = _cairo_array_num_elements (&info->visuals);
    for (i = 0; i < n_visuals; i++) {
	if (visuals[i]->visualid == visual->visualid) {
	    ret = visuals[i];
	    break;
	}
    }
    CAIRO_MUTEX_UNLOCK (info->mutex);

    if (ret != NULL) {
	*out = ret;
	return CAIRO_STATUS_SUCCESS;
    }

    status = _cairo_xlib_visual_info_create (dpy,
					     XScreenNumberOfScreen (info->screen),
					     visual->visualid,
					     &ret);
    if (unlikely (status))
	return status;

    CAIRO_MUTEX_LOCK (info->mutex);
    if (n_visuals != _cairo_array_num_elements (&info->visuals)) {
	
	int new_visuals = _cairo_array_num_elements (&info->visuals);
	visuals = _cairo_array_index (&info->visuals, 0);
	for (i = n_visuals; i < new_visuals; i++) {
	    if (visuals[i]->visualid == visual->visualid) {
		_cairo_xlib_visual_info_destroy (dpy, ret);
		ret = visuals[i];
		break;
	    }
	}
	if (i == new_visuals)
	    status = _cairo_array_append (&info->visuals, &ret);
    } else
	status = _cairo_array_append (&info->visuals, &ret);
    CAIRO_MUTEX_UNLOCK (info->mutex);

    if (unlikely (status)) {
	_cairo_xlib_visual_info_destroy (dpy, ret);
	return status;
    }

    *out = ret;
    return CAIRO_STATUS_SUCCESS;
}

cairo_font_options_t *
_cairo_xlib_screen_get_font_options (cairo_xlib_screen_info_t *info)
{
    if (info->has_font_options)
	return &info->font_options;

    CAIRO_MUTEX_LOCK (info->mutex);
    if (! info->has_font_options) {
	Display *dpy = info->display->display;

	_cairo_font_options_init_default (&info->font_options);

	if (info->screen != NULL)
	    _cairo_xlib_init_screen_font_options (dpy, info);

	info->has_font_options = TRUE;
    }
    CAIRO_MUTEX_UNLOCK (info->mutex);

    return &info->font_options;
}
