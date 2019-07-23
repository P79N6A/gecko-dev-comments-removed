





































#include "cairoint.h"
#include "cairo-private.h"

#include "cairo-arc-private.h"
#include "cairo-path-private.h"

#define CAIRO_TOLERANCE_MINIMUM	_cairo_fixed_to_double(1)

static const cairo_t _cairo_nil = {
  CAIRO_REFERENCE_COUNT_INVALID,	
  CAIRO_STATUS_NO_MEMORY,	
  { 0, 0, 0, NULL },		
  NULL,				
  {{ 0 }, { 0 }},		
  NULL,				
  {{				
    { 0, 0 },			
    { 0, 0 },			
    FALSE,			
    FALSE,			
    NULL, {{NULL}}		
  }}
};

#include <assert.h>


















cairo_status_t
_cairo_error (cairo_status_t status)
{
    CAIRO_ENSURE_UNIQUE;
    assert (_cairo_status_is_error (status));

    return status;
}

















static void
_cairo_set_error (cairo_t *cr, cairo_status_t status)
{
    if (status == CAIRO_STATUS_SUCCESS)
	return;

    

    _cairo_status_set_error (&cr->status, status);

    status = _cairo_error (status);
}
























cairo_t *
cairo_create (cairo_surface_t *target)
{
    cairo_t *cr;
    cairo_status_t status;

    
    if (target && target->status == CAIRO_STATUS_NO_MEMORY)
	return (cairo_t *) &_cairo_nil;

    cr = malloc (sizeof (cairo_t));
    if (unlikely (cr == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	return (cairo_t *) &_cairo_nil;
    }

    CAIRO_REFERENCE_COUNT_INIT (&cr->ref_count, 1);

    cr->status = CAIRO_STATUS_SUCCESS;

    _cairo_user_data_array_init (&cr->user_data);
    _cairo_path_fixed_init (cr->path);

    cr->gstate = &cr->gstate_tail[0];
    cr->gstate_freelist = &cr->gstate_tail[1];
    cr->gstate_tail[1].next = NULL;

    status = _cairo_gstate_init (cr->gstate, target);
    if (unlikely (status))
	_cairo_set_error (cr, status);

    return cr;
}
slim_hidden_def (cairo_create);














cairo_t *
cairo_reference (cairo_t *cr)
{
    if (cr == NULL || CAIRO_REFERENCE_COUNT_IS_INVALID (&cr->ref_count))
	return cr;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&cr->ref_count));

    _cairo_reference_count_inc (&cr->ref_count);

    return cr;
}









void
cairo_destroy (cairo_t *cr)
{
    cairo_surface_t *surface;

    if (cr == NULL || CAIRO_REFERENCE_COUNT_IS_INVALID (&cr->ref_count))
	return;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&cr->ref_count));

    if (! _cairo_reference_count_dec_and_test (&cr->ref_count))
	return;

    while (cr->gstate != &cr->gstate_tail[0]) {
	if (_cairo_gstate_restore (&cr->gstate, &cr->gstate_freelist))
	    break;
    }

    




    surface = _cairo_gstate_get_original_target (cr->gstate);
    if (surface != NULL)
	cairo_surface_flush (surface);

    _cairo_gstate_fini (cr->gstate);
    cr->gstate_freelist = cr->gstate_freelist->next; 
    while (cr->gstate_freelist != NULL) {
	cairo_gstate_t *gstate = cr->gstate_freelist;
	cr->gstate_freelist = gstate->next;
	free (gstate);
    }

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
    if (CAIRO_REFERENCE_COUNT_IS_INVALID (&cr->ref_count))
	return cr->status;

    return _cairo_user_data_array_set_data (&cr->user_data,
					    key, user_data, destroy);
}












unsigned int
cairo_get_reference_count (cairo_t *cr)
{
    if (cr == NULL || CAIRO_REFERENCE_COUNT_IS_INVALID (&cr->ref_count))
	return 0;

    return CAIRO_REFERENCE_COUNT_GET_VALUE (&cr->ref_count);
}

















