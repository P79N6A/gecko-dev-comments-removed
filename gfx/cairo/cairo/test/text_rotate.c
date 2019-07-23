


























































#include "cairo_test.h"

#define WIDTH  150
#define HEIGHT 150
#define NUM_TEXT 20
#define TEXT_SIZE 12

cairo_test_t test = {
    "text_rotate",
    "Tests show_text under various rotations",
    WIDTH, HEIGHT
};


static void
draw (cairo_t *cr, int width, int height)
{
    int i, x_off, y_off;
    cairo_text_extents_t extents;
    static char text[] = "cairo";

    cairo_select_font (cr, "Bitstream Vera Sans",
		       CAIRO_FONT_SLANT_NORMAL,
		       CAIRO_FONT_WEIGHT_NORMAL);
    cairo_scale_font (cr, TEXT_SIZE);

    cairo_set_rgb_color (cr, 0,0,0);

    cairo_translate (cr, WIDTH/2.0, HEIGHT/2.0);

    cairo_text_extents (cr, text, &extents);

    if (NUM_TEXT == 1) {
	x_off = y_off = 0;
    } else {
	y_off = - round (extents.height / 2.0);
	x_off = round ((extents.height+1) / (2 * tan (M_PI/NUM_TEXT)));
    }
  
    for (i=0; i < NUM_TEXT; i++) {
	cairo_save (cr);
	cairo_rotate (cr, 2*M_PI*i/NUM_TEXT);
	cairo_set_line_width (cr, 1.0);
	cairo_rectangle (cr, x_off - 0.5, y_off - 0.5, extents.width + 1, extents.height + 1);
	cairo_set_rgb_color (cr, 1, 0, 0);
	cairo_stroke (cr);
	cairo_move_to (cr, x_off - extents.x_bearing, y_off - extents.y_bearing);
	cairo_set_rgb_color (cr, 0, 0, 0);
	cairo_show_text (cr, "cairo");
	cairo_restore (cr);
    }
}

int
main (void)
{
    return cairo_test (&test, draw);
}
