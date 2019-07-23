
























#include "cairo_test.h"

#define WIDTH 64
#define HEIGHT 64

cairo_test_t test = {
    "clip_twice",
    "Verifies that the clip mask is updated correctly when it constructed by setting the clip path twice.",
    WIDTH, HEIGHT
};

static void
draw (cairo_t *cr, int width, int height)
{
    cairo_set_alpha (cr, 1.0);
    cairo_new_path (cr);
    cairo_arc (cr, WIDTH / 2, HEIGHT / 2, WIDTH / 3, 0, 2 * M_PI);
    cairo_clip (cr);

    cairo_new_path (cr);
    cairo_move_to (cr, 0, 0);
    cairo_line_to (cr, WIDTH / 4, HEIGHT / 2);
    cairo_line_to (cr, 0, HEIGHT);
    cairo_line_to (cr, WIDTH, HEIGHT);
    cairo_line_to (cr, 3 * WIDTH / 4, HEIGHT / 2);
    cairo_line_to (cr, WIDTH, 0);
    cairo_close_path (cr);
    cairo_clip (cr);

    cairo_set_rgb_color (cr, 0, 0, 0.6);

    cairo_new_path (cr);
    cairo_move_to (cr, 0, 0);
    cairo_line_to (cr, 0, HEIGHT);
    cairo_line_to (cr, WIDTH / 2, 3 * HEIGHT / 4);
    cairo_line_to (cr, WIDTH, HEIGHT);
    cairo_line_to (cr, WIDTH, 0);
    cairo_line_to (cr, WIDTH / 2, HEIGHT / 4);
    cairo_close_path (cr);
    cairo_fill (cr);
}

int
main (void)
{
    return cairo_test (&test, draw);
}
