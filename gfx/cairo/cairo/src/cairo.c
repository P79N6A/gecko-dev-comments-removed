





































#include "cairoint.h"
#include "cairo-private.h"

#include "cairo-arc-private.h"
#include "cairo-path-private.h"

#define CAIRO_TOLERANCE_MINIMUM	0.0002 /* We're limited by 16 bits of sub-pixel precision */

static const cairo_t _cairo_nil = {
  CAIRO_REF_COUNT_INVALID,	
  CAIRO_STATUS_NO_MEMORY,	
  { 0, 0, 0, NULL },		
  NULL,				
  {{				
    0
  }},
  {{ 				
    { 0, 0 },			   
    { 0, 0 },			   
    FALSE,			   
    FALSE,			   
    NULL, {{0}}			   
  }}
};

#include <assert.h>





#define CAIRO_STATUS_LAST_STATUS CAIRO_STATUS_INVALID_INDEX
















void
_cairo_error (cairo_status_t status)
{
    assert (status > CAIRO_STATUS_SUCCESS &&
	    status <= CAIRO_STATUS_LAST_STATUS);
}

















static void
_cairo_set_error (cairo_t *cr, cairo_status_t status)
{
    


    if (cr->status == CAIRO_STATUS_SUCCESS)
	cr->status = status;

    _cairo_error (status);
}




















int
cairo_version (void)
{
    return CAIRO_VERSION;
}












const char*
cairo_version_string (void)
{
    return CAIRO_VERSION_STRING;
}
slim_hidden_def (cairo_version_string);
























cairo_t *
cairo_create (cairo_surface_t *target)
{
    cairo_t *cr;
    cairo_status_t status;

    
    if (target && target->status == CAIRO_STATUS_NO_MEMORY)
	return (cairo_t *) &_cairo_nil;

    cr = malloc (sizeof (cairo_t));
    if (cr == NULL)
	return (cairo_t *) &_cairo_nil;

    cr->ref_count = 1;

    cr->status = CAIRO_STATUS_SUCCESS;

    _cairo_user_data_array_init (&cr->user_data);
    _cairo_path_fixed_init (cr->path);

    cr->gstate = cr->gstate_tail;
    status = _cairo_gstate_init (cr->gstate, target);

    if (status)
	_cairo_set_error (cr, status);

    return cr;
}
slim_hidden_def (cairo_create);














cairo_t *
cairo_reference (cairo_t *cr)
{
    if (cr == NULL || cr->ref_count == CAIRO_REF_COUNT_INVALID)
	return cr;

    assert (cr->ref_count > 0);

    cr->ref_count++;

    return cr;
}









void
cairo_destroy (cairo_t *cr)
{
    if (cr == NULL || cr->ref_count == CAIRO_REF_COUNT_INVALID)
	return;

    assert (cr->ref_count > 0);

    cr->ref_count--;
    if (cr->ref_count)
	return;

    while (cr->gstate != cr->gstate_tail) {
	if (_cairo_gstate_restore (&cr->gstate))
	    break;
    }

    _cairo_gstate_fini (cr->gstate);

    _cairo_path_fixed_fini (cr->path);

    _cairo_user_data_array_fini (&cr->user_data);

    free (cr);
}
slim_hidden_def (cairo_destroy);















void *
cairo_get_user_data (cairo_t			 *cr,
		     const cairo_user_data_key_t *key)
{
    return _cairo_user_data_array_get_data (&cr->user_data,
					    key);
}



















cairo_status_t
cairo_set_user_data (cairo_t			 *cr,
		     const cairo_user_data_key_t *key,
		     void			 *user_data,
		     cairo_destroy_func_t	 destroy)
{
    if (cr->ref_count == CAIRO_REF_COUNT_INVALID)
	return CAIRO_STATUS_NO_MEMORY;

    return _cairo_user_data_array_set_data (&cr->user_data,
					    key, user_data, destroy);
}












unsigned int
cairo_get_reference_count (cairo_t *cr)
{
    if (cr == NULL || cr->ref_count == CAIRO_REF_COUNT_INVALID)
	return 0;

    return cr->ref_count;
}

















void
cairo_save (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_save (&cr->gstate);
    if (status) {
	_cairo_set_error (cr, status);
    }
}
slim_hidden_def(cairo_save);









void
cairo_restore (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_restore (&cr->gstate);
    if (status) {
	_cairo_set_error (cr, status);
    }
}
slim_hidden_def(cairo_restore);













































void
cairo_push_group (cairo_t *cr)
{
    cairo_push_group_with_content (cr, CAIRO_CONTENT_COLOR_ALPHA);
}
slim_hidden_def(cairo_push_group);




















void
cairo_push_group_with_content (cairo_t *cr, cairo_content_t content)
{
    cairo_status_t status;
    cairo_rectangle_int_t extents;
    cairo_surface_t *parent_surface, *group_surface = NULL;
    int width;
    int height;

    if (cr->status)
	return;

    parent_surface = _cairo_gstate_get_target (cr->gstate);
    
    status = _cairo_surface_get_extents (parent_surface, &extents);
    if (status)
	goto bail;
    status = _cairo_clip_intersect_to_rectangle (_cairo_gstate_get_clip (cr->gstate), &extents);
    if (status)
	goto bail;

    width = extents.width;
    height = extents.height;

    if (width == 0 || height == 0)
	width = height = 1;

    group_surface = cairo_surface_create_similar (_cairo_gstate_get_target (cr->gstate),
						  content,
						  width, height);
    status = cairo_surface_status (group_surface);
    if (status)
	goto bail;

    




    cairo_surface_set_device_offset (group_surface,
                                     parent_surface->device_transform.x0 - extents.x,
                                     parent_surface->device_transform.y0 - extents.y);

    
    cairo_save (cr);
    if (cr->status)
	goto bail;

    status = _cairo_gstate_redirect_target (cr->gstate, group_surface);

bail:
    cairo_surface_destroy (group_surface);
    if (status)
	_cairo_set_error (cr, status);
}
slim_hidden_def(cairo_push_group_with_content);






















