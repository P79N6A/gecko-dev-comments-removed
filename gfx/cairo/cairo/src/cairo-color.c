




































#include "cairoint.h"

static cairo_color_t const cairo_color_white = {
    1.0,    1.0,    1.0,    1.0,
    0xffff, 0xffff, 0xffff, 0xffff
};

static cairo_color_t const cairo_color_black = {
    0.0, 0.0, 0.0, 1.0,
    0x0, 0x0, 0x0, 0xffff
};

static cairo_color_t const cairo_color_transparent = {
    0.0, 0.0, 0.0, 0.0,
    0x0, 0x0, 0x0, 0x0
};

static cairo_color_t const cairo_color_magenta = {
    1.0,    0.0, 1.0,    1.0,
    0xffff, 0x0, 0xffff, 0xffff
};

const cairo_color_t *
_cairo_stock_color (cairo_stock_t stock)
{
    switch (stock) {
    case CAIRO_STOCK_WHITE:
	return &cairo_color_white;
    case CAIRO_STOCK_BLACK:
	return &cairo_color_black;
    case CAIRO_STOCK_TRANSPARENT:
	return &cairo_color_transparent;
    }

    ASSERT_NOT_REACHED;

    

    return &cairo_color_magenta;
}

void
_cairo_color_init (cairo_color_t *color)
{
    *color = cairo_color_white;
}

void
_cairo_color_init_rgb (cairo_color_t *color,
		       double red, double green, double blue)
{
    _cairo_color_init_rgba (color, red, green, blue, 1.0);
}







uint16_t
_cairo_color_double_to_short (double d)
{
    uint32_t i;
    i = (uint32_t) (d * 65536);
    i -= (i >> 16);
    return i;
}

static void
_cairo_color_compute_shorts (cairo_color_t *color)
{
    color->red_short   = _cairo_color_double_to_short (color->red   * color->alpha);
    color->green_short = _cairo_color_double_to_short (color->green * color->alpha);
    color->blue_short  = _cairo_color_double_to_short (color->blue  * color->alpha);
    color->alpha_short = _cairo_color_double_to_short (color->alpha);
}

void
_cairo_color_init_rgba (cairo_color_t *color,
			double red, double green, double blue,
			double alpha)
{
    color->red   = red;
    color->green = green;
    color->blue  = blue;
    color->alpha = alpha;

    _cairo_color_compute_shorts (color);
}

void
_cairo_color_multiply_alpha (cairo_color_t *color,
			     double	    alpha)
{
    color->alpha *= alpha;

    _cairo_color_compute_shorts (color);
}

void
_cairo_color_get_rgba (cairo_color_t *color,
		       double	     *red,
		       double	     *green,
		       double	     *blue,
		       double	     *alpha)
{
    *red   = color->red;
    *green = color->green;
    *blue  = color->blue;
    *alpha = color->alpha;
}

void
_cairo_color_get_rgba_premultiplied (cairo_color_t *color,
				     double	   *red,
				     double	   *green,
				     double	   *blue,
				     double	   *alpha)
{
    *red   = color->red   * color->alpha;
    *green = color->green * color->alpha;
    *blue  = color->blue  * color->alpha;
    *alpha = color->alpha;
}
