


































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
	style_expansion < style->miter_limit)
    {
	style_expansion = style->miter_limit;
    }

    style_expansion *= style->line_width;

    *dx = style_expansion * (fabs (ctm->xx) + fabs (ctm->xy));
    *dy = style_expansion * (fabs (ctm->yy) + fabs (ctm->yx));
}
