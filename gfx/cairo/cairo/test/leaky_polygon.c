




















































#include "cairo_test.h"

#define WIDTH 21
#define HEIGHT 21

cairo_test_t test = {
    "leaky_polygon",
    "Exercises a corner case in the trapezoid rasterization in which pixels outside the trapezoids received a non-zero alpha",
    WIDTH, HEIGHT
};

static void
draw (cairo_t *cr, int width, int height)
{
    cairo_scale (cr, 1.0/(1<<16), 1.0/(1<<16));

    cairo_move_to (cr, 131072,39321);
    cairo_line_to (cr, 1103072,1288088);
    cairo_line_to (cr, 1179648,1294990);
    cairo_close_path (cr);

    cairo_fill (cr);
}

int
main (void)
{
    return cairo_test (&test, draw);
}
