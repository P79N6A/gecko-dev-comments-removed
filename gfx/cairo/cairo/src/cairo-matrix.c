



































#define _GNU_SOURCE
#include <stdlib.h>

#include "cairoint.h"

static void
_cairo_matrix_scalar_multiply (cairo_matrix_t *matrix, double scalar);

static void
_cairo_matrix_compute_adjoint (cairo_matrix_t *matrix);







void
cairo_matrix_init_identity (cairo_matrix_t *matrix)
{
    cairo_matrix_init (matrix,
		       1, 0,
		       0, 1,
		       0, 0);
}
slim_hidden_def(cairo_matrix_init_identity);



















void
cairo_matrix_init (cairo_matrix_t *matrix,
		   double xx, double yx,

		   double xy, double yy,
		   double x0, double y0)
{
    matrix->xx = xx; matrix->yx = yx;
    matrix->xy = xy; matrix->yy = yy;
    matrix->x0 = x0; matrix->y0 = y0;
}
slim_hidden_def(cairo_matrix_init);





















void
_cairo_matrix_get_affine (const cairo_matrix_t *matrix,
			  double *xx, double *yx,
			  double *xy, double *yy,
			  double *x0, double *y0)
{
    *xx  = matrix->xx;
    *yx  = matrix->yx;

    *xy  = matrix->xy;
    *yy  = matrix->yy;

    if (x0)
	*x0 = matrix->x0;
    if (y0)
	*y0 = matrix->y0;
}










void
cairo_matrix_init_translate (cairo_matrix_t *matrix,
			     double tx, double ty)
{
    cairo_matrix_init (matrix,
		       1, 0,
		       0, 1,
		       tx, ty);
}
slim_hidden_def(cairo_matrix_init_translate);












void
cairo_matrix_translate (cairo_matrix_t *matrix, double tx, double ty)
{
    cairo_matrix_t tmp;

    cairo_matrix_init_translate (&tmp, tx, ty);

    cairo_matrix_multiply (matrix, &tmp, matrix);
}
slim_hidden_def (cairo_matrix_translate);










void
cairo_matrix_init_scale (cairo_matrix_t *matrix,
			 double sx, double sy)
{
    cairo_matrix_init (matrix,
		       sx,  0,
		       0, sy,
		       0, 0);
}
slim_hidden_def(cairo_matrix_init_scale);











void
cairo_matrix_scale (cairo_matrix_t *matrix, double sx, double sy)
{
    cairo_matrix_t tmp;

    cairo_matrix_init_scale (&tmp, sx, sy);

    cairo_matrix_multiply (matrix, &tmp, matrix);
}
slim_hidden_def(cairo_matrix_scale);












void
cairo_matrix_init_rotate (cairo_matrix_t *matrix,
			  double radians)
{
    double  s;
    double  c;

    s = sin (radians);
    c = cos (radians);

    cairo_matrix_init (matrix,
		       c, s,
		       -s, c,
		       0, 0);
}
slim_hidden_def(cairo_matrix_init_rotate);















void
cairo_matrix_rotate (cairo_matrix_t *matrix, double radians)
{
    cairo_matrix_t tmp;

    cairo_matrix_init_rotate (&tmp, radians);

    cairo_matrix_multiply (matrix, &tmp, matrix);
}





















void
cairo_matrix_multiply (cairo_matrix_t *result, const cairo_matrix_t *a, const cairo_matrix_t *b)
{
    cairo_matrix_t r;

    r.xx = a->xx * b->xx + a->yx * b->xy;
    r.yx = a->xx * b->yx + a->yx * b->yy;

    r.xy = a->xy * b->xx + a->yy * b->xy;
    r.yy = a->xy * b->yx + a->yy * b->yy;

    r.x0 = a->x0 * b->xx + a->y0 * b->xy + b->x0;
    r.y0 = a->x0 * b->yx + a->y0 * b->yy + b->y0;

    *result = r;
}
slim_hidden_def(cairo_matrix_multiply);






