cairo_pattern_t *
cairo_pop_group (cairo_t *cr)
{
    cairo_surface_t *group_surface, *parent_target;
    cairo_pattern_t *group_pattern = NULL;
    cairo_matrix_t group_matrix;

    if (cr->status)
	return (cairo_pattern_t*) &_cairo_pattern_nil.base;

    
    group_surface = _cairo_gstate_get_target (cr->gstate);
    parent_target = _cairo_gstate_get_parent_target (cr->gstate);

    
    if (parent_target == NULL) {
	_cairo_set_error (cr, CAIRO_STATUS_INVALID_POP_GROUP);
	return (cairo_pattern_t*) &_cairo_pattern_nil.base;
    }

    


    group_surface = cairo_surface_reference (group_surface);

    cairo_restore (cr);

    if (cr->status)
	goto done;

    group_pattern = cairo_pattern_create_for_surface (group_surface);
    if (cairo_pattern_status (group_pattern)) {
	_cairo_set_error (cr, cairo_pattern_status (group_pattern));
        goto done;
    }

    _cairo_gstate_get_matrix (cr->gstate, &group_matrix);
    cairo_pattern_set_matrix (group_pattern, &group_matrix);
done:
    cairo_surface_destroy (group_surface);

    return group_pattern;
}
slim_hidden_def(cairo_pop_group);




























void
cairo_pop_group_to_source (cairo_t *cr)
{
    cairo_pattern_t *group_pattern;

    group_pattern = cairo_pop_group (cr);
    cairo_set_source (cr, group_pattern);
    cairo_pattern_destroy (group_pattern);
}
slim_hidden_def(cairo_pop_group_to_source);














void
cairo_set_operator (cairo_t *cr, cairo_operator_t op)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_operator (cr->gstate, op);
    if (status)
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_operator);
















void
cairo_set_source_rgb (cairo_t *cr, double red, double green, double blue)
{
    cairo_pattern_t *pattern;

    if (cr->status)
	return;

    
    cairo_set_source (cr, (cairo_pattern_t *) &cairo_pattern_none);

    pattern = cairo_pattern_create_rgb (red, green, blue);
    cairo_set_source (cr, pattern);
    cairo_pattern_destroy (pattern);
}

















void
cairo_set_source_rgba (cairo_t *cr,
		       double red, double green, double blue,
		       double alpha)
{
    cairo_pattern_t *pattern;

    if (cr->status)
	return;

    
    cairo_set_source (cr, (cairo_pattern_t *) &cairo_pattern_none);

    pattern = cairo_pattern_create_rgba (red, green, blue, alpha);
    cairo_set_source (cr, pattern);
    cairo_pattern_destroy (pattern);
}
























void
cairo_set_source_surface (cairo_t	  *cr,
			  cairo_surface_t *surface,
			  double	   x,
			  double	   y)
{
    cairo_pattern_t *pattern;
    cairo_matrix_t matrix;

    if (cr->status)
	return;

    
    cairo_set_source (cr, (cairo_pattern_t *) &cairo_pattern_none);

    pattern = cairo_pattern_create_for_surface (surface);

    cairo_matrix_init_translate (&matrix, -x, -y);
    cairo_pattern_set_matrix (pattern, &matrix);

    cairo_set_source (cr, pattern);
    cairo_pattern_destroy (pattern);
}
slim_hidden_def (cairo_set_source_surface);




















void
cairo_set_source (cairo_t *cr, cairo_pattern_t *source)
{
    cairo_status_t status;

    if (cr->status)
	return;

    if (source == NULL) {
	_cairo_set_error (cr, CAIRO_STATUS_NULL_POINTER);
	return;
    }

    if (source->status) {
	_cairo_set_error (cr, source->status);
	return;
    }

    status = _cairo_gstate_set_source (cr->gstate, source);
    if (status)
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_source);











cairo_pattern_t *
cairo_get_source (cairo_t *cr)
{
    if (cr->status)
	return (cairo_pattern_t*) &_cairo_pattern_nil.base;

    return _cairo_gstate_get_source (cr->gstate);
}














void
cairo_set_tolerance (cairo_t *cr, double tolerance)
{
    cairo_status_t status;

    if (cr->status)
	return;

    _cairo_restrict_value (&tolerance, CAIRO_TOLERANCE_MINIMUM, tolerance);

    status = _cairo_gstate_set_tolerance (cr->gstate, tolerance);
    if (status)
	_cairo_set_error (cr, status);
}














void
cairo_set_antialias (cairo_t *cr, cairo_antialias_t antialias)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_antialias (cr->gstate, antialias);
    if (status)
	_cairo_set_error (cr, status);
}












