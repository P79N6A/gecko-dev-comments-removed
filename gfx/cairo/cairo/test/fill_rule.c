






























































#include "cairo_test.h"

#define LITTLE_STAR_SIZE 20
#define BIG_STAR_SIZE    80

cairo_test_t test = {
    "fill_rule",
    "Tests cairo_set_full_rule with some star shapes",
    BIG_STAR_SIZE * 2 + 3, BIG_STAR_SIZE + LITTLE_STAR_SIZE + 3
};


static void
little_star_path (cairo_t *cr)
{
    cairo_move_to (cr, 10, 0);
    cairo_rel_line_to (cr, 6, 20);
    cairo_rel_line_to (cr, -16, -12);
    cairo_rel_line_to (cr, 20, 0);
    cairo_rel_line_to (cr, -16, 12);
}



static void
big_star_path (cairo_t *cr)
{
    cairo_move_to (cr, 40, 0);
    cairo_rel_line_to (cr, 25, 80);
    cairo_rel_line_to (cr, -65, -50);
    cairo_rel_line_to (cr, 80, 0);
    cairo_rel_line_to (cr, -65, 50);
    cairo_close_path (cr);
}


static void
draw (cairo_t *cr, int width, int height)
{
    cairo_set_rgb_color (cr, 1, 0, 0);

    cairo_translate (cr, 1, 1);
    little_star_path (cr);
    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_WINDING);
    cairo_fill (cr);

    cairo_translate (cr, LITTLE_STAR_SIZE + 1, 0);
    little_star_path (cr);
    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_fill (cr);

    cairo_translate (cr, -(LITTLE_STAR_SIZE + 1), LITTLE_STAR_SIZE + 1);
    big_star_path (cr);
    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_WINDING);
    cairo_fill (cr);

    cairo_translate (cr, BIG_STAR_SIZE + 1, 0);
    big_star_path (cr);
    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_fill (cr);
}

int
main (void)
{
    return cairo_test (&test, draw);
}
