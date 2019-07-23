


































#ifndef CAIRO_PATH_FIXED_PRIVATE_H
#define CAIRO_PATH_FIXED_PRIVATE_H

#include "cairo-types-private.h"
#include "cairo-compiler-private.h"

enum cairo_path_op {
    CAIRO_PATH_OP_MOVE_TO = 0,
    CAIRO_PATH_OP_LINE_TO = 1,
    CAIRO_PATH_OP_CURVE_TO = 2,
    CAIRO_PATH_OP_CLOSE_PATH = 3
};

typedef char cairo_path_op_t;


#define CAIRO_PATH_BUF_SIZE ((512 - 4 * sizeof (void*) - sizeof (cairo_path_buf_t)) \
			   / (2 * sizeof (cairo_point_t) + sizeof (cairo_path_op_t)))

typedef struct _cairo_path_buf {
    struct _cairo_path_buf *next, *prev;
    unsigned int buf_size;
    unsigned int num_ops;
    unsigned int num_points;

    cairo_path_op_t *op;
    cairo_point_t *points;
} cairo_path_buf_t;
typedef struct _cairo_path_buf_fixed {
    cairo_path_buf_t base;

    cairo_path_op_t op[CAIRO_PATH_BUF_SIZE];
    cairo_point_t points[2 * CAIRO_PATH_BUF_SIZE];
} cairo_path_buf_fixed_t;

struct _cairo_path_fixed {
    cairo_point_t last_move_point;
    cairo_point_t current_point;
    unsigned int has_current_point	: 1;
    unsigned int has_curve_to		: 1;

    cairo_path_buf_t       *buf_tail;
    cairo_path_buf_fixed_t  buf_head;
};

cairo_private unsigned long
_cairo_path_fixed_hash (const cairo_path_fixed_t *path);

cairo_private unsigned long
_cairo_path_fixed_size (const cairo_path_fixed_t *path);

cairo_private cairo_bool_t
_cairo_path_fixed_equal (const cairo_path_fixed_t *a,
			 const cairo_path_fixed_t *b);

typedef struct _cairo_path_fixed_iter {
    cairo_path_buf_t *buf;
    unsigned int n_op;
    unsigned int n_point;
} cairo_path_fixed_iter_t;

cairo_private void
_cairo_path_fixed_iter_init (cairo_path_fixed_iter_t *iter,
			     cairo_path_fixed_t *path);

cairo_private cairo_bool_t
_cairo_path_fixed_iter_is_fill_box (cairo_path_fixed_iter_t *_iter,
				    cairo_box_t *box);

cairo_private cairo_bool_t
_cairo_path_fixed_iter_at_end (const cairo_path_fixed_iter_t *iter);

#endif 