void
cairo_set_fill_rule (cairo_t *cr, cairo_fill_rule_t fill_rule)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_fill_rule (cr->gstate, fill_rule);
    if (status)
	_cairo_set_error (cr, status);
}



























void
cairo_set_line_width (cairo_t *cr, double width)
{
    cairo_status_t status;

    if (cr->status)
	return;

    _cairo_restrict_value (&width, 0.0, width);

    status = _cairo_gstate_set_line_width (cr->gstate, width);
    if (status)
	_cairo_set_error (cr, status);
}















void
cairo_set_line_cap (cairo_t *cr, cairo_line_cap_t line_cap)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_line_cap (cr->gstate, line_cap);
    if (status)
	_cairo_set_error (cr, status);
}















void
cairo_set_line_join (cairo_t *cr, cairo_line_join_t line_join)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_line_join (cr->gstate, line_join);
    if (status)
	_cairo_set_error (cr, status);
}

































void
cairo_set_dash (cairo_t	     *cr,
		const double *dashes,
		int	      num_dashes,
		double	      offset)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_dash (cr->gstate,
				     dashes, num_dashes, offset);
    if (status)
	_cairo_set_error (cr, status);
}














int
cairo_get_dash_count (cairo_t *cr)
{
    int num_dashes;

    _cairo_gstate_get_dash (cr->gstate, NULL, &num_dashes, NULL);

    return num_dashes;
}













void
cairo_get_dash (cairo_t *cr,
		double  *dashes,
		double  *offset)
{
    _cairo_gstate_get_dash (cr->gstate, dashes, NULL, offset);
}




















void
cairo_set_miter_limit (cairo_t *cr, double limit)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_miter_limit (cr->gstate, limit);
    if (status)
	_cairo_set_error (cr, status);
}













void
cairo_translate (cairo_t *cr, double tx, double ty)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_translate (cr->gstate, tx, ty);
    if (status)
	_cairo_set_error (cr, status);
}












void
cairo_scale (cairo_t *cr, double sx, double sy)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_scale (cr->gstate, sx, sy);
    if (status)
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_scale);













void
cairo_rotate (cairo_t *cr, double angle)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_rotate (cr->gstate, angle);
    if (status)
	_cairo_set_error (cr, status);
}










void
cairo_transform (cairo_t	      *cr,
		 const cairo_matrix_t *matrix)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_transform (cr->gstate, matrix);
    if (status)
	_cairo_set_error (cr, status);
}









void
cairo_set_matrix (cairo_t	       *cr,
		  const cairo_matrix_t *matrix)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_matrix (cr->gstate, matrix);
    if (status)
	_cairo_set_error (cr, status);
}










void
cairo_identity_matrix (cairo_t *cr)
{
    if (cr->status)
	return;

    _cairo_gstate_identity_matrix (cr->gstate);
}











void
cairo_user_to_device (cairo_t *cr, double *x, double *y)
{
    if (cr->status)
	return;

    _cairo_gstate_user_to_device (cr->gstate, x, y);
}












void
cairo_user_to_device_distance (cairo_t *cr, double *dx, double *dy)
{
    if (cr->status)
	return;

    _cairo_gstate_user_to_device_distance (cr->gstate, dx, dy);
}











void
cairo_device_to_user (cairo_t *cr, double *x, double *y)
{
    if (cr->status)
	return;

    _cairo_gstate_device_to_user (cr->gstate, x, y);
}












void
cairo_device_to_user_distance (cairo_t *cr, double *dx, double *dy)
{
    if (cr->status)
	return;

    _cairo_gstate_device_to_user_distance (cr->gstate, dx, dy);
}








void
cairo_new_path (cairo_t *cr)
{
    if (cr->status)
	return;

    _cairo_path_fixed_fini (cr->path);
}
slim_hidden_def(cairo_new_path);










void
cairo_move_to (cairo_t *cr, double x, double y)
{
    cairo_status_t status;
    cairo_fixed_t x_fixed, y_fixed;

    if (cr->status)
	return;

    _cairo_gstate_user_to_backend (cr->gstate, &x, &y);
    x_fixed = _cairo_fixed_from_double (x);
    y_fixed = _cairo_fixed_from_double (y);

    status = _cairo_path_fixed_move_to (cr->path, x_fixed, y_fixed);
    if (status)
	_cairo_set_error (cr, status);
}
slim_hidden_def(cairo_move_to);



















void
cairo_new_sub_path (cairo_t *cr)
{
    if (cr->status)
	return;

    _cairo_path_fixed_new_sub_path (cr->path);
}














void
cairo_line_to (cairo_t *cr, double x, double y)
{
    cairo_status_t status;
    cairo_fixed_t x_fixed, y_fixed;

    if (cr->status)
	return;

    _cairo_gstate_user_to_backend (cr->gstate, &x, &y);
    x_fixed = _cairo_fixed_from_double (x);
    y_fixed = _cairo_fixed_from_double (y);

    status = _cairo_path_fixed_line_to (cr->path, x_fixed, y_fixed);
    if (status)
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_line_to);




















