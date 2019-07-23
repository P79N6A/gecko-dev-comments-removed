


































#ifndef CAIRO_PATH_FIXED_PRIVATE_H
#define CAIRO_PATH_FIXED_PRIVATE_H

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
    int buf_size;
    int num_ops;
    int num_points;

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

#endif 
