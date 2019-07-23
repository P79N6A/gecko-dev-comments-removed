





























































#include "cairo_test.h"

cairo_test_t test = {
    "text_cache_crash",
    "Test case for bug causing an assertion failure in _cairo_cache_lookup",
    0, 0,
};
#include <cairo.h>

static void
draw (cairo_t *cr, int width, int height)
{
    
    cairo_select_font(cr, "sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_scale_font(cr, 40.0);

    cairo_select_font(cr, "sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_scale_font(cr, 40.0);
    cairo_move_to(cr, 10, 50);
    cairo_show_text(cr, "hello");

    























}

int
main (void)
{
    int ret;

    ret = cairo_test (&test, draw);

    








    




    return ret;
}