void
cairo_save (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_save (&cr->gstate, &cr->gstate_freelist);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
slim_hidden_def(cairo_save);









void
cairo_restore (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_restore (&cr->gstate, &cr->gstate_freelist);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
slim_hidden_def(cairo_restore);













































void
cairo_push_group (cairo_t *cr)
{
    cairo_push_group_with_content (cr, CAIRO_CONTENT_COLOR_ALPHA);
}




















void
cairo_push_group_with_content (cairo_t *cr, cairo_content_t content)
{
    cairo_status_t status;
    cairo_rectangle_int_t extents;
    cairo_surface_t *parent_surface, *group_surface = NULL;

    if (cr->status)
	return;

    parent_surface = _cairo_gstate_get_target (cr->gstate);
    
    status = _cairo_surface_get_extents (parent_surface, &extents);
    if (unlikely (status))
	goto bail;
    status = _cairo_clip_intersect_to_rectangle (_cairo_gstate_get_clip (cr->gstate), &extents);
    if (unlikely (status))
	goto bail;

    group_surface = cairo_surface_create_similar (_cairo_gstate_get_target (cr->gstate),
						  content,
						  extents.width,
						  extents.height);
    status = cairo_surface_status (group_surface);
    if (unlikely (status))
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
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
slim_hidden_def(cairo_push_group_with_content);






















cairo_pattern_t *
cairo_pop_group (cairo_t *cr)
{
    cairo_surface_t *group_surface, *parent_target;
    cairo_pattern_t *group_pattern = (cairo_pattern_t*) &_cairo_pattern_nil.base;
    cairo_matrix_t group_matrix;

    if (cr->status)
	return group_pattern;

    
    group_surface = _cairo_gstate_get_target (cr->gstate);
    parent_target = _cairo_gstate_get_parent_target (cr->gstate);

    
    if (parent_target == NULL) {
	_cairo_set_error (cr, CAIRO_STATUS_INVALID_POP_GROUP);
	return group_pattern;
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
    


    if (_cairo_surface_has_device_transform (group_surface)) {
	cairo_pattern_set_matrix (group_pattern, &group_surface->device_transform);
	_cairo_pattern_transform (group_pattern, &group_matrix);
	_cairo_pattern_transform (group_pattern, &group_surface->device_transform_inverse);
    } else {
	cairo_pattern_set_matrix (group_pattern, &group_matrix);
    }

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












void
cairo_set_operator (cairo_t *cr, cairo_operator_t op)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_operator (cr->gstate, op);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_operator);


static cairo_bool_t
_current_source_matches_solid (cairo_t *cr,
			       double red,
			       double green,
			       double blue,
			       double alpha)
{
    const cairo_pattern_t *current;
    cairo_color_t color;

    current = cr->gstate->source;
    if (current->type != CAIRO_PATTERN_TYPE_SOLID)
	return FALSE;

    red   = _cairo_restrict_value (red,   0.0, 1.0);
    green = _cairo_restrict_value (green, 0.0, 1.0);
    blue  = _cairo_restrict_value (blue,  0.0, 1.0);
    alpha = _cairo_restrict_value (alpha, 0.0, 1.0);

    _cairo_color_init_rgba (&color, red, green, blue, alpha);
    return _cairo_color_equal (&color,
			       &((cairo_solid_pattern_t *) current)->color);
}


















void
cairo_set_source_rgb (cairo_t *cr, double red, double green, double blue)
{
    cairo_pattern_t *pattern;

    if (cr->status)
	return;

    if (_current_source_matches_solid (cr, red, green, blue, 1.))
	return;

    
    cairo_set_source (cr, (cairo_pattern_t *) &_cairo_pattern_black);

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

    if (_current_source_matches_solid (cr, red, green, blue, alpha))
	return;

    
    cairo_set_source (cr, (cairo_pattern_t *) &_cairo_pattern_black);

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

    
    cairo_set_source (cr, (cairo_pattern_t *) &_cairo_pattern_black);

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
    if (unlikely (status))
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

    if (tolerance < CAIRO_TOLERANCE_MINIMUM)
	tolerance = CAIRO_TOLERANCE_MINIMUM;

    status = _cairo_gstate_set_tolerance (cr->gstate, tolerance);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_tolerance);














void
cairo_set_antialias (cairo_t *cr, cairo_antialias_t antialias)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_antialias (cr->gstate, antialias);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}














void
cairo_set_fill_rule (cairo_t *cr, cairo_fill_rule_t fill_rule)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_fill_rule (cr->gstate, fill_rule);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}



























void
cairo_set_line_width (cairo_t *cr, double width)
{
    cairo_status_t status;

    if (cr->status)
	return;

    if (width < 0.)
	width = 0.;

    status = _cairo_gstate_set_line_width (cr->gstate, width);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_line_width);

















void
cairo_set_line_cap (cairo_t *cr, cairo_line_cap_t line_cap)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_line_cap (cr->gstate, line_cap);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_line_cap);

















void
cairo_set_line_join (cairo_t *cr, cairo_line_join_t line_join)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_line_join (cr->gstate, line_join);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_line_join);

































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
    if (unlikely (status))
	_cairo_set_error (cr, status);
}