void
cairo_matrix_transform_distance (const cairo_matrix_t *matrix, double *dx, double *dy)
{
    double new_x, new_y;

    new_x = (matrix->xx * *dx + matrix->xy * *dy);
    new_y = (matrix->yx * *dx + matrix->yy * *dy);

    *dx = new_x;
    *dy = new_y;
}
slim_hidden_def(cairo_matrix_transform_distance);









void
cairo_matrix_transform_point (const cairo_matrix_t *matrix, double *x, double *y)
{
    cairo_matrix_transform_distance (matrix, x, y);

    *x += matrix->x0;
    *y += matrix->y0;
}
slim_hidden_def(cairo_matrix_transform_point);

void
cairo_matrix_transform_bounding_box (const cairo_matrix_t *matrix,
                                     double *x1, double *y1,
                                     double *x2, double *y2,
                                     cairo_bool_t *is_tight)
{
    int i;
    double quad_x[4], quad_y[4];
    double min_x, max_x;
    double min_y, max_y;

    quad_x[0] = *x1;
    quad_y[0] = *y1;
    cairo_matrix_transform_point (matrix, &quad_x[0], &quad_y[0]);

    quad_x[1] = *x2;
    quad_y[1] = *y1;
    cairo_matrix_transform_point (matrix, &quad_x[1], &quad_y[1]);

    quad_x[2] = *x1;
    quad_y[2] = *y2;
    cairo_matrix_transform_point (matrix, &quad_x[2], &quad_y[2]);

    quad_x[3] = *x2;
    quad_y[3] = *y2;
    cairo_matrix_transform_point (matrix, &quad_x[3], &quad_y[3]);

    min_x = max_x = quad_x[0];
    min_y = max_y = quad_y[0];

    for (i=1; i < 4; i++) {
	if (quad_x[i] < min_x)
	    min_x = quad_x[i];
	if (quad_x[i] > max_x)
	    max_x = quad_x[i];

	if (quad_y[i] < min_y)
	    min_y = quad_y[i];
	if (quad_y[i] > max_y)
	    max_y = quad_y[i];
    }

    *x1 = min_x;
    *y1 = min_y;
    *x2 = max_x;
    *y2 = max_y;
    
    if (is_tight) {
        






        *is_tight =
            (quad_x[1] == quad_x[0] && quad_y[1] == quad_y[3] &&
             quad_x[2] == quad_x[3] && quad_y[2] == quad_y[0]) ||
            (quad_x[1] == quad_x[3] && quad_y[1] == quad_y[0] &&
             quad_x[2] == quad_x[0] && quad_y[2] == quad_y[3]);
    }
}
slim_hidden_def(cairo_matrix_transform_bounding_box);

static void
_cairo_matrix_scalar_multiply (cairo_matrix_t *matrix, double scalar)
{
    matrix->xx *= scalar;
    matrix->yx *= scalar;

    matrix->xy *= scalar;
    matrix->yy *= scalar;

    matrix->x0 *= scalar;
    matrix->y0 *= scalar;
}





static void
_cairo_matrix_compute_adjoint (cairo_matrix_t *matrix)
{
    
    double a, b, c, d, tx, ty;

    _cairo_matrix_get_affine (matrix,
			      &a,  &b,
			      &c,  &d,
			      &tx, &ty);

    cairo_matrix_init (matrix,
		       d, -b,
		       -c, a,
		       c*ty - d*tx, b*tx - a*ty);
}














cairo_status_t
cairo_matrix_invert (cairo_matrix_t *matrix)
{
    
    double det;

    _cairo_matrix_compute_determinant (matrix, &det);

    if (det == 0)
	return CAIRO_STATUS_INVALID_MATRIX;

    _cairo_matrix_compute_adjoint (matrix);
    _cairo_matrix_scalar_multiply (matrix, 1 / det);

    return CAIRO_STATUS_SUCCESS;
}
slim_hidden_def(cairo_matrix_invert);

void
_cairo_matrix_compute_determinant (const cairo_matrix_t *matrix,
				   double		*det)
{
    double a, b, c, d;

    a = matrix->xx; b = matrix->yx;
    c = matrix->xy; d = matrix->yy;

    *det = a*d - b*c;
}


