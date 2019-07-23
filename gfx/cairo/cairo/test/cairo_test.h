
























#ifndef _CAIRO_TEST_H_
#define _CAIRO_TEST_H_

#include <math.h>
#include <cairo.h>

typedef enum cairo_test_status {
    CAIRO_TEST_SUCCESS = 0,
    CAIRO_TEST_FAILURE
} cairo_test_status_t;

typedef struct cairo_test {
    char *name;
    char *description;
    int width;
    int height;
} cairo_test_t;

typedef void  (*cairo_test_draw_function_t) (cairo_t *cr, int width, int height);


cairo_test_status_t
cairo_test (cairo_test_t *test, cairo_test_draw_function_t draw);

cairo_pattern_t *
cairo_test_create_png_pattern (cairo_t *cr, const char *filename);


#endif