void
cairo_curve_to (cairo_t *cr,
		double x1, double y1,
		double x2, double y2,
		double x3, double y3)
{
    cairo_status_t status;
    cairo_fixed_t x1_fixed, y1_fixed;
    cairo_fixed_t x2_fixed, y2_fixed;
    cairo_fixed_t x3_fixed, y3_fixed;

    if (cr->status)
	return;

    _cairo_gstate_user_to_backend (cr->gstate, &x1, &y1);
    _cairo_gstate_user_to_backend (cr->gstate, &x2, &y2);
    _cairo_gstate_user_to_backend (cr->gstate, &x3, &y3);

    x1_fixed = _cairo_fixed_from_double (x1);
    y1_fixed = _cairo_fixed_from_double (y1);

    x2_fixed = _cairo_fixed_from_double (x2);
    y2_fixed = _cairo_fixed_from_double (y2);

    x3_fixed = _cairo_fixed_from_double (x3);
    y3_fixed = _cairo_fixed_from_double (y3);

    status = _cairo_path_fixed_curve_to (cr->path,
					 x1_fixed, y1_fixed,
					 x2_fixed, y2_fixed,
					 x3_fixed, y3_fixed);
    if (status)
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_curve_to);















































void
cairo_arc (cairo_t *cr,
	   double xc, double yc,
	   double radius,
	   double angle1, double angle2)
{
    if (cr->status)
	return;

    
    if (radius <= 0.0)
	return;

    while (angle2 < angle1)
	angle2 += 2 * M_PI;

    cairo_line_to (cr,
		   xc + radius * cos (angle1),
		   yc + radius * sin (angle1));

    _cairo_arc_path (cr, xc, yc, radius,
		     angle1, angle2);
}



















void
cairo_arc_negative (cairo_t *cr,
		    double xc, double yc,
		    double radius,
		    double angle1, double angle2)
{
    if (cr->status)
	return;

    
    if (radius <= 0.0)
	return;

    while (angle2 > angle1)
	angle2 -= 2 * M_PI;

    cairo_line_to (cr,
		   xc + radius * cos (angle1),
		   yc + radius * sin (angle1));

     _cairo_arc_path_negative (cr, xc, yc, radius,
			       angle1, angle2);
}






































void
cairo_rel_move_to (cairo_t *cr, double dx, double dy)
{
    cairo_fixed_t dx_fixed, dy_fixed;
    cairo_status_t status;

    if (cr->status)
	return;

    _cairo_gstate_user_to_device_distance (cr->gstate, &dx, &dy);

    dx_fixed = _cairo_fixed_from_double (dx);
    dy_fixed = _cairo_fixed_from_double (dy);

    status = _cairo_path_fixed_rel_move_to (cr->path, dx_fixed, dy_fixed);
    if (status)
	_cairo_set_error (cr, status);
}



















void
cairo_rel_line_to (cairo_t *cr, double dx, double dy)
{
    cairo_fixed_t dx_fixed, dy_fixed;
    cairo_status_t status;

    if (cr->status)
	return;

    _cairo_gstate_user_to_device_distance (cr->gstate, &dx, &dy);

    dx_fixed = _cairo_fixed_from_double (dx);
    dy_fixed = _cairo_fixed_from_double (dy);

    status = _cairo_path_fixed_rel_line_to (cr->path, dx_fixed, dy_fixed);
    if (status)
	_cairo_set_error (cr, status);
}
slim_hidden_def(cairo_rel_line_to);



























void
cairo_rel_curve_to (cairo_t *cr,
		    double dx1, double dy1,
		    double dx2, double dy2,
		    double dx3, double dy3)
{
    cairo_fixed_t dx1_fixed, dy1_fixed;
    cairo_fixed_t dx2_fixed, dy2_fixed;
    cairo_fixed_t dx3_fixed, dy3_fixed;
    cairo_status_t status;

    if (cr->status)
	return;

    _cairo_gstate_user_to_device_distance (cr->gstate, &dx1, &dy1);
    _cairo_gstate_user_to_device_distance (cr->gstate, &dx2, &dy2);
    _cairo_gstate_user_to_device_distance (cr->gstate, &dx3, &dy3);

    dx1_fixed = _cairo_fixed_from_double (dx1);
    dy1_fixed = _cairo_fixed_from_double (dy1);

    dx2_fixed = _cairo_fixed_from_double (dx2);
    dy2_fixed = _cairo_fixed_from_double (dy2);

    dx3_fixed = _cairo_fixed_from_double (dx3);
    dy3_fixed = _cairo_fixed_from_double (dy3);

    status = _cairo_path_fixed_rel_curve_to (cr->path,
					     dx1_fixed, dy1_fixed,
					     dx2_fixed, dy2_fixed,
					     dx3_fixed, dy3_fixed);
    if (status)
	_cairo_set_error (cr, status);
}





















void
cairo_rectangle (cairo_t *cr,
		 double x, double y,
		 double width, double height)
{
    if (cr->status)
	return;

    cairo_move_to (cr, x, y);
    cairo_rel_line_to (cr, width, 0);
    cairo_rel_line_to (cr, 0, height);
    cairo_rel_line_to (cr, -width, 0);
    cairo_close_path (cr);
}










































void
cairo_close_path (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_path_fixed_close_path (cr->path);
    if (status)
	_cairo_set_error (cr, status);
}
slim_hidden_def(cairo_close_path);








void
cairo_paint (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_paint (cr->gstate);
    if (status)
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_paint);











