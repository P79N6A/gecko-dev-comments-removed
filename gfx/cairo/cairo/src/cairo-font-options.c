



































#include "cairoint.h"
#include "cairo-error-private.h"













static const cairo_font_options_t _cairo_font_options_nil = {
    CAIRO_ANTIALIAS_DEFAULT,
    CAIRO_SUBPIXEL_ORDER_DEFAULT,
    CAIRO_LCD_FILTER_DEFAULT,
    CAIRO_HINT_STYLE_DEFAULT,
    CAIRO_HINT_METRICS_DEFAULT,
    CAIRO_ROUND_GLYPH_POS_DEFAULT
};







void
_cairo_font_options_init_default (cairo_font_options_t *options)
{
    options->antialias = CAIRO_ANTIALIAS_DEFAULT;
    options->subpixel_order = CAIRO_SUBPIXEL_ORDER_DEFAULT;
    options->lcd_filter = CAIRO_LCD_FILTER_DEFAULT;
    options->hint_style = CAIRO_HINT_STYLE_DEFAULT;
    options->hint_metrics = CAIRO_HINT_METRICS_DEFAULT;
    options->round_glyph_positions = CAIRO_ROUND_GLYPH_POS_DEFAULT;
}

void
_cairo_font_options_init_copy (cairo_font_options_t		*options,
			       const cairo_font_options_t	*other)
{
    options->antialias = other->antialias;
    options->subpixel_order = other->subpixel_order;
    options->lcd_filter = other->lcd_filter;
    options->hint_style = other->hint_style;
    options->hint_metrics = other->hint_metrics;
    options->round_glyph_positions = other->round_glyph_positions;
}













cairo_font_options_t *
cairo_font_options_create (void)
{
    cairo_font_options_t *options;

    options = malloc (sizeof (cairo_font_options_t));
    if (!options) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_font_options_t *) &_cairo_font_options_nil;
    }

    _cairo_font_options_init_default (options);

    return options;
}














cairo_font_options_t *
cairo_font_options_copy (const cairo_font_options_t *original)
{
    cairo_font_options_t *options;

    if (cairo_font_options_status ((cairo_font_options_t *) original))
	return (cairo_font_options_t *) &_cairo_font_options_nil;

    options = malloc (sizeof (cairo_font_options_t));
    if (!options) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_font_options_t *) &_cairo_font_options_nil;
    }

    _cairo_font_options_init_copy (options, original);

    return options;
}








void
cairo_font_options_destroy (cairo_font_options_t *options)
{
    if (cairo_font_options_status (options))
	return;

    free (options);
}










cairo_status_t
cairo_font_options_status (cairo_font_options_t *options)
{
    if (options == NULL)
	return CAIRO_STATUS_NULL_POINTER;
    else if (options == (cairo_font_options_t *) &_cairo_font_options_nil)
	return CAIRO_STATUS_NO_MEMORY;
    else
	return CAIRO_STATUS_SUCCESS;
}
slim_hidden_def (cairo_font_options_status);











void
cairo_font_options_merge (cairo_font_options_t       *options,
			  const cairo_font_options_t *other)
{
    if (cairo_font_options_status (options))
	return;

    if (cairo_font_options_status ((cairo_font_options_t *) other))
	return;

    if (other->antialias != CAIRO_ANTIALIAS_DEFAULT)
	options->antialias = other->antialias;
    if (other->subpixel_order != CAIRO_SUBPIXEL_ORDER_DEFAULT)
	options->subpixel_order = other->subpixel_order;
    if (other->lcd_filter != CAIRO_LCD_FILTER_DEFAULT)
	options->lcd_filter = other->lcd_filter;
    if (other->hint_style != CAIRO_HINT_STYLE_DEFAULT)
	options->hint_style = other->hint_style;
    if (other->hint_metrics != CAIRO_HINT_METRICS_DEFAULT)
	options->hint_metrics = other->hint_metrics;
    if (other->round_glyph_positions != CAIRO_ROUND_GLYPH_POS_DEFAULT)
	options->round_glyph_positions = other->round_glyph_positions;
}
slim_hidden_def (cairo_font_options_merge);












