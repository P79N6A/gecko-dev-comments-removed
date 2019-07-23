




















































#include <stdlib.h>
#include <string.h>

#include "cairo-xlib-private.h"

#include <fontconfig/fontconfig.h>

#include <X11/Xlibint.h>	
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
_cairo_xlib_init_screen_font_options (cairo_xlib_screen_info_t *info)
{
    cairo_bool_t xft_hinting;
    cairo_bool_t xft_antialias;
    int xft_hintstyle;
    int xft_rgba;
    cairo_antialias_t antialias;
    cairo_subpixel_order_t subpixel_order;
    cairo_hint_style_t hint_style;

    if (!get_boolean_default (info->display, "antialias", &xft_antialias))
	xft_antialias = TRUE;

    if (!get_boolean_default (info->display, "hinting", &xft_hinting))
	xft_hinting = TRUE;

    if (!get_integer_default (info->display, "hintstyle", &xft_hintstyle))
	xft_hintstyle = FC_HINT_FULL;

    if (!get_integer_default (info->display, "rgba", &xft_rgba))
    {
	xft_rgba = FC_RGBA_UNKNOWN;

#if RENDER_MAJOR > 0 || RENDER_MINOR >= 6
	if (info->has_render)
	{
	    int render_order = XRenderQuerySubpixelOrder (info->display,
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

    _cairo_font_options_init_default (&info->font_options);
    cairo_font_options_set_hint_style (&info->font_options, hint_style);
    cairo_font_options_set_antialias (&info->font_options, antialias);
    cairo_font_options_set_subpixel_order (&info->font_options, subpixel_order);
}

CAIRO_MUTEX_DECLARE(_xlib_screen_mutex);

static cairo_xlib_screen_info_t *_cairo_xlib_screen_list = NULL;




static int
_cairo_xlib_close_display (Display *dpy, XExtCodes *codes)
{
    cairo_xlib_screen_info_t *info, *prev;

    


    CAIRO_MUTEX_LOCK (_xlib_screen_mutex);

    prev = NULL;
    for (info = _cairo_xlib_screen_list; info; info = info->next) {
	if (info->display == dpy) {
	    if (prev)
		prev->next = info->next;
	    else
		_cairo_xlib_screen_list = info->next;
	    free (info);
	    break;
	}
	prev = info;
    }
    CAIRO_MUTEX_UNLOCK (_xlib_screen_mutex);

    

    return 0;
}

static void
_cairo_xlib_screen_info_reset (void)
{
    cairo_xlib_screen_info_t *info, *next;

    


    CAIRO_MUTEX_LOCK (_xlib_screen_mutex);

    for (info = _cairo_xlib_screen_list; info; info = next) {
	next = info->next;
	free (info);
    }

    _cairo_xlib_screen_list = NULL;

    CAIRO_MUTEX_UNLOCK (_xlib_screen_mutex);

}

cairo_xlib_screen_info_t *
_cairo_xlib_screen_info_get (Display *dpy, Screen *screen)
{
    cairo_xlib_screen_info_t *info;
    cairo_xlib_screen_info_t **prev;
    int event_base, error_base;
    XExtCodes *codes;
    cairo_bool_t seen_display = FALSE;

    






    CAIRO_MUTEX_LOCK (_xlib_screen_mutex);

    for (prev = &_cairo_xlib_screen_list; (info = *prev); prev = &(*prev)->next)
    {
	if (info->display == dpy) {
	    seen_display = TRUE;
	    if (info->screen == screen)
	    {
		


		if (prev != &_cairo_xlib_screen_list)
		{
		    *prev = info->next;
		    info->next = _cairo_xlib_screen_list;
		    _cairo_xlib_screen_list = info;
		}
		break;
	    }
	}
    }

    if (info)
	goto out;

    info = malloc (sizeof (cairo_xlib_screen_info_t));
    if (!info)
	goto out;

    if (!seen_display) {
	codes = XAddExtension (dpy);
	if (!codes) {
	    free (info);
	    info = NULL;
	    goto out;
	}

	XESetCloseDisplay (dpy, codes->extension, _cairo_xlib_close_display);
    }

    info->display = dpy;
    info->screen = screen;
    info->has_render = (XRenderQueryExtension (dpy, &event_base, &error_base) &&
			(XRenderFindVisualFormat (dpy, DefaultVisual (dpy, DefaultScreen (dpy))) != 0));

    _cairo_xlib_init_screen_font_options (info);

    info->next = _cairo_xlib_screen_list;
    _cairo_xlib_screen_list = info;

 out:
    CAIRO_MUTEX_UNLOCK (_xlib_screen_mutex);

    return info;
}

void
_cairo_xlib_screen_reset_static_data (void)
{
    _cairo_xlib_screen_info_reset ();

#if HAVE_XRMFINALIZE
    XrmFinalize ();
#endif

}