void
cairo_paint_with_alpha (cairo_t *cr,
			double   alpha)
{
    cairo_status_t status;
    cairo_color_t color;
    cairo_pattern_union_t pattern;

    if (cr->status)
	return;

    if (CAIRO_ALPHA_IS_OPAQUE (alpha)) {
	cairo_paint (cr);
	return;
    }

    if (CAIRO_ALPHA_IS_ZERO (alpha)) {
	return;
    }

    _cairo_color_init_rgba (&color, 1., 1., 1., alpha);
    _cairo_pattern_init_solid (&pattern.solid, &color, CAIRO_CONTENT_ALPHA);

    status = _cairo_gstate_mask (cr->gstate, &pattern.base);
    if (status)
	_cairo_set_error (cr, status);

    _cairo_pattern_fini (&pattern.base);
}











void
cairo_mask (cairo_t         *cr,
	    cairo_pattern_t *pattern)
{
    cairo_status_t status;

    if (cr->status)
	return;

    if (pattern == NULL) {
	_cairo_set_error (cr, CAIRO_STATUS_NULL_POINTER);
	return;
    }

    if (pattern->status) {
	_cairo_set_error (cr, pattern->status);
	return;
    }

    status = _cairo_gstate_mask (cr->gstate, pattern);
    if (status)
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_mask);













void
cairo_mask_surface (cairo_t         *cr,
		    cairo_surface_t *surface,
		    double           surface_x,
		    double           surface_y)
{
    cairo_pattern_t *pattern;
    cairo_matrix_t matrix;

    if (cr->status)
	return;

    pattern = cairo_pattern_create_for_surface (surface);

    cairo_matrix_init_translate (&matrix, - surface_x, - surface_y);
    cairo_pattern_set_matrix (pattern, &matrix);

    cairo_mask (cr, pattern);

    cairo_pattern_destroy (pattern);
}

































void
cairo_stroke (cairo_t *cr)
{
    cairo_stroke_preserve (cr);

    cairo_new_path (cr);
}














void
cairo_stroke_preserve (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_stroke (cr->gstate, cr->path);
    if (status)
	_cairo_set_error (cr, status);
}
slim_hidden_def(cairo_stroke_preserve);











void
cairo_fill (cairo_t *cr)
{
    cairo_fill_preserve (cr);

    cairo_new_path (cr);
}












void
cairo_fill_preserve (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_fill (cr->gstate, cr->path);
    if (status)
	_cairo_set_error (cr, status);
}
slim_hidden_def(cairo_fill_preserve);










void
cairo_copy_page (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_copy_page (cr->gstate);
    if (status)
	_cairo_set_error (cr, status);
}








void
cairo_show_page (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_show_page (cr->gstate);
    if (status)
	_cairo_set_error (cr, status);
}



















cairo_bool_t
cairo_in_stroke (cairo_t *cr, double x, double y)
{
    cairo_status_t status;
    cairo_bool_t inside = FALSE;

    if (cr->status)
	return 0;

    status = _cairo_gstate_in_stroke (cr->gstate,
				      cr->path,
				      x, y, &inside);
    if (status)
	_cairo_set_error (cr, status);

    return inside;
}

















cairo_bool_t
cairo_in_fill (cairo_t *cr, double x, double y)
{
    cairo_status_t status;
    cairo_bool_t inside = FALSE;

    if (cr->status)
	return 0;

    status = _cairo_gstate_in_fill (cr->gstate,
				    cr->path,
				    x, y, &inside);
    if (status)
	_cairo_set_error (cr, status);

    return inside;
}



















void
cairo_stroke_extents (cairo_t *cr,
                      double *x1, double *y1, double *x2, double *y2)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_stroke_extents (cr->gstate,
					   cr->path,
					   x1, y1, x2, y2);
    if (status)
	_cairo_set_error (cr, status);
}

















void
cairo_fill_extents (cairo_t *cr,
                    double *x1, double *y1, double *x2, double *y2)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_fill_extents (cr->gstate,
					 cr->path,
					 x1, y1, x2, y2);
    if (status)
	_cairo_set_error (cr, status);
}























void
cairo_clip (cairo_t *cr)
{
    cairo_clip_preserve (cr);

    cairo_new_path (cr);
}























void
cairo_clip_preserve (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_clip (cr->gstate, cr->path);
    if (status)
	_cairo_set_error (cr, status);
}
slim_hidden_def(cairo_clip_preserve);

















void
cairo_reset_clip (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_reset_clip (cr->gstate);
    if (status)
	_cairo_set_error (cr, status);
}














void
cairo_clip_extents (cairo_t *cr,
		    double *x1, double *y1,
		    double *x2, double *y2)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_clip_extents (cr->gstate, x1, y1, x2, y2);
    if (status)
	_cairo_set_error (cr, status);
}

static cairo_rectangle_list_t *
_cairo_rectangle_list_create_in_error (cairo_status_t status)
{
    cairo_rectangle_list_t *list;

    if (status == CAIRO_STATUS_NO_MEMORY)
        return (cairo_rectangle_list_t*) &_cairo_rectangles_nil;

    list = malloc (sizeof (cairo_rectangle_list_t));
    if (list == NULL)
        return (cairo_rectangle_list_t*) &_cairo_rectangles_nil;
    list->status = status;
    list->rectangles = NULL;
    list->num_rectangles = 0;
    return list;
}




















cairo_rectangle_list_t *
cairo_copy_clip_rectangle_list (cairo_t *cr)
{
    if (cr->status)
        return _cairo_rectangle_list_create_in_error (cr->status);

    return _cairo_gstate_copy_clip_rectangle_list (cr->gstate);
}
