cairo_status_t
_cairo_matrix_compute_scale_factors (const cairo_matrix_t *matrix,
				     double *sx, double *sy, int x_major)
{
    double det;

    _cairo_matrix_compute_determinant (matrix, &det);

    if (det == 0)
    {
	*sx = *sy = 0;
    }
    else
    {
	double x = x_major != 0;
	double y = x == 0;
	double major, minor;

	cairo_matrix_transform_distance (matrix, &x, &y);
	major = sqrt(x*x + y*y);
	


	if (det < 0)
	    det = -det;
	if (major)
	    minor = det / major;
	else
	    minor = 0.0;
	if (x_major)
	{
	    *sx = major;
	    *sy = minor;
	}
	else
	{
	    *sx = minor;
	    *sy = major;
	}
    }

    return CAIRO_STATUS_SUCCESS;
}

cairo_bool_t
_cairo_matrix_is_identity (const cairo_matrix_t *matrix)
{
    return (matrix->xx == 1.0 && matrix->yx == 0.0 &&
	    matrix->xy == 0.0 && matrix->yy == 1.0 &&
	    matrix->x0 == 0.0 && matrix->y0 == 0.0);
}

cairo_bool_t
_cairo_matrix_is_translation (const cairo_matrix_t *matrix)
{
    return (matrix->xx == 1.0 && matrix->yx == 0.0 &&
	    matrix->xy == 0.0 && matrix->yy == 1.0);
}

cairo_bool_t
_cairo_matrix_is_integer_translation (const cairo_matrix_t *matrix,
				      int *itx, int *ity)
{
    if (_cairo_matrix_is_translation (matrix))
    {
        cairo_fixed_t x0_fixed = _cairo_fixed_from_double (matrix->x0);
        cairo_fixed_t y0_fixed = _cairo_fixed_from_double (matrix->y0);

        if (_cairo_fixed_is_integer (x0_fixed) &&
            _cairo_fixed_is_integer (y0_fixed))
        {
            if (itx)
                *itx = _cairo_fixed_integer_part (x0_fixed);
            if (ity)
                *ity = _cairo_fixed_integer_part (y0_fixed);

            return TRUE;
        }
    }

    return FALSE;
}






















































































































double
_cairo_matrix_transformed_circle_major_axis (cairo_matrix_t *matrix, double radius)
{
    double  a, b, c, d, f, g, h, i, j;

    _cairo_matrix_get_affine (matrix,
                              &a, &b,
                              &c, &d,
                              NULL, NULL);

    i = a*a + b*b;
    j = c*c + d*d;

    f = 0.5 * (i + j);
    g = 0.5 * (i - j);
    h = a*c + b*d;

    return radius * sqrt (f + sqrt (g*g+h*h));

    



}

void
_cairo_matrix_to_pixman_matrix (const cairo_matrix_t	*matrix,
				pixman_transform_t	*pixman_transform)
{
    static const pixman_transform_t pixman_identity_transform = {{
        {1 << 16,        0,       0},
        {       0, 1 << 16,       0},
        {       0,       0, 1 << 16}
    }};

    if (_cairo_matrix_is_identity (matrix)) {
        *pixman_transform = pixman_identity_transform;
    }
    else {
        pixman_transform->matrix[0][0] = _cairo_fixed_from_double (matrix->xx);
        pixman_transform->matrix[0][1] = _cairo_fixed_from_double (matrix->xy);
        pixman_transform->matrix[0][2] = _cairo_fixed_from_double (matrix->x0);

        pixman_transform->matrix[1][0] = _cairo_fixed_from_double (matrix->yx);
        pixman_transform->matrix[1][1] = _cairo_fixed_from_double (matrix->yy);
        pixman_transform->matrix[1][2] = _cairo_fixed_from_double (matrix->y0);

        pixman_transform->matrix[2][0] = 0;
        pixman_transform->matrix[2][1] = 0;
        pixman_transform->matrix[2][2] = 1 << 16;
    }
}
