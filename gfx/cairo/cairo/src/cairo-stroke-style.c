


































#include "cairoint.h"

void
_cairo_stroke_style_init (cairo_stroke_style_t *style)
{
    VG (VALGRIND_MAKE_MEM_UNDEFINED (style, sizeof (cairo_stroke_style_t)));

    style->line_width = CAIRO_GSTATE_LINE_WIDTH_DEFAULT;
    style->line_cap = CAIRO_GSTATE_LINE_CAP_DEFAULT;
    style->line_join = CAIRO_GSTATE_LINE_JOIN_DEFAULT;
    style->miter_limit = CAIRO_GSTATE_MITER_LIMIT_DEFAULT;

    style->dash = NULL;
    style->num_dashes = 0;
    style->dash_offset = 0.0;
}

cairo_status_t
_cairo_stroke_style_init_copy (cairo_stroke_style_t *style,
			       cairo_stroke_style_t *other)
{
    if (CAIRO_INJECT_FAULT ())
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    VG (VALGRIND_MAKE_MEM_UNDEFINED (style, sizeof (cairo_stroke_style_t)));

    style->line_width = other->line_width;
    style->line_cap = other->line_cap;
    style->line_join = other->line_join;
    style->miter_limit = other->miter_limit;

    style->num_dashes = other->num_dashes;

    if (other->dash == NULL) {
	style->dash = NULL;
    } else {
	style->dash = _cairo_malloc_ab (style->num_dashes, sizeof (double));
	if (unlikely (style->dash == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	memcpy (style->dash, other->dash,
		style->num_dashes * sizeof (double));
    }

    style->dash_offset = other->dash_offset;

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_stroke_style_fini (cairo_stroke_style_t *style)
{
    if (style->dash) {
	free (style->dash);
	style->dash = NULL;
    }
    style->num_dashes = 0;

    VG (VALGRIND_MAKE_MEM_NOACCESS (style, sizeof (cairo_stroke_style_t)));
}






void
_cairo_stroke_style_max_distance_from_path (const cairo_stroke_style_t *style,
                                            const cairo_matrix_t *ctm,
                                            double *dx, double *dy)
{
    double style_expansion = 0.5;

    if (style->line_cap == CAIRO_LINE_CAP_SQUARE)
	style_expansion = M_SQRT1_2;

    if (style->line_join == CAIRO_LINE_JOIN_MITER &&
	style_expansion < M_SQRT2 * style->miter_limit)
    {
	style_expansion = M_SQRT2 * style->miter_limit;
    }

    style_expansion *= style->line_width;

    *dx = style_expansion * hypot (ctm->xx, ctm->xy);
    *dy = style_expansion * hypot (ctm->yy, ctm->yx);
}





double
_cairo_stroke_style_dash_period (const cairo_stroke_style_t *style)
{
    double period;
    unsigned int i;

    period = 0.0;
    for (i = 0; i < style->num_dashes; i++)
	period += style->dash[i];

    if (style->num_dashes & 1)
	period *= 2.0;

    return period;
}





#define ROUND_MINSQ_APPROXIMATION (9*M_PI/32)






double
_cairo_stroke_style_dash_stroked (const cairo_stroke_style_t *style)
{
    double stroked, cap_scale;
    unsigned int i;

    switch (style->line_cap) {
    default: ASSERT_NOT_REACHED;
    case CAIRO_LINE_CAP_BUTT:   cap_scale = 0.0; break;
    case CAIRO_LINE_CAP_ROUND:  cap_scale = ROUND_MINSQ_APPROXIMATION; break;
    case CAIRO_LINE_CAP_SQUARE: cap_scale = 1.0; break;
    }

    stroked = 0.0;
    if (style->num_dashes & 1) {
        

	for (i = 0; i < style->num_dashes; i++)
	    stroked += style->dash[i] + cap_scale * MIN (style->dash[i], style->line_width);
    } else {
        

	for (i = 0; i < style->num_dashes; i+=2)
	    stroked += style->dash[i] + cap_scale * MIN (style->dash[i+1], style->line_width);
    }

    return stroked;
}







cairo_bool_t
_cairo_stroke_style_dash_can_approximate (const cairo_stroke_style_t *style,
					  const cairo_matrix_t *ctm,
					  double tolerance)
{
    double period;

    if (! style->num_dashes)
        return FALSE;

    period = _cairo_stroke_style_dash_period (style);
    return _cairo_matrix_transformed_circle_major_axis (ctm, period) < tolerance;
}





void
_cairo_stroke_style_dash_approximate (const cairo_stroke_style_t *style,
				      const cairo_matrix_t *ctm,
				      double tolerance,
				      double *dash_offset,
				      double *dashes,
				      unsigned int *num_dashes)
{
    double coverage, scale, offset;
    cairo_bool_t on = TRUE;
    unsigned int i = 0;

    coverage = _cairo_stroke_style_dash_stroked (style) / _cairo_stroke_style_dash_period (style);
    coverage = MIN (coverage, 1.0);
    scale = tolerance / _cairo_matrix_transformed_circle_major_axis (ctm, 1.0);

    


    offset = style->dash_offset;
    while (offset > 0.0 && offset >= style->dash[i]) {
	offset -= style->dash[i];
	on = !on;
	if (++i == style->num_dashes)
	    i = 0;
    }

    *num_dashes = 2;

    dashes[0] = scale * coverage;
    dashes[1] = scale * (1.0 - coverage);

    *dash_offset = on ? 0.0 : dashes[0];
}