int
cairo_get_dash_count (cairo_t *cr)
{
    int num_dashes;

    if (cr->status)
	return 0;

    _cairo_gstate_get_dash (cr->gstate, NULL, &num_dashes, NULL);

    return num_dashes;
}













void
cairo_get_dash (cairo_t *cr,
		double  *dashes,
		double  *offset)
{
    if (cr->status)
	return;

    _cairo_gstate_get_dash (cr->gstate, dashes, NULL, offset);
}





























void
cairo_set_miter_limit (cairo_t *cr, double limit)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_miter_limit (cr->gstate, limit);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}













void
cairo_translate (cairo_t *cr, double tx, double ty)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_translate (cr->gstate, tx, ty);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_translate);












void
cairo_scale (cairo_t *cr, double sx, double sy)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_scale (cr->gstate, sx, sy);
    if (unlikely (status))
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
    if (unlikely (status))
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
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_transform);









void
cairo_set_matrix (cairo_t	       *cr,
		  const cairo_matrix_t *matrix)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_matrix (cr->gstate, matrix);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_matrix);










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
slim_hidden_def (cairo_user_to_device);












void
cairo_user_to_device_distance (cairo_t *cr, double *dx, double *dy)
{
    if (cr->status)
	return;

    _cairo_gstate_user_to_device_distance (cr->gstate, dx, dy);
}
slim_hidden_def (cairo_user_to_device_distance);











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
    if (unlikely (status))
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
    if (unlikely (status))
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
    if (unlikely (status))
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
    if (unlikely (status))
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
    if (unlikely (status))
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
    if (unlikely (status))
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

#if 0

