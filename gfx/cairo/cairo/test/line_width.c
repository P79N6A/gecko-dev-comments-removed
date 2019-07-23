
























#include "cairo_test.h"

#define LINES 5
#define LINE_LENGTH 10
#define IMAGE_WIDTH 2 * LINE_LENGTH + 6
#define IMAGE_HEIGHT ((LINES+4)*LINES)/2 + 2

cairo_test_t test = {
    "line_width",
    "Tests cairo_set_line_width",
    IMAGE_WIDTH, IMAGE_HEIGHT
};

static void
draw (cairo_t *cr, int width, int height)
{
    int i;

    cairo_set_rgb_color (cr, 0, 0, 0);
    cairo_translate (cr, 2, 2);

    for (i=0; i < LINES; i++) {
	cairo_set_line_width (cr, i+1);
	cairo_move_to (cr, 0, 0);
	cairo_rel_line_to (cr, LINE_LENGTH, 0);
	cairo_stroke (cr);
	cairo_move_to (cr, LINE_LENGTH + 2, 0.5);
	cairo_rel_line_to (cr, LINE_LENGTH, 0);
	cairo_stroke (cr);
	cairo_translate (cr, 0, i+3);
    }
}

int
main (void)
{
    return cairo_test (&test, draw);
}