void
cairo_select_font_face (cairo_t              *cr,
			const char           *family,
			cairo_font_slant_t    slant,
			cairo_font_weight_t   weight)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_select_font_face (cr->gstate, family, slant, weight);
    if (status)
	_cairo_set_error (cr, status);
}









void
cairo_font_extents (cairo_t              *cr,
		    cairo_font_extents_t *extents)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_get_font_extents (cr->gstate, extents);
    if (status)
	_cairo_set_error (cr, status);
}










void
cairo_set_font_face (cairo_t           *cr,
		     cairo_font_face_t *font_face)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_font_face (cr->gstate, font_face);
    if (status)
	_cairo_set_error (cr, status);
}



















cairo_font_face_t *
cairo_get_font_face (cairo_t *cr)
{
    cairo_status_t status;
    cairo_font_face_t *font_face;

    if (cr->status)
	return (cairo_font_face_t*) &_cairo_font_face_nil;

    status = _cairo_gstate_get_font_face (cr->gstate, &font_face);
    if (status) {
	_cairo_set_error (cr, status);
	return (cairo_font_face_t*) &_cairo_font_face_nil;
    }

    return font_face;
}












void
cairo_set_font_size (cairo_t *cr, double size)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_font_size (cr->gstate, size);
    if (status)
	_cairo_set_error (cr, status);
}














void
cairo_set_font_matrix (cairo_t		    *cr,
		       const cairo_matrix_t *matrix)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_font_matrix (cr->gstate, matrix);
    if (status)
	_cairo_set_error (cr, status);
}









void
cairo_get_font_matrix (cairo_t *cr, cairo_matrix_t *matrix)
{
    _cairo_gstate_get_font_matrix (cr->gstate, matrix);
}












void
cairo_set_font_options (cairo_t                    *cr,
			const cairo_font_options_t *options)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = cairo_font_options_status ((cairo_font_options_t *) options);
    if (status) {
	_cairo_set_error (cr, status);
	return;
    }

    _cairo_gstate_set_font_options (cr->gstate, options);
}












void
cairo_get_font_options (cairo_t              *cr,
			cairo_font_options_t *options)
{
    
    if (cairo_font_options_status (options))
	return;

    _cairo_gstate_get_font_options (cr->gstate, options);
}














void
cairo_set_scaled_font (cairo_t                   *cr,
		       const cairo_scaled_font_t *scaled_font)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = scaled_font->status;
    if (status)
        goto BAIL;

    status = _cairo_gstate_set_font_face (cr->gstate, scaled_font->font_face);
    if (status)
        goto BAIL;

    status = _cairo_gstate_set_font_matrix (cr->gstate, &scaled_font->font_matrix);
    if (status)
        goto BAIL;

    _cairo_gstate_set_font_options (cr->gstate, &scaled_font->options);

    return;

BAIL:
    _cairo_set_error (cr, status);
}





















cairo_scaled_font_t *
cairo_get_scaled_font (cairo_t *cr)
{
    cairo_status_t status;
    cairo_scaled_font_t *scaled_font;

    if (cr->status)
	return (cairo_scaled_font_t *)&_cairo_scaled_font_nil;

    status = _cairo_gstate_get_scaled_font (cr->gstate, &scaled_font);
    if (status) {
	_cairo_set_error (cr, status);
	return (cairo_scaled_font_t *)&_cairo_scaled_font_nil;
    }

    return scaled_font;
}





















void
cairo_text_extents (cairo_t              *cr,
		    const char		 *utf8,
		    cairo_text_extents_t *extents)
{
    cairo_status_t status;
    cairo_glyph_t *glyphs = NULL;
    int num_glyphs;
    double x, y;

    if (cr->status)
	return;

    if (utf8 == NULL) {
	extents->x_bearing = 0.0;
	extents->y_bearing = 0.0;
	extents->width = 0.0;
	extents->height = 0.0;
	extents->x_advance = 0.0;
	extents->y_advance = 0.0;
	return;
    }

    cairo_get_current_point (cr, &x, &y);

    status = _cairo_gstate_text_to_glyphs (cr->gstate, utf8,
					   x, y,
					   &glyphs, &num_glyphs);

    if (status) {
	if (glyphs)
	    free (glyphs);
	_cairo_set_error (cr, status);
	return;
    }

    status = _cairo_gstate_glyph_extents (cr->gstate, glyphs, num_glyphs, extents);
    if (glyphs)
	free (glyphs);

    if (status)
	_cairo_set_error (cr, status);
}



















void
cairo_glyph_extents (cairo_t                *cr,
		     const cairo_glyph_t    *glyphs,
		     int                    num_glyphs,
		     cairo_text_extents_t   *extents)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_glyph_extents (cr->gstate, glyphs, num_glyphs,
					  extents);
    if (status)
	_cairo_set_error (cr, status);
}




