void
cairo_stroke_to_path (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    

    status = _cairo_gstate_stroke_path (cr->gstate);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
#endif



























void
cairo_close_path (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_path_fixed_close_path (cr->path);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
slim_hidden_def(cairo_close_path);
































void
cairo_path_extents (cairo_t *cr,
		    double *x1, double *y1, double *x2, double *y2)
{
    if (cr->status) {
	if (x1)
	    *x1 = 0.0;
	if (y1)
	    *y1 = 0.0;
	if (x2)
	    *x2 = 0.0;
	if (y2)
	    *y2 = 0.0;

	return;
    }

    _cairo_gstate_path_extents (cr->gstate,
				cr->path,
				x1, y1, x2, y2);
}








void
cairo_paint (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_paint (cr->gstate);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_paint);











void
cairo_paint_with_alpha (cairo_t *cr,
			double   alpha)
{
    cairo_status_t status;
    cairo_color_t color;
    cairo_solid_pattern_t pattern;

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
    _cairo_pattern_init_solid (&pattern, &color, CAIRO_CONTENT_ALPHA);

    status = _cairo_gstate_mask (cr->gstate, &pattern.base);
    if (unlikely (status))
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
    if (unlikely (status))
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
slim_hidden_def(cairo_stroke);














void
cairo_stroke_preserve (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_stroke (cr->gstate, cr->path);
    if (unlikely (status))
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
    if (unlikely (status))
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
    if (unlikely (status))
	_cairo_set_error (cr, status);
}











void
cairo_show_page (cairo_t *cr)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_show_page (cr->gstate);
    if (unlikely (status))
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
    if (unlikely (status))
	_cairo_set_error (cr, status);

    return inside;
}

















cairo_bool_t
cairo_in_fill (cairo_t *cr, double x, double y)
{
    cairo_bool_t inside;

    if (cr->status)
	return 0;

    _cairo_gstate_in_fill (cr->gstate,
			   cr->path,
			   x, y, &inside);

    return inside;
}






























void
cairo_stroke_extents (cairo_t *cr,
                      double *x1, double *y1, double *x2, double *y2)
{
    cairo_status_t status;

    if (cr->status) {
	if (x1)
	    *x1 = 0.0;
	if (y1)
	    *y1 = 0.0;
	if (x2)
	    *x2 = 0.0;
	if (y2)
	    *y2 = 0.0;

	return;
    }

    status = _cairo_gstate_stroke_extents (cr->gstate,
					   cr->path,
					   x1, y1, x2, y2);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}


























void
cairo_fill_extents (cairo_t *cr,
                    double *x1, double *y1, double *x2, double *y2)
{
    cairo_status_t status;

    if (cr->status) {
	if (x1)
	    *x1 = 0.0;
	if (y1)
	    *y1 = 0.0;
	if (x2)
	    *x2 = 0.0;
	if (y2)
	    *y2 = 0.0;

	return;
    }

    status = _cairo_gstate_fill_extents (cr->gstate,
					 cr->path,
					 x1, y1, x2, y2);
    if (unlikely (status))
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
    if (unlikely (status))
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
    if (unlikely (status))
	_cairo_set_error (cr, status);
}














void
cairo_clip_extents (cairo_t *cr,
		    double *x1, double *y1,
		    double *x2, double *y2)
{
    cairo_status_t status;

    if (cr->status) {
	if (x1)
	    *x1 = 0.0;
	if (y1)
	    *y1 = 0.0;
	if (x2)
	    *x2 = 0.0;
	if (y2)
	    *y2 = 0.0;

	return;
    }

    status = _cairo_gstate_clip_extents (cr->gstate, x1, y1, x2, y2);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}

static cairo_rectangle_list_t *
_cairo_rectangle_list_create_in_error (cairo_status_t status)
{
    cairo_rectangle_list_t *list;

    if (status == CAIRO_STATUS_NO_MEMORY)
        return (cairo_rectangle_list_t*) &_cairo_rectangles_nil;

    list = malloc (sizeof (cairo_rectangle_list_t));
    if (unlikely (list == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
        return (cairo_rectangle_list_t*) &_cairo_rectangles_nil;
    }

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
    if (unlikely (status))
	_cairo_set_error (cr, status);
}









void
cairo_font_extents (cairo_t              *cr,
		    cairo_font_extents_t *extents)
{
    cairo_status_t status;

    extents->ascent = 0.0;
    extents->descent = 0.0;
    extents->height = 0.0;
    extents->max_x_advance = 0.0;
    extents->max_y_advance = 0.0;

    if (cr->status)
	return;

    status = _cairo_gstate_get_font_extents (cr->gstate, extents);
    if (unlikely (status))
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
    if (unlikely (status))
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
    if (unlikely (status)) {
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
    if (unlikely (status))
	_cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_font_size);














void
cairo_set_font_matrix (cairo_t		    *cr,
		       const cairo_matrix_t *matrix)
{
    cairo_status_t status;

    if (cr->status)
	return;

    status = _cairo_gstate_set_font_matrix (cr->gstate, matrix);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}









void
cairo_get_font_matrix (cairo_t *cr, cairo_matrix_t *matrix)
{
    if (cr->status) {
	cairo_matrix_init_identity (matrix);
	return;
    }

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
    if (unlikely (status)) {
	_cairo_set_error (cr, status);
	return;
    }

    _cairo_gstate_set_font_options (cr->gstate, options);
}
slim_hidden_def (cairo_set_font_options);












void
cairo_get_font_options (cairo_t              *cr,
			cairo_font_options_t *options)
{
    
    if (cairo_font_options_status (options))
	return;

    if (cr->status) {
	_cairo_font_options_init_default (options);
	return;
    }

    _cairo_gstate_get_font_options (cr->gstate, options);
}














void
cairo_set_scaled_font (cairo_t                   *cr,
		       const cairo_scaled_font_t *scaled_font)
{
    cairo_status_t status;

    if (cr->status)
	return;

    if (scaled_font == NULL) {
	status = _cairo_error (CAIRO_STATUS_NULL_POINTER);
	goto BAIL;
    }

    status = scaled_font->status;
    if (unlikely (status))
        goto BAIL;

    if (scaled_font == cr->gstate->scaled_font)
	return;

    status = _cairo_gstate_set_font_face (cr->gstate, scaled_font->font_face);
    if (unlikely (status))
        goto BAIL;

    status = _cairo_gstate_set_font_matrix (cr->gstate, &scaled_font->font_matrix);
    if (unlikely (status))
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
	return _cairo_scaled_font_create_in_error (cr->status);

    status = _cairo_gstate_get_scaled_font (cr->gstate, &scaled_font);
    if (unlikely (status)) {
	_cairo_set_error (cr, status);
	return _cairo_scaled_font_create_in_error (status);
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

    extents->x_bearing = 0.0;
    extents->y_bearing = 0.0;
    extents->width  = 0.0;
    extents->height = 0.0;
    extents->x_advance = 0.0;
    extents->y_advance = 0.0;

    if (cr->status)
	return;

    if (utf8 == NULL)
	return;

    cairo_get_current_point (cr, &x, &y);

    status = _cairo_gstate_text_to_glyphs (cr->gstate,
					   x, y,
					   utf8, strlen (utf8),
					   &glyphs, &num_glyphs,
					   NULL, NULL,
					   NULL);

    if (status == CAIRO_STATUS_SUCCESS)
	status = _cairo_gstate_glyph_extents (cr->gstate,
		                              glyphs, num_glyphs,
					      extents);
    cairo_glyph_free (glyphs);

    if (unlikely (status))
	_cairo_set_error (cr, status);
}



















void
cairo_glyph_extents (cairo_t                *cr,
		     const cairo_glyph_t    *glyphs,
		     int                    num_glyphs,
		     cairo_text_extents_t   *extents)
{
    cairo_status_t status;

    extents->x_bearing = 0.0;
    extents->y_bearing = 0.0;
    extents->width  = 0.0;
    extents->height = 0.0;
    extents->x_advance = 0.0;
    extents->y_advance = 0.0;

    if (cr->status)
	return;

    if (num_glyphs == 0)
	return;

    if (num_glyphs < 0) {
	_cairo_set_error (cr, CAIRO_STATUS_NEGATIVE_COUNT);
	return;
    }

    if (glyphs == NULL) {
	_cairo_set_error (cr, CAIRO_STATUS_NULL_POINTER);
	return;
    }

    status = _cairo_gstate_glyph_extents (cr->gstate, glyphs, num_glyphs,
					  extents);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}




























void
cairo_show_text (cairo_t *cr, const char *utf8)
{
    cairo_text_extents_t extents;
    cairo_status_t status;
    cairo_glyph_t *glyphs, *last_glyph;
    cairo_text_cluster_t *clusters;
    int utf8_len, num_glyphs, num_clusters;
    cairo_text_cluster_flags_t cluster_flags;
    double x, y;
    cairo_bool_t has_show_text_glyphs;
    cairo_glyph_t stack_glyphs[CAIRO_STACK_ARRAY_LENGTH (cairo_glyph_t)];
    cairo_text_cluster_t stack_clusters[CAIRO_STACK_ARRAY_LENGTH (cairo_text_cluster_t)];

    if (cr->status)
	return;

    if (utf8 == NULL)
	return;

    cairo_get_current_point (cr, &x, &y);

    utf8_len = strlen (utf8);

    has_show_text_glyphs =
	cairo_surface_has_show_text_glyphs (cairo_get_target (cr));

    glyphs = stack_glyphs;
    num_glyphs = ARRAY_LENGTH (stack_glyphs);

    if (has_show_text_glyphs) {
	clusters = stack_clusters;
	num_clusters = ARRAY_LENGTH (stack_clusters);
    } else {
	clusters = NULL;
	num_clusters = 0;
    }

    status = _cairo_gstate_text_to_glyphs (cr->gstate,
					   x, y,
					   utf8, utf8_len,
					   &glyphs, &num_glyphs,
					   has_show_text_glyphs ? &clusters : NULL, &num_clusters,
					   &cluster_flags);
    if (unlikely (status))
	goto BAIL;

    if (num_glyphs == 0)
	return;

    status = _cairo_gstate_show_text_glyphs (cr->gstate,
					     utf8, utf8_len,
					     glyphs, num_glyphs,
					     clusters, num_clusters,
					     cluster_flags);
    if (unlikely (status))
	goto BAIL;

    last_glyph = &glyphs[num_glyphs - 1];
    status = _cairo_gstate_glyph_extents (cr->gstate,
					  last_glyph, 1,
					  &extents);
    if (unlikely (status))
	goto BAIL;

    x = last_glyph->x + extents.x_advance;
    y = last_glyph->y + extents.y_advance;
    cairo_move_to (cr, x, y);

 BAIL:
    if (glyphs != stack_glyphs)
	cairo_glyph_free (glyphs);
    if (clusters != stack_clusters)
	cairo_text_cluster_free (clusters);

    if (unlikely (status))
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

    if (num_glyphs < 0) {
	_cairo_set_error (cr, CAIRO_STATUS_NEGATIVE_COUNT);
	return;
    }

    if (glyphs == NULL) {
	_cairo_set_error (cr, CAIRO_STATUS_NULL_POINTER);
	return;
    }

    status = _cairo_gstate_show_text_glyphs (cr->gstate,
					     NULL, 0,
					     glyphs, num_glyphs,
					     NULL, 0,
					     FALSE);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}



































void
cairo_show_text_glyphs (cairo_t			   *cr,
			const char		   *utf8,
			int			    utf8_len,
			const cairo_glyph_t	   *glyphs,
			int			    num_glyphs,
			const cairo_text_cluster_t *clusters,
			int			    num_clusters,
			cairo_text_cluster_flags_t  cluster_flags)
{
    cairo_status_t status;

    if (cr->status)
	return;

    

    
    if (utf8 == NULL && utf8_len == -1)
	utf8_len = 0;

    
    if ((num_glyphs   && glyphs   == NULL) ||
	(utf8_len     && utf8     == NULL) ||
	(num_clusters && clusters == NULL)) {
	_cairo_set_error (cr, CAIRO_STATUS_NULL_POINTER);
	return;
    }

    
    if (utf8_len == -1)
	utf8_len = strlen (utf8);

    
    if (num_glyphs < 0 || utf8_len < 0 || num_clusters < 0) {
	_cairo_set_error (cr, CAIRO_STATUS_NEGATIVE_COUNT);
	return;
    }

    

    status = _cairo_validate_text_clusters (utf8, utf8_len,
					    glyphs, num_glyphs,
					    clusters, num_clusters, cluster_flags);
    if (status == CAIRO_STATUS_INVALID_CLUSTERS) {
	


	cairo_status_t status2;

	status2 = _cairo_utf8_to_ucs4 (utf8, utf8_len, NULL, NULL);
	if (status2)
	    status = status2;

	_cairo_set_error (cr, status);
	return;
    }

    if (num_glyphs == 0 && utf8_len == 0)
	return;

    status = _cairo_gstate_show_text_glyphs (cr->gstate,
					     utf8, utf8_len,
					     glyphs, num_glyphs,
					     clusters, num_clusters, cluster_flags);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}

























void
cairo_text_path  (cairo_t *cr, const char *utf8)
{
    cairo_status_t status;
    cairo_text_extents_t extents;
    cairo_glyph_t stack_glyphs[CAIRO_STACK_ARRAY_LENGTH (cairo_glyph_t)];
    cairo_glyph_t *glyphs, *last_glyph;
    int num_glyphs;
    double x, y;

    if (cr->status)
	return;

    if (utf8 == NULL)
	return;

    cairo_get_current_point (cr, &x, &y);

    glyphs = stack_glyphs;
    num_glyphs = ARRAY_LENGTH (stack_glyphs);

    status = _cairo_gstate_text_to_glyphs (cr->gstate,
					   x, y,
					   utf8, strlen (utf8),
					   &glyphs, &num_glyphs,
					   NULL, NULL,
					   NULL);

    if (unlikely (status))
	goto BAIL;

    if (num_glyphs == 0)
	return;

    status = _cairo_gstate_glyph_path (cr->gstate,
				       glyphs, num_glyphs,
				       cr->path);

    if (unlikely (status))
	goto BAIL;

    last_glyph = &glyphs[num_glyphs - 1];
    status = _cairo_gstate_glyph_extents (cr->gstate,
					  last_glyph, 1,
					  &extents);

    if (unlikely (status))
	goto BAIL;

    x = last_glyph->x + extents.x_advance;
    y = last_glyph->y + extents.y_advance;
    cairo_move_to (cr, x, y);

 BAIL:
    if (glyphs != stack_glyphs)
	cairo_glyph_free (glyphs);

    if (unlikely (status))
	_cairo_set_error (cr, status);
}











void
cairo_glyph_path (cairo_t *cr, const cairo_glyph_t *glyphs, int num_glyphs)
{
    cairo_status_t status;

    if (cr->status)
	return;

    if (num_glyphs == 0)
	return;

    if (num_glyphs < 0) {
	_cairo_set_error (cr, CAIRO_STATUS_NEGATIVE_COUNT);
	return;
    }

    if (glyphs == NULL) {
	_cairo_set_error (cr, CAIRO_STATUS_NULL_POINTER);
	return;
    }

    status = _cairo_gstate_glyph_path (cr->gstate,
				       glyphs, num_glyphs,
				       cr->path);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}









cairo_operator_t
cairo_get_operator (cairo_t *cr)
{
    if (cr->status)
        return CAIRO_GSTATE_OPERATOR_DEFAULT;

    return _cairo_gstate_get_operator (cr->gstate);
}









double
cairo_get_tolerance (cairo_t *cr)
{
    if (cr->status)
        return CAIRO_GSTATE_TOLERANCE_DEFAULT;

    return _cairo_gstate_get_tolerance (cr->gstate);
}
slim_hidden_def (cairo_get_tolerance);









cairo_antialias_t
cairo_get_antialias (cairo_t *cr)
{
    if (cr->status)
        return CAIRO_ANTIALIAS_DEFAULT;

    return _cairo_gstate_get_antialias (cr->gstate);
}












cairo_bool_t
cairo_has_current_point (cairo_t *cr)
{
    if (cr->status)
    return FALSE;

    return cr->path->has_current_point;
}































void
cairo_get_current_point (cairo_t *cr, double *x_ret, double *y_ret)
{
    cairo_fixed_t x_fixed, y_fixed;
    double x, y;

    if (cr->status == CAIRO_STATUS_SUCCESS &&
	_cairo_path_fixed_get_current_point (cr->path, &x_fixed, &y_fixed))
    {
	x = _cairo_fixed_to_double (x_fixed);
	y = _cairo_fixed_to_double (y_fixed);
	_cairo_gstate_backend_to_user (cr->gstate, &x, &y);
    }
    else
    {
	x = 0.0;
	y = 0.0;
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
    if (cr->status)
        return CAIRO_GSTATE_FILL_RULE_DEFAULT;

    return _cairo_gstate_get_fill_rule (cr->gstate);
}












double
cairo_get_line_width (cairo_t *cr)
{
    if (cr->status)
        return CAIRO_GSTATE_LINE_WIDTH_DEFAULT;

    return _cairo_gstate_get_line_width (cr->gstate);
}
slim_hidden_def (cairo_get_line_width);









cairo_line_cap_t
cairo_get_line_cap (cairo_t *cr)
{
    if (cr->status)
        return CAIRO_GSTATE_LINE_CAP_DEFAULT;

    return _cairo_gstate_get_line_cap (cr->gstate);
}









cairo_line_join_t
cairo_get_line_join (cairo_t *cr)
{
    if (cr->status)
        return CAIRO_GSTATE_LINE_JOIN_DEFAULT;

    return _cairo_gstate_get_line_join (cr->gstate);
}









double
cairo_get_miter_limit (cairo_t *cr)
{
    if (cr->status)
        return CAIRO_GSTATE_MITER_LIMIT_DEFAULT;

    return _cairo_gstate_get_miter_limit (cr->gstate);
}








void
cairo_get_matrix (cairo_t *cr, cairo_matrix_t *matrix)
{
    if (cr->status) {
	cairo_matrix_init_identity (matrix);
	return;
    }

    _cairo_gstate_get_matrix (cr->gstate, matrix);
}
slim_hidden_def (cairo_get_matrix);

















cairo_surface_t *
cairo_get_target (cairo_t *cr)
{
    if (cr->status)
	return _cairo_surface_create_in_error (cr->status);

    return _cairo_gstate_get_original_target (cr->gstate);
}
slim_hidden_def (cairo_get_target);





















cairo_surface_t *
cairo_get_group_target (cairo_t *cr)
{
    if (cr->status)
	return _cairo_surface_create_in_error (cr->status);

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

    if (path->num_data == 0)
	return;

    if (path->data == NULL) {
	_cairo_set_error (cr, CAIRO_STATUS_NULL_POINTER);
	return;
    }

    status = _cairo_path_append_to_context (path, cr);
    if (unlikely (status))
	_cairo_set_error (cr, status);
}









cairo_status_t
cairo_status (cairo_t *cr)
{
    return cr->status;
}
slim_hidden_def (cairo_status);
