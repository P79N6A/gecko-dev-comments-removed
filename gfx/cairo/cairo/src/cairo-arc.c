



































#include "cairoint.h"

#include "cairo-arc-private.h"





















static double
_arc_error_normalized (double angle)
{
    return 2.0/27.0 * pow (sin (angle / 4), 6) / pow (cos (angle / 4), 2);
}

static double
_arc_max_angle_for_tolerance_normalized (double tolerance)
{
    double angle, error;
    int i;

    
    struct {
	double angle;
	double error;
    } table[] = {
	{ M_PI / 1.0,   0.0185185185185185036127 },
	{ M_PI / 2.0,   0.000272567143730179811158 },
	{ M_PI / 3.0,   2.38647043651461047433e-05 },
	{ M_PI / 4.0,   4.2455377443222443279e-06 },
	{ M_PI / 5.0,   1.11281001494389081528e-06 },
	{ M_PI / 6.0,   3.72662000942734705475e-07 },
	{ M_PI / 7.0,   1.47783685574284411325e-07 },
	{ M_PI / 8.0,   6.63240432022601149057e-08 },
	{ M_PI / 9.0,   3.2715520137536980553e-08 },
	{ M_PI / 10.0,  1.73863223499021216974e-08 },
	{ M_PI / 11.0,  9.81410988043554039085e-09 },
    };
    int table_size = ARRAY_LENGTH (table);

    for (i = 0; i < table_size; i++)
	if (table[i].error < tolerance)
	    return table[i].angle;

    ++i;
    do {
	angle = M_PI / i++;
	error = _arc_error_normalized (angle);
    } while (error > tolerance);

    return angle;
}

static int
_arc_segments_needed (double	      angle,
		      double	      radius,
		      cairo_matrix_t *ctm,
		      double	      tolerance)
{
    double major_axis, max_angle;

    


    major_axis = _cairo_matrix_transformed_circle_major_axis (ctm, radius);
    max_angle = _arc_max_angle_for_tolerance_normalized (tolerance / major_axis);

    return (int) ceil (angle / max_angle);
}


























static void
_cairo_arc_segment (cairo_t *cr,
		    double   xc,
		    double   yc,
		    double   radius,
		    double   angle_A,
		    double   angle_B)
{
    double r_sin_A, r_cos_A;
    double r_sin_B, r_cos_B;
    double h;

    r_sin_A = radius * sin (angle_A);
    r_cos_A = radius * cos (angle_A);
    r_sin_B = radius * sin (angle_B);
    r_cos_B = radius * cos (angle_B);

    h = 4.0/3.0 * tan ((angle_B - angle_A) / 4.0);

    cairo_curve_to (cr,
		    xc + r_cos_A - h * r_sin_A,
		    yc + r_sin_A + h * r_cos_A,
		    xc + r_cos_B + h * r_sin_B,
		    yc + r_sin_B - h * r_cos_B,
		    xc + r_cos_B,
		    yc + r_sin_B);
}

static void
_cairo_arc_in_direction (cairo_t	  *cr,
			 double		   xc,
			 double		   yc,
			 double		   radius,
			 double		   angle_min,
			 double		   angle_max,
			 cairo_direction_t dir)
{
    while (angle_max - angle_min > 4 * M_PI)
	angle_max -= 2 * M_PI;

    
    if (angle_max - angle_min > M_PI) {
	double angle_mid = angle_min + (angle_max - angle_min) / 2.0;
	
	if (dir == CAIRO_DIRECTION_FORWARD) {
	    _cairo_arc_in_direction (cr, xc, yc, radius,
				     angle_min, angle_mid,
				     dir);

	    _cairo_arc_in_direction (cr, xc, yc, radius,
				     angle_mid, angle_max,
				     dir);
	} else {
	    _cairo_arc_in_direction (cr, xc, yc, radius,
				     angle_mid, angle_max,
				     dir);

	    _cairo_arc_in_direction (cr, xc, yc, radius,
				     angle_min, angle_mid,
				     dir);
	}
    } else {
	cairo_matrix_t ctm;
	int i, segments;
	double angle, angle_step;

	cairo_get_matrix (cr, &ctm);
	segments = _arc_segments_needed (angle_max - angle_min,
					 radius, &ctm,
					 cairo_get_tolerance (cr));
	angle_step = (angle_max - angle_min) / (double) segments;

	if (dir == CAIRO_DIRECTION_FORWARD) {
	    angle = angle_min;
	} else {
	    angle = angle_max;
	    angle_step = - angle_step;
	}

	for (i = 0; i < segments; i++, angle += angle_step) {
	    _cairo_arc_segment (cr, xc, yc,
				radius,
				angle,
				angle + angle_step);
	}
    }
}














void
_cairo_arc_path (cairo_t *cr,
		 double	  xc,
		 double	  yc,
		 double	  radius,
		 double	  angle1,
		 double	  angle2)
{
    _cairo_arc_in_direction (cr, xc, yc,
			     radius,
			     angle1, angle2,
			     CAIRO_DIRECTION_FORWARD);
}

















void
_cairo_arc_path_negative (cairo_t *cr,
			  double   xc,
			  double   yc,
			  double   radius,
			  double   angle1,
			  double   angle2)
{
    _cairo_arc_in_direction (cr, xc, yc,
			     radius,
			     angle2, angle1,
			     CAIRO_DIRECTION_REVERSE);
}