void
cairo_show_text (cairo_t *cr, const char *utf8)
{
    cairo_text_extents_t extents;
    cairo_status_t status;
    cairo_glyph_t *glyphs = NULL, *last_glyph;
    int num_glyphs;
    double x, y;

    if (cr->status)
	return;

    if (utf8 == NULL)
	return;

    cairo_get_current_point (cr, &x, &y);

    status = _cairo_gstate_text_to_glyphs (cr->gstate, utf8,
					       x, y,
					       &glyphs, &num_glyphs);
    if (status)
	goto BAIL;

    if (num_glyphs == 0)
	return;

    status = _cairo_gstate_show_glyphs (cr->gstate, glyphs, num_glyphs);
    if (status)
	goto BAIL;

    last_glyph = &glyphs[num_glyphs - 1];
    status = _cairo_gstate_glyph_extents (cr->gstate,
					  last_glyph, 1,
					  &extents);
    if (status)
	goto BAIL;

    x = last_glyph->x + extents.x_advance;
    y = last_glyph->y + extents.y_advance;
    cairo_move_to (cr, x, y);

 BAIL:
    if (glyphs)
	free (glyphs);

    if (status)
	_cairo_set_error (cr, status);
}











void
cairo_show_glyphs (cairo_t *cr, const cairo_glyph_t *glyphs, int num_glyphs)
{
    cairo_status_t status;

    if (cr->status)
	return;

    if (num_glyphs == 0)
	return;

    status = _cairo_gstate_show_glyphs (cr->gstate, glyphs, num_glyphs);
    if (status)
	_cairo_set_error (cr, status);
}

























void
cairo_text_path  (cairo_t *cr, const char *utf8)
{
    cairo_status_t status;
    cairo_text_extents_t extents;
    cairo_glyph_t *glyphs = NULL, *last_glyph;
    int num_glyphs;
    double x, y;

    if (cr->status)
	return;

    cairo_get_current_point (cr, &x, &y);

    status = _cairo_gstate_text_to_glyphs (cr->gstate, utf8,
					   x, y,
					   &glyphs, &num_glyphs);

    if (status)
	goto BAIL;

    if (num_glyphs == 0)
	return;

    status = _cairo_gstate_glyph_path (cr->gstate,
				       glyphs, num_glyphs,
				       cr->path);

    if (status)
	goto BAIL;

    last_glyph = &glyphs[num_glyphs - 1];
    status = _cairo_gstate_glyph_extents (cr->gstate,
					  last_glyph, 1,
					  &extents);

    if (status)
	goto BAIL;

    x = last_glyph->x + extents.x_advance;
    y = last_glyph->y + extents.y_advance;
    cairo_move_to (cr, x, y);

 BAIL:
    if (glyphs)
	free (glyphs);

    if (status)
	_cairo_set_error (cr, status);
}











void
cairo_glyph_path (cairo_t *cr, const cairo_glyph_t *glyphs, int num_glyphs)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_glyph_path (cr->gstate,
				       glyphs, num_glyphs,
				       cr->path);
    if (status)
	_cairo_set_error (cr, status);
}









cairo_operator_t
cairo_get_operator (cairo_t *cr)
{
    return _cairo_gstate_get_operator (cr->gstate);
}









double
cairo_get_tolerance (cairo_t *cr)
{
    return _cairo_gstate_get_tolerance (cr->gstate);
}
slim_hidden_def (cairo_get_tolerance);









cairo_antialias_t
cairo_get_antialias (cairo_t *cr)
{
    return _cairo_gstate_get_antialias (cr->gstate);
}






























void
cairo_get_current_point (cairo_t *cr, double *x_ret, double *y_ret)
{
    cairo_status_t status;
    cairo_fixed_t x_fixed, y_fixed;
    double x, y;

    status = _cairo_path_fixed_get_current_point (cr->path, &x_fixed, &y_fixed);
    if (status == CAIRO_STATUS_NO_CURRENT_POINT) {
	x = 0.0;
	y = 0.0;
    } else {
	x = _cairo_fixed_to_double (x_fixed);
	y = _cairo_fixed_to_double (y_fixed);
	_cairo_gstate_backend_to_user (cr->gstate, &x, &y);
    }

    if (x_ret)
	*x_ret = x;
    if (y_ret)
	*y_ret = y;
}
slim_hidden_def(cairo_get_current_point);









cairo_fill_rule_t
cairo_get_fill_rule (cairo_t *cr)
{
    return _cairo_gstate_get_fill_rule (cr->gstate);
}












double
cairo_get_line_width (cairo_t *cr)
{
    return _cairo_gstate_get_line_width (cr->gstate);
}









cairo_line_cap_t
cairo_get_line_cap (cairo_t *cr)
{
    return _cairo_gstate_get_line_cap (cr->gstate);
}









cairo_line_join_t
cairo_get_line_join (cairo_t *cr)
{
    return _cairo_gstate_get_line_join (cr->gstate);
}









double
cairo_get_miter_limit (cairo_t *cr)
{
    return _cairo_gstate_get_miter_limit (cr->gstate);
}








void
cairo_get_matrix (cairo_t *cr, cairo_matrix_t *matrix)
{
    _cairo_gstate_get_matrix (cr->gstate, matrix);
}
slim_hidden_def (cairo_get_matrix);

















cairo_surface_t *
cairo_get_target (cairo_t *cr)
{
    if (cr->status)
	return (cairo_surface_t*) &_cairo_surface_nil;

    return _cairo_gstate_get_original_target (cr->gstate);
}



















cairo_surface_t *
cairo_get_group_target (cairo_t *cr)
{
    if (cr->status)
	return (cairo_surface_t*) &_cairo_surface_nil;

    return _cairo_gstate_get_target (cr->gstate);
}































