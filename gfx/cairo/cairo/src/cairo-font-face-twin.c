



































#include "cairoint.h"










#define twin_glyph_left(g)      ((g)[0])
#define twin_glyph_right(g)     ((g)[1])
#define twin_glyph_ascent(g)    ((g)[2])
#define twin_glyph_descent(g)   ((g)[3])

#define twin_glyph_n_snap_x(g)  ((g)[4])
#define twin_glyph_n_snap_y(g)  ((g)[5])
#define twin_glyph_snap_x(g)    (&g[6])
#define twin_glyph_snap_y(g)    (twin_glyph_snap_x(g) + twin_glyph_n_snap_x(g))
#define twin_glyph_draw(g)      (twin_glyph_snap_y(g) + twin_glyph_n_snap_y(g))

#define SNAPI(p)	(p)
#define SNAPH(p)	(p)

#define FX(g)		((g) / 64.)
#define FY(g)		((g) / 64.)


static cairo_status_t
twin_scaled_font_init (cairo_scaled_font_t  *scaled_font,
		       cairo_t              *cr,
		       cairo_font_extents_t *metrics)
{
  metrics->ascent  = FY (50);
  metrics->descent = FY (14);
  return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
twin_scaled_font_unicode_to_glyph (cairo_scaled_font_t *scaled_font,
				   unsigned long        unicode,
				   unsigned long       *glyph)
{
    




    if (unicode < ARRAY_LENGTH (_cairo_twin_charmap))
	*glyph = unicode;
    else
	*glyph = 0;

    return CAIRO_STATUS_SUCCESS;
}

#define SNAPX(p)	_twin_snap (p, info.snap_x, info.n_snap_x)
#define SNAPY(p)	_twin_snap (p, info.snap_y, info.n_snap_y)

static double
_twin_snap (double v, int a, int b)
{
    return v; 
}

static cairo_status_t
twin_scaled_font_render_glyph (cairo_scaled_font_t  *scaled_font,
			       unsigned long         glyph,
			       cairo_t              *cr,
			       cairo_text_extents_t *metrics)
{
    double x1, y1, x2, y2, x3, y3;
    const int8_t *b = _cairo_twin_outlines +
		      _cairo_twin_charmap[glyph >= ARRAY_LENGTH (_cairo_twin_charmap) ? 0 : glyph];
    const int8_t *g = twin_glyph_draw(b);

    struct {
      cairo_bool_t snap;
      int snap_x;
      int snap_y;
      int n_snap_x;
      int n_snap_y;
    } info = {FALSE};

    cairo_set_line_width (cr, 0.06);
    cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);

    for (;;) {
	switch (*g++) {
	case 'M':
	    cairo_close_path (cr);
	    
	case 'm':
	    x1 = FX(*g++);
	    y1 = FY(*g++);
	    if (info.snap)
	    {
		x1 = SNAPX (x1);
		y1 = SNAPY (y1);
	    }
	    cairo_move_to (cr, x1, y1);
	    continue;
	case 'L':
	    cairo_close_path (cr);
	    
	case 'l':
	    x1 = FX(*g++);
	    y1 = FY(*g++);
	    if (info.snap)
	    {
		x1 = SNAPX (x1);
		y1 = SNAPY (y1);
	    }
	    cairo_line_to (cr, x1, y1);
	    continue;
	case 'C':
	    cairo_close_path (cr);
	    
	case 'c':
	    x1 = FX(*g++);
	    y1 = FY(*g++);
	    x2 = FX(*g++);
	    y2 = FY(*g++);
	    x3 = FX(*g++);
	    y3 = FY(*g++);
	    if (info.snap)
	    {
		x1 = SNAPX (x1);
		y1 = SNAPY (y1);
		x2 = SNAPX (x2);
		y2 = SNAPY (y2);
		x3 = SNAPX (x3);
		y3 = SNAPY (y3);
	    }
	    cairo_curve_to (cr, x1, y1, x2, y2, x3, y3);
	    continue;
	case 'E':
	    cairo_close_path (cr);
	    
	case 'e':
	    cairo_stroke (cr);
	    break;
	case 'X':
	    
	    continue;
	}
	break;
    }

    metrics->x_advance = FX(twin_glyph_right(b)) + cairo_get_line_width (cr);
    metrics->x_advance +=  cairo_get_line_width (cr);
    if (info.snap)
	metrics->x_advance = SNAPI (SNAPX (metrics->x_advance));


    return CAIRO_STATUS_SUCCESS;
}

cairo_font_face_t *
_cairo_font_face_twin_create (cairo_font_slant_t slant,
			      cairo_font_weight_t weight)
{
    cairo_font_face_t *twin_font_face;

    twin_font_face = cairo_user_font_face_create ();
    cairo_user_font_face_set_init_func             (twin_font_face, twin_scaled_font_init);
    cairo_user_font_face_set_render_glyph_func     (twin_font_face, twin_scaled_font_render_glyph);
    cairo_user_font_face_set_unicode_to_glyph_func (twin_font_face, twin_scaled_font_unicode_to_glyph);

    return twin_font_face;
}