cairo_bool_t
cairo_font_options_equal (const cairo_font_options_t *options,
			  const cairo_font_options_t *other)
{
    if (cairo_font_options_status ((cairo_font_options_t *) options))
	return FALSE;
    if (cairo_font_options_status ((cairo_font_options_t *) other))
	return FALSE;

    if (options == other)
	return TRUE;

    return (options->antialias == other->antialias &&
	    options->subpixel_order == other->subpixel_order &&
	    options->lcd_filter == other->lcd_filter &&
	    options->hint_style == other->hint_style &&
	    options->hint_metrics == other->hint_metrics &&
	    options->round_glyph_positions == other->round_glyph_positions);
}
slim_hidden_def (cairo_font_options_equal);













unsigned long
cairo_font_options_hash (const cairo_font_options_t *options)
{
    if (cairo_font_options_status ((cairo_font_options_t *) options))
	options = &_cairo_font_options_nil; 

    return ((options->antialias) |
	    (options->subpixel_order << 4) |
	    (options->lcd_filter << 8) |
	    (options->hint_style << 12) |
	    (options->hint_metrics << 16));
}
slim_hidden_def (cairo_font_options_hash);









void
cairo_font_options_set_antialias (cairo_font_options_t *options,
				  cairo_antialias_t     antialias)
{
    if (cairo_font_options_status (options))
	return;

    options->antialias = antialias;
}
slim_hidden_def (cairo_font_options_set_antialias);









cairo_antialias_t
cairo_font_options_get_antialias (const cairo_font_options_t *options)
{
    if (cairo_font_options_status ((cairo_font_options_t *) options))
	return CAIRO_ANTIALIAS_DEFAULT;

    return options->antialias;
}












void
cairo_font_options_set_subpixel_order (cairo_font_options_t   *options,
				       cairo_subpixel_order_t  subpixel_order)
{
    if (cairo_font_options_status (options))
	return;

    options->subpixel_order = subpixel_order;
}
slim_hidden_def (cairo_font_options_set_subpixel_order);










cairo_subpixel_order_t
cairo_font_options_get_subpixel_order (const cairo_font_options_t *options)
{
    if (cairo_font_options_status ((cairo_font_options_t *) options))
	return CAIRO_SUBPIXEL_ORDER_DEFAULT;

    return options->subpixel_order;
}













void
_cairo_font_options_set_lcd_filter (cairo_font_options_t *options,
				    cairo_lcd_filter_t    lcd_filter)
{
    if (cairo_font_options_status (options))
	return;

    options->lcd_filter = lcd_filter;
}












cairo_lcd_filter_t
_cairo_font_options_get_lcd_filter (const cairo_font_options_t *options)
{
    if (cairo_font_options_status ((cairo_font_options_t *) options))
	return CAIRO_LCD_FILTER_DEFAULT;

    return options->lcd_filter;
}











void
_cairo_font_options_set_round_glyph_positions (cairo_font_options_t *options,
					       cairo_round_glyph_positions_t  round)
{
    if (cairo_font_options_status (options))
	return;

    options->round_glyph_positions = round;
}











cairo_round_glyph_positions_t
_cairo_font_options_get_round_glyph_positions (const cairo_font_options_t *options)
{
    if (cairo_font_options_status ((cairo_font_options_t *) options))
	return CAIRO_ROUND_GLYPH_POS_DEFAULT;

    return options->round_glyph_positions;
}











void
cairo_font_options_set_hint_style (cairo_font_options_t *options,
				   cairo_hint_style_t    hint_style)
{
    if (cairo_font_options_status (options))
	return;

    options->hint_style = hint_style;
}
slim_hidden_def (cairo_font_options_set_hint_style);










cairo_hint_style_t
cairo_font_options_get_hint_style (const cairo_font_options_t *options)
{
    if (cairo_font_options_status ((cairo_font_options_t *) options))
	return CAIRO_HINT_STYLE_DEFAULT;

    return options->hint_style;
}











void
cairo_font_options_set_hint_metrics (cairo_font_options_t *options,
				     cairo_hint_metrics_t  hint_metrics)
{
    if (cairo_font_options_status (options))
	return;

    options->hint_metrics = hint_metrics;
}
slim_hidden_def (cairo_font_options_set_hint_metrics);










cairo_hint_metrics_t
cairo_font_options_get_hint_metrics (const cairo_font_options_t *options)
{
    if (cairo_font_options_status ((cairo_font_options_t *) options))
	return CAIRO_HINT_METRICS_DEFAULT;

    return options->hint_metrics;
}