cairo_path_t *
cairo_copy_path (cairo_t *cr)
{
    if (cr->status)
	return _cairo_path_create_in_error (cr->status);

    return _cairo_path_create (cr->path, cr->gstate);
}


































cairo_path_t *
cairo_copy_path_flat (cairo_t *cr)
{
    if (cr->status)
	return _cairo_path_create_in_error (cr->status);

    return _cairo_path_create_flat (cr->path, cr->gstate);
}













void
cairo_append_path (cairo_t		*cr,
		   const cairo_path_t	*path)
{
    cairo_status_t status;

    if (cr->status)
	return;

    if (path == NULL) {
	_cairo_set_error (cr, CAIRO_STATUS_NULL_POINTER);
	return;
    }

    if (path->status) {
	if (path->status > CAIRO_STATUS_SUCCESS &&
	    path->status <= CAIRO_STATUS_LAST_STATUS)
	    _cairo_set_error (cr, path->status);
	else
	    _cairo_set_error (cr, CAIRO_STATUS_INVALID_STATUS);
	return;
    }

    if (path->data == NULL) {
	_cairo_set_error (cr, CAIRO_STATUS_NULL_POINTER);
	return;
    }

    status = _cairo_path_append_to_context (path, cr);
    if (status)
	_cairo_set_error (cr, status);
}









cairo_status_t
cairo_status (cairo_t *cr)
{
    return cr->status;
}
slim_hidden_def (cairo_status);









const char *
cairo_status_to_string (cairo_status_t status)
{
    switch (status) {
    case CAIRO_STATUS_SUCCESS:
	return "success";
    case CAIRO_STATUS_NO_MEMORY:
	return "out of memory";
    case CAIRO_STATUS_INVALID_RESTORE:
	return "cairo_restore without matching cairo_save";
    case CAIRO_STATUS_INVALID_POP_GROUP:
	return "cairo_pop_group without matching cairo_push_group";
    case CAIRO_STATUS_NO_CURRENT_POINT:
	return "no current point defined";
    case CAIRO_STATUS_INVALID_MATRIX:
	return "invalid matrix (not invertible)";
    case CAIRO_STATUS_INVALID_STATUS:
	return "invalid value for an input cairo_status_t";
    case CAIRO_STATUS_NULL_POINTER:
	return "NULL pointer";
    case CAIRO_STATUS_INVALID_STRING:
	return "input string not valid UTF-8";
    case CAIRO_STATUS_INVALID_PATH_DATA:
	return "input path data not valid";
    case CAIRO_STATUS_READ_ERROR:
	return "error while reading from input stream";
    case CAIRO_STATUS_WRITE_ERROR:
	return "error while writing to output stream";
    case CAIRO_STATUS_SURFACE_FINISHED:
	return "the target surface has been finished";
    case CAIRO_STATUS_SURFACE_TYPE_MISMATCH:
	return "the surface type is not appropriate for the operation";
    case CAIRO_STATUS_PATTERN_TYPE_MISMATCH:
	return "the pattern type is not appropriate for the operation";
    case CAIRO_STATUS_INVALID_CONTENT:
	return "invalid value for an input cairo_content_t";
    case CAIRO_STATUS_INVALID_FORMAT:
	return "invalid value for an input cairo_format_t";
    case CAIRO_STATUS_INVALID_VISUAL:
	return "invalid value for an input Visual*";
    case CAIRO_STATUS_FILE_NOT_FOUND:
	return "file not found";
    case CAIRO_STATUS_INVALID_DASH:
	return "invalid value for a dash setting";
    case CAIRO_STATUS_INVALID_DSC_COMMENT:
	return "invalid value for a DSC comment";
    case CAIRO_STATUS_INVALID_INDEX:
	return "invalid index passed to getter";
    case CAIRO_STATUS_CLIP_NOT_REPRESENTABLE:
        return "clip region not representable in desired format";
    }

    return "<unknown error status>";
}

void
_cairo_restrict_value (double *value, double min, double max)
{
    if (*value < min)
	*value = min;
    else if (*value > max)
	*value = max;
}




















int
_cairo_lround (double d)
{
    uint32_t top, shift_amount, output;
    union {
        double d;
        uint64_t ui64;
        uint32_t ui32[2];
    } u;

    u.d = d;

    








#if ( defined(FLOAT_WORDS_BIGENDIAN) && !defined(WORDS_BIGENDIAN)) || \
    (!defined(FLOAT_WORDS_BIGENDIAN) &&  defined(WORDS_BIGENDIAN))
    {
        uint32_t temp = u.ui32[0];
        u.ui32[0] = u.ui32[1];
        u.ui32[1] = temp;
    }
#endif

#ifdef WORDS_BIGENDIAN
    #define MSW (0) /* Most Significant Word */
    #define LSW (1) /* Least Significant Word */
#else
    #define MSW (1)
    #define LSW (0)
#endif

    



    top = u.ui32[MSW] >> 20;

    



















    shift_amount = 1053 - (top & 0x7FF);

    


    top >>= 11;

    




    u.ui32[MSW] |= 0x100000;

    





    u.ui64 -= top;

    



    top--;

    







    output = (u.ui32[MSW] << 11) | (u.ui32[LSW] >> 21);

    




















    output >>= shift_amount;

    




    output = (output >> 1) + (output & 1);

    
















    output &= ((shift_amount > 31) - 1);

    

































    output = (output & top) - (output & ~top);

    return output;
#undef MSW
#undef LSW
}
