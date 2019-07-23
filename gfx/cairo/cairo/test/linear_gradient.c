
























#include "cairo_test.h"
#include "stdio.h"















static const double gradient_angles[] = { 0, 45, 90 };
#define N_GRADIENT_ANGLES 3
static const double rotate_angles[] = { 0, 45, 90 };
#define N_ROTATE_ANGLES 3
static const int n_stops[] = { 2, 3 };
#define N_N_STOPS 2

#define UNIT_SIZE 75
#define UNIT_SIZE 75
#define PAD 5

#define WIDTH  N_GRADIENT_ANGLES * UNIT_SIZE + (N_GRADIENT_ANGLES + 1) * PAD
#define HEIGHT N_N_STOPS * N_ROTATE_ANGLES * UNIT_SIZE + (N_N_STOPS * N_ROTATE_ANGLES + 1) * PAD

cairo_test_t test = {
    "linear_gradient",
    "Tests the drawing of linear gradients",
    WIDTH, HEIGHT
};

static void
draw_unit (cairo_t *cr,
	   double   gradient_angle,
	   double   rotate_angle,
	   int      n_stops)
{
    cairo_pattern_t *pattern;

    cairo_rectangle (cr, 0, 0, 1, 1);
    cairo_clip (cr);
    cairo_new_path(cr);
    
    cairo_set_rgb_color (cr, 0.0, 0.0, 0.0);
    cairo_rectangle (cr, 0, 0, 1, 1);
    cairo_fill (cr);
    
    cairo_translate (cr, 0.5, 0.5);
    cairo_scale (cr, 1 / 1.5, 1 / 1.5);
    cairo_rotate (cr, rotate_angle);
    
    pattern = cairo_pattern_create_linear (-0.5 * cos (gradient_angle),  -0.5 * sin (gradient_angle),
 					    0.5 * cos (gradient_angle),   0.5 * sin (gradient_angle));

    if (n_stops == 2) {
	cairo_pattern_add_color_stop (pattern, 0.,
				      0.3, 0.3, 0.3,
				      1.0);
	cairo_pattern_add_color_stop (pattern, 1.,
				      1.0, 1.0, 1.0,
				      1.0);
    } else {
	cairo_pattern_add_color_stop (pattern, 0.,
				      1.0, 0.0, 0.0,
				      1.0);
	cairo_pattern_add_color_stop (pattern, 0.5,
				      1.0, 1.0, 1.0,
				      1.0);
	cairo_pattern_add_color_stop (pattern, 1.,
				      0.0, 0.0, 1.0,
				      1.0);
    }

    cairo_set_pattern (cr, pattern);
    cairo_pattern_destroy (pattern);
    cairo_rectangle (cr, -0.5, -0.5, 1, 1);
    cairo_fill (cr);
}

static void
draw (cairo_t *cr, int width, int height)
{
    int i, j, k;

    cairo_set_rgb_color (cr, 0.5, 0.5, 0.5);
    cairo_rectangle (cr, 0, 0, width, height);
    cairo_fill (cr);

    for (i = 0; i < N_GRADIENT_ANGLES; i++)
	for (j = 0; j < N_ROTATE_ANGLES; j++)
	  for (k = 0; k < N_N_STOPS; k++) {
		cairo_save (cr);
		cairo_translate (cr,
				 PAD + (PAD + UNIT_SIZE) * i,
				 PAD + (PAD + UNIT_SIZE) * (N_ROTATE_ANGLES * k + j));
		cairo_scale (cr, UNIT_SIZE, UNIT_SIZE);
		
		draw_unit (cr,
			   gradient_angles[i] * M_PI / 180.,
			   rotate_angles[j] * M_PI / 180.,
			   n_stops[k]);
		cairo_restore (cr);
	    }
}

int
main (void)
{
    return cairo_test (&test, draw);
}
