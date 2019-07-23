




































#include "cairo_test.h"

cairo_test_t test = {
    "move_to_show_surface",
    "Tests calls to cairo_show_surface after cairo_move_to",
    2, 2
};

static void
draw (cairo_t *cr, int width, int height)
{
    cairo_surface_t *surface;
    uint32_t colors[4] = {
	0xffffffff, 0xffff0000,
	0xff00ff00, 0xff0000ff
    };
    int i;

    for (i=0; i < 4; i++) {
	surface = cairo_surface_create_for_image ((char *) &colors[i],
						  CAIRO_FORMAT_ARGB32, 1, 1, 4);
	cairo_move_to (cr, i % 2, i / 2);
	cairo_show_surface (cr, surface, 1, 1);
	cairo_surface_destroy (surface);
    }
}

int
main (void)
{
    return cairo_test (&test, draw);
}
