



































#include "cairoint.h"

static const cairo_font_options_t _cairo_font_options_nil = {
    CAIRO_ANTIALIAS_DEFAULT,
    CAIRO_SUBPIXEL_ORDER_DEFAULT,
    CAIRO_HINT_STYLE_DEFAULT,
    CAIRO_HINT_METRICS_DEFAULT
};







void
_cairo_font_options_init_default (cairo_font_options_t *options)
{
    if (options == (cairo_font_options_t *)&_cairo_font_options_nil)
	return;

    options->antialias = CAIRO_ANTIALIAS_DEFAULT;
    options->subpixel_order = CAIRO_SUBPIXEL_ORDER_DEFAULT;
    options->hint_style = CAIRO_HINT_STYLE_DEFAULT;
    options->hint_metrics = CAIRO_HINT_METRICS_DEFAULT;
}

void
_cairo_font_options_init_copy (cairo_font_options_t		*options,
			       const cairo_font_options_t	*other)
{
    options->antialias = other->antialias;
    options->subpixel_order = other->subpixel_order;
    options->hint_style = other->hint_style;
    options->hint_metrics = other->hint_metrics;
}













cairo_font_options_t *
cairo_font_options_create (void)
{
    cairo_font_options_t *options = malloc (sizeof (cairo_font_options_t));

    if (!options)
	return (cairo_font_options_t *)&_cairo_font_options_nil;

    _cairo_font_options_init_default (options);

    return options;
}
slim_hidden_def (cairo_font_options_create);














cairo_font_options_t *
cairo_font_options_copy (const cairo_font_options_t *original)
{
    cairo_font_options_t *options;

    if (original == &_cairo_font_options_nil)
	return (cairo_font_options_t *)&_cairo_font_options_nil;

    options = malloc (sizeof (cairo_font_options_t));
    if (!options)
	return (cairo_font_options_t *)&_cairo_font_options_nil;

    _cairo_font_options_init_copy (options, original);

    return options;
}








void
cairo_font_options_destroy (cairo_font_options_t *options)
{
    if (options == (cairo_font_options_t *)&_cairo_font_options_nil)
	return;

    free (options);
}
slim_hidden_def (cairo_font_options_destroy);










cairo_status_t
cairo_font_options_status (cairo_font_options_t *options)
{
    if (options == (cairo_font_options_t *)&_cairo_font_options_nil)
	return CAIRO_STATUS_NO_MEMORY;
    else
	return CAIRO_STATUS_SUCCESS;
}
slim_hidden_def (cairo_font_options_status);











void
cairo_font_options_merge (cairo_font_options_t       *options,
			  const cairo_font_options_t *other)
{
    if (options == (cairo_font_options_t *)&_cairo_font_options_nil)
	return;

    if (other->antialias != CAIRO_ANTIALIAS_DEFAULT)
	options->antialias = other->antialias;
    if (other->subpixel_order != CAIRO_SUBPIXEL_ORDER_DEFAULT)
	options->subpixel_order = other->subpixel_order;
    if (other->hint_style != CAIRO_HINT_STYLE_DEFAULT)
	options->hint_style = other->hint_style;
    if (other->hint_metrics != CAIRO_HINT_METRICS_DEFAULT)
	options->hint_metrics = other->hint_metrics;
}
slim_hidden_def (cairo_font_options_merge);










cairo_bool_t
cairo_font_options_equal (const cairo_font_options_t *options,
			  const cairo_font_options_t *other)
{
    return (options->antialias == other->antialias &&
	    options->subpixel_order == other->subpixel_order &&
	    options->hint_style == other->hint_style &&
	    options->hint_metrics == other->hint_metrics);
}
slim_hidden_def (cairo_font_options_equal);













unsigned long
cairo_font_options_hash (const cairo_font_options_t *options)
{
    return ((options->antialias) |
	    (options->subpixel_order << 4) |
	    (options->hint_style << 8) |
	    (options->hint_metrics << 16));
}
slim_hidden_def (cairo_font_options_hash);









void
cairo_font_options_set_antialias (cairo_font_options_t *options,
				  cairo_antialias_t     antialias)
{
    if (options == (cairo_font_options_t *)&_cairo_font_options_nil)
	return;

    options->antialias = antialias;
}
slim_hidden_def (cairo_font_options_set_antialias);









cairo_antialias_t
cairo_font_options_get_antialias (const cairo_font_options_t *options)
{
    return options->antialias;
}












void
cairo_font_options_set_subpixel_order (cairo_font_options_t   *options,
				       cairo_subpixel_order_t  subpixel_order)
{
    if (options == (cairo_font_options_t *)&_cairo_font_options_nil)
	return;

    options->subpixel_order = subpixel_order;
}
slim_hidden_def (cairo_font_options_set_subpixel_order);










cairo_subpixel_order_t
cairo_font_options_get_subpixel_order (const cairo_font_options_t *options)
{
    return options->subpixel_order;
}











void
cairo_font_options_set_hint_style (cairo_font_options_t *options,
				   cairo_hint_style_t    hint_style)
{
    if (options == (cairo_font_options_t *)&_cairo_font_options_nil)
	return;

    options->hint_style = hint_style;
}
slim_hidden_def (cairo_font_options_set_hint_style);










cairo_hint_style_t
cairo_font_options_get_hint_style (const cairo_font_options_t *options)
{
    return options->hint_style;
}











void
cairo_font_options_set_hint_metrics (cairo_font_options_t *options,
				     cairo_hint_metrics_t  hint_metrics)
{
    if (options == (cairo_font_options_t *)&_cairo_font_options_nil)
	return;

    options->hint_metrics = hint_metrics;
}
slim_hidden_def (cairo_font_options_set_hint_metrics);










cairo_hint_metrics_t
cairo_font_options_get_hint_metrics (const cairo_font_options_t *options)
{
    return options->hint_metrics;
}
