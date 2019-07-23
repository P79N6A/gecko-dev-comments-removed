





























#include "cairoint.h"

const cairo_solid_pattern_t _cairo_pattern_nil = {
    { CAIRO_PATTERN_TYPE_SOLID, 	
      CAIRO_REFERENCE_COUNT_INVALID,	
      CAIRO_STATUS_NO_MEMORY,		
      { 0, 0, 0, NULL },		
      { 1., 0., 0., 1., 0., 0., },	
      CAIRO_FILTER_DEFAULT,		
      CAIRO_EXTEND_GRADIENT_DEFAULT },	
};

static const cairo_solid_pattern_t _cairo_pattern_nil_null_pointer = {
    { CAIRO_PATTERN_TYPE_SOLID, 	
      CAIRO_REFERENCE_COUNT_INVALID,	
      CAIRO_STATUS_NULL_POINTER,	
      { 0, 0, 0, NULL },		
      { 1., 0., 0., 1., 0., 0., },	
      CAIRO_FILTER_DEFAULT,		
      CAIRO_EXTEND_GRADIENT_DEFAULT },	
};

const cairo_solid_pattern_t cairo_pattern_none = {
    { CAIRO_PATTERN_TYPE_SOLID, 	
      CAIRO_REFERENCE_COUNT_INVALID,	
      CAIRO_STATUS_SUCCESS,		
      { 0, 0, 0, NULL },		
      { 1., 0., 0., 1., 0., 0., },	
      CAIRO_FILTER_DEFAULT,		
      CAIRO_EXTEND_GRADIENT_DEFAULT },	
};


















static cairo_status_t
_cairo_pattern_set_error (cairo_pattern_t *pattern,
			  cairo_status_t status)
{
    

    _cairo_status_set_error (&pattern->status, status);

    return _cairo_error (status);
}

static void
_cairo_pattern_init (cairo_pattern_t *pattern, cairo_pattern_type_t type)
{
    pattern->type      = type;
    CAIRO_REFERENCE_COUNT_INIT (&pattern->ref_count, 1);
    pattern->status    = CAIRO_STATUS_SUCCESS;

    _cairo_user_data_array_init (&pattern->user_data);

    if (type == CAIRO_PATTERN_TYPE_SURFACE)
	pattern->extend = CAIRO_EXTEND_SURFACE_DEFAULT;
    else
	pattern->extend = CAIRO_EXTEND_GRADIENT_DEFAULT;

    pattern->filter    = CAIRO_FILTER_DEFAULT;

    cairo_matrix_init_identity (&pattern->matrix);
}

static cairo_status_t
_cairo_gradient_pattern_init_copy (cairo_gradient_pattern_t	  *pattern,
				   const cairo_gradient_pattern_t *other)
{
    if (other->base.type == CAIRO_PATTERN_TYPE_LINEAR)
    {
	cairo_linear_pattern_t *dst = (cairo_linear_pattern_t *) pattern;
	cairo_linear_pattern_t *src = (cairo_linear_pattern_t *) other;

	*dst = *src;
    }
    else
    {
	cairo_radial_pattern_t *dst = (cairo_radial_pattern_t *) pattern;
	cairo_radial_pattern_t *src = (cairo_radial_pattern_t *) other;

	*dst = *src;
    }

    if (other->stops == other->stops_embedded)
	pattern->stops = pattern->stops_embedded;
    else if (other->stops)
    {
	pattern->stops = _cairo_malloc_ab (other->stops_size,
					   sizeof (cairo_gradient_stop_t));
	if (pattern->stops == NULL) {
	    pattern->stops_size = 0;
	    pattern->n_stops = 0;
	    return _cairo_pattern_set_error (&pattern->base, CAIRO_STATUS_NO_MEMORY);
	}

	memcpy (pattern->stops, other->stops,
		other->n_stops * sizeof (cairo_gradient_stop_t));
    }

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_pattern_init_copy (cairo_pattern_t	*pattern,
			  const cairo_pattern_t *other)
{
    if (other->status)
	return _cairo_pattern_set_error (pattern, other->status);

    switch (other->type) {
    case CAIRO_PATTERN_TYPE_SOLID: {
	cairo_solid_pattern_t *dst = (cairo_solid_pattern_t *) pattern;
	cairo_solid_pattern_t *src = (cairo_solid_pattern_t *) other;

	*dst = *src;
    } break;
    case CAIRO_PATTERN_TYPE_SURFACE: {
	cairo_surface_pattern_t *dst = (cairo_surface_pattern_t *) pattern;
	cairo_surface_pattern_t *src = (cairo_surface_pattern_t *) other;

	*dst = *src;
	cairo_surface_reference (dst->surface);
    } break;
    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL: {
	cairo_gradient_pattern_t *dst = (cairo_gradient_pattern_t *) pattern;
	cairo_gradient_pattern_t *src = (cairo_gradient_pattern_t *) other;
	cairo_status_t status;

	status = _cairo_gradient_pattern_init_copy (dst, src);
	if (status)
	    return status;

    } break;
    }

    
    CAIRO_REFERENCE_COUNT_INIT (&pattern->ref_count, 1);
    _cairo_user_data_array_init (&pattern->user_data);

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_pattern_fini (cairo_pattern_t *pattern)
{
    _cairo_user_data_array_fini (&pattern->user_data);

    switch (pattern->type) {
    case CAIRO_PATTERN_TYPE_SOLID:
	break;
    case CAIRO_PATTERN_TYPE_SURFACE: {
	cairo_surface_pattern_t *surface_pattern =
	    (cairo_surface_pattern_t *) pattern;

	cairo_surface_destroy (surface_pattern->surface);
    } break;
    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL: {
	cairo_gradient_pattern_t *gradient =
	    (cairo_gradient_pattern_t *) pattern;

	if (gradient->stops && gradient->stops != gradient->stops_embedded)
	    free (gradient->stops);
    } break;
    }
}

cairo_status_t
_cairo_pattern_create_copy (cairo_pattern_t	  **pattern,
			    const cairo_pattern_t  *other)
{
    cairo_status_t status;

    if (other->status)
	return other->status;

    switch (other->type) {
    case CAIRO_PATTERN_TYPE_SOLID:
	*pattern = malloc (sizeof (cairo_solid_pattern_t));
	break;
    case CAIRO_PATTERN_TYPE_SURFACE:
	*pattern = malloc (sizeof (cairo_surface_pattern_t));
	break;
    case CAIRO_PATTERN_TYPE_LINEAR:
	*pattern = malloc (sizeof (cairo_linear_pattern_t));
	break;
    case CAIRO_PATTERN_TYPE_RADIAL:
	*pattern = malloc (sizeof (cairo_radial_pattern_t));
	break;
    }
    if (*pattern == NULL)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    status = _cairo_pattern_init_copy (*pattern, other);
    if (status) {
	free (*pattern);
	return status;
    }

    return CAIRO_STATUS_SUCCESS;
}


void
_cairo_pattern_init_solid (cairo_solid_pattern_t *pattern,
			   const cairo_color_t	 *color,
			   cairo_content_t	  content)
{
    _cairo_pattern_init (&pattern->base, CAIRO_PATTERN_TYPE_SOLID);
    pattern->color = *color;
    pattern->content = content;
}

void
_cairo_pattern_init_for_surface (cairo_surface_pattern_t *pattern,
				 cairo_surface_t	 *surface)
{
    if (surface->status) {
	
	_cairo_pattern_init (&pattern->base, CAIRO_PATTERN_TYPE_SOLID);
	_cairo_pattern_set_error (&pattern->base, surface->status);
	return;
    }

    _cairo_pattern_init (&pattern->base, CAIRO_PATTERN_TYPE_SURFACE);

    pattern->surface = cairo_surface_reference (surface);
}

static void
_cairo_pattern_init_gradient (cairo_gradient_pattern_t *pattern,
			      cairo_pattern_type_t     type)
{
    _cairo_pattern_init (&pattern->base, type);

    pattern->n_stops    = 0;
    pattern->stops_size = 0;
    pattern->stops      = NULL;
}

void
_cairo_pattern_init_linear (cairo_linear_pattern_t *pattern,
			    double x0, double y0, double x1, double y1)
{
    _cairo_pattern_init_gradient (&pattern->base, CAIRO_PATTERN_TYPE_LINEAR);

    pattern->p1.x = _cairo_fixed_from_double (x0);
    pattern->p1.y = _cairo_fixed_from_double (y0);
    pattern->p2.x = _cairo_fixed_from_double (x1);
    pattern->p2.y = _cairo_fixed_from_double (y1);
}

void
_cairo_pattern_init_radial (cairo_radial_pattern_t *pattern,
			    double cx0, double cy0, double radius0,
			    double cx1, double cy1, double radius1)
{
    _cairo_pattern_init_gradient (&pattern->base, CAIRO_PATTERN_TYPE_RADIAL);

    pattern->c1.x = _cairo_fixed_from_double (cx0);
    pattern->c1.y = _cairo_fixed_from_double (cy0);
    pattern->r1   = _cairo_fixed_from_double (fabs (radius0));
    pattern->c2.x = _cairo_fixed_from_double (cx1);
    pattern->c2.y = _cairo_fixed_from_double (cy1);
    pattern->r2   = _cairo_fixed_from_double (fabs (radius1));
}



#define MAX_PATTERN_CACHE_SIZE 4
static struct {
    cairo_solid_pattern_t *patterns[MAX_PATTERN_CACHE_SIZE];
    int size;
} solid_pattern_cache;

cairo_pattern_t *
_cairo_pattern_create_solid (const cairo_color_t *color,
			     cairo_content_t	  content)
{
    cairo_solid_pattern_t *pattern = NULL;

    CAIRO_MUTEX_LOCK (_cairo_pattern_solid_pattern_cache_lock);

    if (solid_pattern_cache.size) {
	int i = --solid_pattern_cache.size %
	    ARRAY_LENGTH (solid_pattern_cache.patterns);
	pattern = solid_pattern_cache.patterns[i];
	solid_pattern_cache.patterns[i] = NULL;
    }

    CAIRO_MUTEX_UNLOCK (_cairo_pattern_solid_pattern_cache_lock);

    if (pattern == NULL) {
	
	pattern = malloc (sizeof (cairo_solid_pattern_t));
    }

    if (pattern == NULL) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	pattern = (cairo_solid_pattern_t *) &_cairo_pattern_nil;
    } else
	_cairo_pattern_init_solid (pattern, color, content);

    return &pattern->base;
}

static void
_cairo_pattern_reset_solid_pattern_cache (void)
{
    int i;

    CAIRO_MUTEX_LOCK (_cairo_pattern_solid_pattern_cache_lock);

    for (i = 0; i < MIN (ARRAY_LENGTH (solid_pattern_cache.patterns), solid_pattern_cache.size); i++) {
	free (solid_pattern_cache.patterns[i]);
	solid_pattern_cache.patterns[i] = NULL;
    }
    solid_pattern_cache.size = 0;

    CAIRO_MUTEX_UNLOCK (_cairo_pattern_solid_pattern_cache_lock);
}

static const cairo_pattern_t *
_cairo_pattern_create_in_error (cairo_status_t status)
{
    cairo_pattern_t *pattern;

    if (status == CAIRO_STATUS_NO_MEMORY)
	return (cairo_pattern_t *)&_cairo_pattern_nil.base;

    CAIRO_MUTEX_INITIALIZE ();

    pattern = _cairo_pattern_create_solid (_cairo_stock_color (CAIRO_STOCK_BLACK),
					   CAIRO_CONTENT_COLOR);
    if (pattern->status == CAIRO_STATUS_SUCCESS)
	status = _cairo_pattern_set_error (pattern, status);

    return pattern;
}





















cairo_pattern_t *
cairo_pattern_create_rgb (double red, double green, double blue)
{
    cairo_color_t color;

    _cairo_restrict_value (&red,   0.0, 1.0);
    _cairo_restrict_value (&green, 0.0, 1.0);
    _cairo_restrict_value (&blue,  0.0, 1.0);

    _cairo_color_init_rgb (&color, red, green, blue);

    CAIRO_MUTEX_INITIALIZE ();

    return _cairo_pattern_create_solid (&color, CAIRO_CONTENT_COLOR);
}
slim_hidden_def (cairo_pattern_create_rgb);






















cairo_pattern_t *
cairo_pattern_create_rgba (double red, double green, double blue,
			   double alpha)
{
    cairo_color_t color;

    _cairo_restrict_value (&red,   0.0, 1.0);
    _cairo_restrict_value (&green, 0.0, 1.0);
    _cairo_restrict_value (&blue,  0.0, 1.0);
    _cairo_restrict_value (&alpha, 0.0, 1.0);

    _cairo_color_init_rgba (&color, red, green, blue, alpha);

    CAIRO_MUTEX_INITIALIZE ();

    return _cairo_pattern_create_solid (&color, CAIRO_CONTENT_COLOR_ALPHA);
}
slim_hidden_def (cairo_pattern_create_rgba);
















cairo_pattern_t *
cairo_pattern_create_for_surface (cairo_surface_t *surface)
{
    cairo_surface_pattern_t *pattern;

    if (surface == NULL) {
	_cairo_error_throw (CAIRO_STATUS_NULL_POINTER);
	return (cairo_pattern_t*) &_cairo_pattern_nil_null_pointer;
    }

    if (surface->status)
	return (cairo_pattern_t*) _cairo_pattern_create_in_error (surface->status);

    pattern = malloc (sizeof (cairo_surface_pattern_t));
    if (pattern == NULL) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_pattern_t *)&_cairo_pattern_nil.base;
    }

    CAIRO_MUTEX_INITIALIZE ();

    _cairo_pattern_init_for_surface (pattern, surface);

    return &pattern->base;
}
slim_hidden_def (cairo_pattern_create_for_surface);



























cairo_pattern_t *
cairo_pattern_create_linear (double x0, double y0, double x1, double y1)
{
    cairo_linear_pattern_t *pattern;

    pattern = malloc (sizeof (cairo_linear_pattern_t));
    if (pattern == NULL) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_pattern_t *) &_cairo_pattern_nil.base;
    }

    CAIRO_MUTEX_INITIALIZE ();

    _cairo_pattern_init_linear (pattern, x0, y0, x1, y1);

    return &pattern->base.base;
}





























cairo_pattern_t *
cairo_pattern_create_radial (double cx0, double cy0, double radius0,
			     double cx1, double cy1, double radius1)
{
    cairo_radial_pattern_t *pattern;

    pattern = malloc (sizeof (cairo_radial_pattern_t));
    if (pattern == NULL) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_pattern_t *) &_cairo_pattern_nil.base;
    }

    CAIRO_MUTEX_INITIALIZE ();

    _cairo_pattern_init_radial (pattern, cx0, cy0, radius0, cx1, cy1, radius1);

    return &pattern->base.base;
}














cairo_pattern_t *
cairo_pattern_reference (cairo_pattern_t *pattern)
{
    if (pattern == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&pattern->ref_count))
	return pattern;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&pattern->ref_count));

    _cairo_reference_count_inc (&pattern->ref_count);

    return pattern;
}
slim_hidden_def (cairo_pattern_reference);












cairo_pattern_type_t
cairo_pattern_get_type (cairo_pattern_t *pattern)
{
    return pattern->type;
}
slim_hidden_def (cairo_pattern_get_type);











cairo_status_t
cairo_pattern_status (cairo_pattern_t *pattern)
{
    return pattern->status;
}
slim_hidden_def (cairo_pattern_status);









void
cairo_pattern_destroy (cairo_pattern_t *pattern)
{
    if (pattern == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&pattern->ref_count))
	return;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&pattern->ref_count));

    if (! _cairo_reference_count_dec_and_test (&pattern->ref_count))
	return;

    _cairo_pattern_fini (pattern);

    
    if (pattern->type == CAIRO_PATTERN_TYPE_SOLID) {
	int i;

	CAIRO_MUTEX_LOCK (_cairo_pattern_solid_pattern_cache_lock);

	i = solid_pattern_cache.size++ %
	    ARRAY_LENGTH (solid_pattern_cache.patterns);
	
	if (solid_pattern_cache.patterns[i])
	    free (solid_pattern_cache.patterns[i]);

	solid_pattern_cache.patterns[i] = (cairo_solid_pattern_t *) pattern;

	CAIRO_MUTEX_UNLOCK (_cairo_pattern_solid_pattern_cache_lock);
    } else {
	free (pattern);
    }
}
slim_hidden_def (cairo_pattern_destroy);












unsigned int
cairo_pattern_get_reference_count (cairo_pattern_t *pattern)
{
    if (pattern == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&pattern->ref_count))
	return 0;

    return CAIRO_REFERENCE_COUNT_GET_VALUE (&pattern->ref_count);
}















void *
cairo_pattern_get_user_data (cairo_pattern_t		 *pattern,
			     const cairo_user_data_key_t *key)
{
    return _cairo_user_data_array_get_data (&pattern->user_data,
					    key);
}



















cairo_status_t
cairo_pattern_set_user_data (cairo_pattern_t		 *pattern,
			     const cairo_user_data_key_t *key,
			     void			 *user_data,
			     cairo_destroy_func_t	  destroy)
{
    if (CAIRO_REFERENCE_COUNT_IS_INVALID (&pattern->ref_count))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    return _cairo_user_data_array_set_data (&pattern->user_data,
					    key, user_data, destroy);
}


static cairo_status_t
_cairo_pattern_gradient_grow (cairo_gradient_pattern_t *pattern)
{
    cairo_gradient_stop_t *new_stops;
    int old_size = pattern->stops_size;
    int embedded_size = ARRAY_LENGTH (pattern->stops_embedded);
    int new_size = 2 * MAX (old_size, 4);

    

    if (old_size < embedded_size) {
	pattern->stops = pattern->stops_embedded;
	pattern->stops_size = embedded_size;
	return CAIRO_STATUS_SUCCESS;
    }

    assert (pattern->n_stops <= pattern->stops_size);

    if (pattern->stops == pattern->stops_embedded) {
	new_stops = _cairo_malloc_ab (new_size, sizeof (cairo_gradient_stop_t));
	if (new_stops)
	    memcpy (new_stops, pattern->stops, old_size * sizeof (cairo_gradient_stop_t));
    } else {
	new_stops = _cairo_realloc_ab (pattern->stops,
	       	                       new_size,
				       sizeof (cairo_gradient_stop_t));
    }

    if (new_stops == NULL)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    pattern->stops = new_stops;
    pattern->stops_size = new_size;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_pattern_add_color_stop (cairo_gradient_pattern_t *pattern,
			       double			 offset,
			       double			 red,
			       double			 green,
			       double			 blue,
			       double			 alpha)
{
    cairo_gradient_stop_t *stops;
    unsigned int	   i;

    if (pattern->n_stops >= pattern->stops_size) {
        cairo_status_t status = _cairo_pattern_gradient_grow (pattern);
	if (status) {
	    status = _cairo_pattern_set_error (&pattern->base, status);
	    return;
	}
    }

    stops = pattern->stops;

    for (i = 0; i < pattern->n_stops; i++)
    {
	if (offset < stops[i].offset)
	{
	    memmove (&stops[i + 1], &stops[i],
		     sizeof (cairo_gradient_stop_t) * (pattern->n_stops - i));

	    break;
	}
    }

    stops[i].offset = offset;

    stops[i].color.red   = red;
    stops[i].color.green = green;
    stops[i].color.blue  = blue;
    stops[i].color.alpha = alpha;

    stops[i].color.red_short   = _cairo_color_double_to_short (red);
    stops[i].color.green_short = _cairo_color_double_to_short (green);
    stops[i].color.blue_short  = _cairo_color_double_to_short (blue);
    stops[i].color.alpha_short = _cairo_color_double_to_short (alpha);

    pattern->n_stops++;
}




























void
cairo_pattern_add_color_stop_rgb (cairo_pattern_t *pattern,
				  double	   offset,
				  double	   red,
				  double	   green,
				  double	   blue)
{
    if (pattern->status)
	return;

    if (pattern->type != CAIRO_PATTERN_TYPE_LINEAR &&
	pattern->type != CAIRO_PATTERN_TYPE_RADIAL)
    {
	_cairo_pattern_set_error (pattern, CAIRO_STATUS_PATTERN_TYPE_MISMATCH);
	return;
    }

    _cairo_restrict_value (&offset, 0.0, 1.0);
    _cairo_restrict_value (&red,    0.0, 1.0);
    _cairo_restrict_value (&green,  0.0, 1.0);
    _cairo_restrict_value (&blue,   0.0, 1.0);

    _cairo_pattern_add_color_stop ((cairo_gradient_pattern_t *) pattern,
				   offset, red, green, blue, 1.0);
}




























void
cairo_pattern_add_color_stop_rgba (cairo_pattern_t *pattern,
				   double	   offset,
				   double	   red,
				   double	   green,
				   double	   blue,
				   double	   alpha)
{
    if (pattern->status)
	return;

    if (pattern->type != CAIRO_PATTERN_TYPE_LINEAR &&
	pattern->type != CAIRO_PATTERN_TYPE_RADIAL)
    {
	_cairo_pattern_set_error (pattern, CAIRO_STATUS_PATTERN_TYPE_MISMATCH);
	return;
    }

    _cairo_restrict_value (&offset, 0.0, 1.0);
    _cairo_restrict_value (&red,    0.0, 1.0);
    _cairo_restrict_value (&green,  0.0, 1.0);
    _cairo_restrict_value (&blue,   0.0, 1.0);
    _cairo_restrict_value (&alpha,  0.0, 1.0);

    _cairo_pattern_add_color_stop ((cairo_gradient_pattern_t *) pattern,
				   offset, red, green, blue, alpha);
}

































void
cairo_pattern_set_matrix (cairo_pattern_t      *pattern,
			  const cairo_matrix_t *matrix)
{
    cairo_matrix_t inverse;
    cairo_status_t status;

    if (pattern->status)
	return;

    pattern->matrix = *matrix;

    inverse = *matrix;
    status = cairo_matrix_invert (&inverse);
    if (status)
	status = _cairo_pattern_set_error (pattern, status);
}
slim_hidden_def (cairo_pattern_set_matrix);








void
cairo_pattern_get_matrix (cairo_pattern_t *pattern, cairo_matrix_t *matrix)
{
    *matrix = pattern->matrix;
}





















void
cairo_pattern_set_filter (cairo_pattern_t *pattern, cairo_filter_t filter)
{
    if (pattern->status)
	return;

    pattern->filter = filter;
}










cairo_filter_t
cairo_pattern_get_filter (cairo_pattern_t *pattern)
{
    return pattern->filter;
}














void
cairo_pattern_set_extend (cairo_pattern_t *pattern, cairo_extend_t extend)
{
    if (pattern->status)
	return;

    pattern->extend = extend;
}











cairo_extend_t
cairo_pattern_get_extend (cairo_pattern_t *pattern)
{
    return pattern->extend;
}
slim_hidden_def (cairo_pattern_get_extend);

void
_cairo_pattern_transform (cairo_pattern_t	*pattern,
			  const cairo_matrix_t  *ctm_inverse)
{
    if (pattern->status)
	return;

    cairo_matrix_multiply (&pattern->matrix, ctm_inverse, &pattern->matrix);
}

static void
_cairo_linear_pattern_classify (cairo_linear_pattern_t *pattern,
				double		       offset_x,
				double		       offset_y,
				int		       width,
				int		       height,
				cairo_bool_t           *is_horizontal,
				cairo_bool_t           *is_vertical)
{
    cairo_point_double_t point0, point1;
    double a, b, c, d, tx, ty;
    double scale, start, dx, dy;
    cairo_fixed_t factors[3];
    int i;

    




    point0.x = _cairo_fixed_to_double (pattern->p1.x);
    point0.y = _cairo_fixed_to_double (pattern->p1.y);
    point1.x = _cairo_fixed_to_double (pattern->p2.x);
    point1.y = _cairo_fixed_to_double (pattern->p2.y);

    _cairo_matrix_get_affine (&pattern->base.base.matrix,
			      &a, &b, &c, &d, &tx, &ty);

    dx = point1.x - point0.x;
    dy = point1.y - point0.y;
    scale = dx * dx + dy * dy;
    scale = (scale) ? 1.0 / scale : 1.0;

    start = dx * point0.x + dy * point0.y;

    for (i = 0; i < 3; i++) {
	double qx_device = (i % 2) * (width - 1) + offset_x;
	double qy_device = (i / 2) * (height - 1) + offset_y;

	
	double qx = a * qx_device + c * qy_device + tx;
	double qy = b * qx_device + d * qy_device + ty;

	factors[i] = _cairo_fixed_from_double (((dx * qx + dy * qy) - start) * scale);
    }

    







    *is_vertical = factors[1] == factors[0];
    *is_horizontal = factors[2] == factors[0];
}

static cairo_int_status_t
_cairo_pattern_acquire_surface_for_gradient (cairo_gradient_pattern_t *pattern,
					     cairo_surface_t	        *dst,
					     int			x,
					     int			y,
					     unsigned int		width,
					     unsigned int	        height,
					     cairo_surface_t	        **out,
					     cairo_surface_attributes_t *attr)
{
    cairo_image_surface_t *image;
    pixman_image_t	  *pixman_image;
    pixman_transform_t	  pixman_transform;
    cairo_status_t	  status;
    cairo_bool_t	  repeat = FALSE;

    pixman_gradient_stop_t pixman_stops_static[2];
    pixman_gradient_stop_t *pixman_stops = pixman_stops_static;
    unsigned int i;

    if (pattern->n_stops > ARRAY_LENGTH(pixman_stops_static)) {
	pixman_stops = _cairo_malloc_ab (pattern->n_stops, sizeof(pixman_gradient_stop_t));
	if (pixman_stops == NULL)
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    for (i = 0; i < pattern->n_stops; i++) {
	pixman_stops[i].x = _cairo_fixed_16_16_from_double (pattern->stops[i].offset);
	pixman_stops[i].color.red = pattern->stops[i].color.red_short;
	pixman_stops[i].color.green = pattern->stops[i].color.green_short;
	pixman_stops[i].color.blue = pattern->stops[i].color.blue_short;
	pixman_stops[i].color.alpha = pattern->stops[i].color.alpha_short;
    }

    if (pattern->base.type == CAIRO_PATTERN_TYPE_LINEAR)
    {
	cairo_linear_pattern_t *linear = (cairo_linear_pattern_t *) pattern;
	pixman_point_fixed_t p1, p2;

	p1.x = _cairo_fixed_to_16_16 (linear->p1.x);
	p1.y = _cairo_fixed_to_16_16 (linear->p1.y);
	p2.x = _cairo_fixed_to_16_16 (linear->p2.x);
	p2.y = _cairo_fixed_to_16_16 (linear->p2.y);

	pixman_image = pixman_image_create_linear_gradient (&p1, &p2,
							    pixman_stops,
							    pattern->n_stops);
    }
    else
    {
	cairo_radial_pattern_t *radial = (cairo_radial_pattern_t *) pattern;
	pixman_point_fixed_t c1, c2;
	pixman_fixed_t r1, r2;

	c1.x = _cairo_fixed_to_16_16 (radial->c1.x);
	c1.y = _cairo_fixed_to_16_16 (radial->c1.y);
	r1   = _cairo_fixed_to_16_16 (radial->r1);

	c2.x = _cairo_fixed_to_16_16 (radial->c2.x);
	c2.y = _cairo_fixed_to_16_16 (radial->c2.y);
	r2   = _cairo_fixed_to_16_16 (radial->r2);

	pixman_image = pixman_image_create_radial_gradient (&c1, &c2,
							    r1, r2,
							    pixman_stops,
							    pattern->n_stops);
    }

    if (pixman_stops != pixman_stops_static)
	free (pixman_stops);

    if (pixman_image == NULL)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    if (_cairo_surface_is_image (dst))
    {
	image = (cairo_image_surface_t *)
	    _cairo_image_surface_create_for_pixman_image (pixman_image,
							  PIXMAN_a8r8g8b8);
	if (image->base.status)
	{
	    pixman_image_unref (pixman_image);
	    return image->base.status;
	}

	attr->x_offset = attr->y_offset = 0;
	attr->matrix = pattern->base.matrix;
	attr->extend = pattern->base.extend;
	attr->filter = CAIRO_FILTER_NEAREST;
	attr->acquired = FALSE;

	*out = &image->base;

	return CAIRO_STATUS_SUCCESS;
    }

    if (pattern->base.type == CAIRO_PATTERN_TYPE_LINEAR) {
	cairo_bool_t is_horizontal;
	cairo_bool_t is_vertical;

	_cairo_linear_pattern_classify ((cairo_linear_pattern_t *)pattern,
					x, y, width, height,
					&is_horizontal, &is_vertical);
	if (is_horizontal) {
	    height = 1;
	    repeat = TRUE;
	}
	






	if (is_vertical && width > 8) {
	    width = 8;
	    repeat = TRUE;
	}
    }

    image = (cairo_image_surface_t *)
	cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    if (image->base.status) {
	pixman_image_unref (pixman_image);
	return image->base.status;
    }

    pixman_image_set_filter (pixman_image, PIXMAN_FILTER_BILINEAR, NULL, 0);

    _cairo_matrix_to_pixman_matrix (&pattern->base.matrix, &pixman_transform);
    if (!pixman_image_set_transform (pixman_image, &pixman_transform)) {
	cairo_surface_destroy (&image->base);
	pixman_image_unref (pixman_image);
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    switch (pattern->base.extend) {
    case CAIRO_EXTEND_NONE:
	pixman_image_set_repeat (pixman_image, PIXMAN_REPEAT_NONE);
	break;
    case CAIRO_EXTEND_REPEAT:
	pixman_image_set_repeat (pixman_image, PIXMAN_REPEAT_NORMAL);
	break;
    case CAIRO_EXTEND_REFLECT:
	pixman_image_set_repeat (pixman_image, PIXMAN_REPEAT_REFLECT);
	break;
    case CAIRO_EXTEND_PAD:
	pixman_image_set_repeat (pixman_image, PIXMAN_REPEAT_PAD);
	break;
    }

    pixman_image_composite (PIXMAN_OP_SRC,
			    pixman_image,
			    NULL,
			    image->pixman_image,
			    x, y,
			    0, 0,
			    0, 0,
			    width, height);

    pixman_image_unref (pixman_image);

    status = _cairo_surface_clone_similar (dst, &image->base,
					   0, 0, width, height, out);

    cairo_surface_destroy (&image->base);

    attr->x_offset = -x;
    attr->y_offset = -y;
    cairo_matrix_init_identity (&attr->matrix);
    attr->extend = repeat ? CAIRO_EXTEND_REPEAT : CAIRO_EXTEND_NONE;
    attr->filter = CAIRO_FILTER_NEAREST;
    attr->acquired = FALSE;

    return status;
}



#define MAX_SURFACE_CACHE_SIZE 16
static struct {
    struct _cairo_pattern_solid_surface_cache{
	cairo_color_t    color;
	cairo_surface_t *surface;
    } cache[MAX_SURFACE_CACHE_SIZE];
    int size;
} solid_surface_cache;

static cairo_bool_t
_cairo_pattern_solid_surface_matches (
	const struct _cairo_pattern_solid_surface_cache	    *cache,
	const cairo_solid_pattern_t			    *pattern,
	cairo_surface_t					    *dst)
{
    if (CAIRO_REFERENCE_COUNT_GET_VALUE (&cache->surface->ref_count) != 1)
	return FALSE;

    if (! _cairo_color_equal (&cache->color, &pattern->color))
	return FALSE;

    if (! _cairo_surface_is_similar (cache->surface, dst, pattern->content))
	return FALSE;

    return TRUE;
}

static cairo_int_status_t
_cairo_pattern_acquire_surface_for_solid (cairo_solid_pattern_t	     *pattern,
					  cairo_surface_t	     *dst,
					  int			     x,
					  int			     y,
					  unsigned int		     width,
					  unsigned int		     height,
					  cairo_surface_t	     **out,
					  cairo_surface_attributes_t *attribs)
{
    static int i;

    cairo_surface_t *surface;
    cairo_status_t   status;

    CAIRO_MUTEX_LOCK (_cairo_pattern_solid_surface_cache_lock);

    
    if (i < solid_surface_cache.size &&
	    _cairo_pattern_solid_surface_matches (&solid_surface_cache.cache[i],
		                                  pattern,
						  dst))
    {
	if (! _cairo_surface_reset (solid_surface_cache.cache[i].surface))
	    goto DONE;
    }

    for (i = 0 ; i < solid_surface_cache.size; i++) {
	if (_cairo_pattern_solid_surface_matches (&solid_surface_cache.cache[i],
		                                  pattern,
						  dst))
	{
	    if (! _cairo_surface_reset (solid_surface_cache.cache[i].surface))
		goto DONE;
	}
    }

    
    surface = _cairo_surface_create_similar_solid (dst,
	                                           pattern->content,
						   1, 1,
						   &pattern->color,
						   &pattern->base);
    if (surface->status) {
	status = surface->status;
	goto UNLOCK;
    }

    if (! _cairo_surface_is_similar (surface, dst, pattern->content)) {
	

	*out = surface;
	goto NOCACHE;
    }

    
    if (solid_surface_cache.size < MAX_SURFACE_CACHE_SIZE) {
	solid_surface_cache.size++;
    } else {
	i = rand () % MAX_SURFACE_CACHE_SIZE;

	
	cairo_surface_destroy (solid_surface_cache.cache[i].surface);
    }

    solid_surface_cache.cache[i].color   = pattern->color;
    solid_surface_cache.cache[i].surface = surface;

DONE:
    *out = cairo_surface_reference (solid_surface_cache.cache[i].surface);

NOCACHE:
    attribs->x_offset = attribs->y_offset = 0;
    cairo_matrix_init_identity (&attribs->matrix);
    attribs->extend = CAIRO_EXTEND_REPEAT;
    attribs->filter = CAIRO_FILTER_NEAREST;
    attribs->acquired = FALSE;

    status = CAIRO_STATUS_SUCCESS;

UNLOCK:
    CAIRO_MUTEX_UNLOCK (_cairo_pattern_solid_surface_cache_lock);

    return status;
}

static void
_cairo_pattern_reset_solid_surface_cache (void)
{
    int i;

    CAIRO_MUTEX_LOCK (_cairo_pattern_solid_surface_cache_lock);

    for (i = 0; i < solid_surface_cache.size; i++)
	cairo_surface_destroy (solid_surface_cache.cache[i].surface);
    solid_surface_cache.size = 0;

    CAIRO_MUTEX_UNLOCK (_cairo_pattern_solid_surface_cache_lock);
}












cairo_bool_t
_cairo_pattern_is_opaque_solid (const cairo_pattern_t *pattern)
{
    cairo_solid_pattern_t *solid;

    if (pattern->type != CAIRO_PATTERN_TYPE_SOLID)
	return FALSE;

    solid = (cairo_solid_pattern_t *) pattern;

    return CAIRO_COLOR_IS_OPAQUE (&solid->color);
}

static cairo_bool_t
_gradient_is_opaque (const cairo_gradient_pattern_t *gradient)
{
    unsigned int i;

    for (i = 0; i < gradient->n_stops; i++)
	if (! CAIRO_COLOR_IS_OPAQUE (&gradient->stops[i].color))
	    return FALSE;

    return TRUE;
}










cairo_bool_t
_cairo_pattern_is_opaque (const cairo_pattern_t *abstract_pattern)
{
    const cairo_pattern_union_t *pattern;

    pattern = (cairo_pattern_union_t *) abstract_pattern;
    switch (pattern->base.type) {
    case CAIRO_PATTERN_TYPE_SOLID:
	return _cairo_pattern_is_opaque_solid (abstract_pattern);
    case CAIRO_PATTERN_TYPE_SURFACE:
	return cairo_surface_get_content (pattern->surface.surface) == CAIRO_CONTENT_COLOR;
    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL:
	return _gradient_is_opaque (&pattern->gradient.base);
    }

    ASSERT_NOT_REACHED;
    return FALSE;
}

static cairo_int_status_t
_cairo_pattern_acquire_surface_for_surface (cairo_surface_pattern_t   *pattern,
					    cairo_surface_t	       *dst,
					    int			       x,
					    int			       y,
					    unsigned int	       width,
					    unsigned int	       height,
					    cairo_surface_t	       **out,
					    cairo_surface_attributes_t *attr)
{
    cairo_int_status_t status;
    int tx, ty;

    attr->acquired = FALSE;

    attr->extend = pattern->base.extend;
    attr->filter = pattern->base.filter;
    if (_cairo_matrix_is_integer_translation (&pattern->base.matrix,
					      &tx, &ty))
    {
	cairo_matrix_init_identity (&attr->matrix);
	attr->x_offset = tx;
	attr->y_offset = ty;
	attr->filter = CAIRO_FILTER_NEAREST;
    }
    else
    {
	attr->matrix = pattern->base.matrix;
	attr->x_offset = attr->y_offset = 0;
	tx = 0;
	ty = 0;
    }

    










    if (attr->extend == CAIRO_EXTEND_REFLECT) {
	cairo_t *cr;
	int w,h;

	cairo_rectangle_int_t extents;
	status = _cairo_surface_get_extents (pattern->surface, &extents);
	if (status)
	    return status;

	attr->extend = CAIRO_EXTEND_REPEAT;

	






	x = extents.x;
	y = extents.y;
	w = 2 * extents.width;
	h = 2 * extents.height;

	*out = cairo_surface_create_similar (dst, dst->content, w, h);
	status = cairo_surface_status (*out);
	if (status) {
	    cairo_surface_destroy (*out);
	    *out = NULL;
	    return status;
	}

	(*out)->device_transform = pattern->surface->device_transform;
	(*out)->device_transform_inverse = pattern->surface->device_transform_inverse;

	cr = cairo_create (*out);

	cairo_set_source_surface (cr, pattern->surface, -x, -y);
	cairo_paint (cr);

	cairo_scale (cr, -1, +1);
	cairo_set_source_surface (cr, pattern->surface, x-w, -y);
	cairo_paint (cr);

	cairo_scale (cr, +1, -1);
	cairo_set_source_surface (cr, pattern->surface, x-w, y-h);
	cairo_paint (cr);

	cairo_scale (cr, -1, +1);
	cairo_set_source_surface (cr, pattern->surface, -x, y-h);
	cairo_paint (cr);

	status = cairo_status (cr);
	cairo_destroy (cr);

	if (status) {
	    cairo_surface_destroy (*out);
	    *out = NULL;
	}

	return status;
    }

    if (_cairo_surface_is_image (dst))
    {
	cairo_image_surface_t *image;

	status = _cairo_surface_acquire_source_image (pattern->surface,
						      &image,
						      &attr->extra);
	if (status)
	    return status;

	*out = &image->base;
	attr->acquired = TRUE;
    }
    else
    {
	cairo_rectangle_int_t extents;
	status = _cairo_surface_get_extents (pattern->surface, &extents);
	if (status)
	    return status;

	
	

	if (attr->extend == CAIRO_EXTEND_REPEAT ||
	    (width == (unsigned int) -1 && height == (unsigned int) -1)) {
	    x = extents.x;
	    y = extents.y;
	    width = extents.width;
	    height = extents.height;
	} else {
	    



	    if (! _cairo_matrix_is_identity (&attr->matrix)) {
		double x1 = x;
		double y1 = y;
		double x2 = x + width;
		double y2 = y + height;
		cairo_bool_t is_tight;

		_cairo_matrix_transform_bounding_box  (&attr->matrix,
						       &x1, &y1, &x2, &y2,
						       &is_tight);

		









		x = MAX (0, floor (x1) - 1);
		y = MAX (0, floor (y1) - 1);
		width = MIN (extents.width, ceil (x2) + 1) - x;
		height = MIN (extents.height, ceil (y2) + 1) - y;
	    }
	    x += tx;
	    y += ty;
	}

	status = _cairo_surface_clone_similar (dst, pattern->surface,
					       x, y, width, height, out);
    }

    return status;
}


















cairo_int_status_t
_cairo_pattern_acquire_surface (cairo_pattern_t		   *pattern,
				cairo_surface_t		   *dst,
				int			   x,
				int			   y,
				unsigned int		   width,
				unsigned int		   height,
				cairo_surface_t		   **surface_out,
				cairo_surface_attributes_t *attributes)
{
    cairo_status_t status;

    if (pattern->status) {
	*surface_out = NULL;
	attributes->acquired = FALSE;
	return pattern->status;
    }

    switch (pattern->type) {
    case CAIRO_PATTERN_TYPE_SOLID: {
	cairo_solid_pattern_t *src = (cairo_solid_pattern_t *) pattern;

	status = _cairo_pattern_acquire_surface_for_solid (src, dst,
							   x, y, width, height,
							   surface_out,
							   attributes);
	} break;
    case CAIRO_PATTERN_TYPE_LINEAR:
    case CAIRO_PATTERN_TYPE_RADIAL: {
	cairo_gradient_pattern_t *src = (cairo_gradient_pattern_t *) pattern;

	
	if (src->n_stops < 2)
	{
	    cairo_solid_pattern_t solid;

	    if (src->n_stops)
	    {
		cairo_color_t color;

		_cairo_color_init_rgba (&color,
					src->stops->color.red,
					src->stops->color.green,
					src->stops->color.blue,
					src->stops->color.alpha);

		_cairo_pattern_init_solid (&solid, &color,
					   CAIRO_COLOR_IS_OPAQUE (&color) ?
					   CAIRO_CONTENT_COLOR :
					   CAIRO_CONTENT_COLOR_ALPHA);
	    }
	    else
	    {
		const cairo_color_t *color;

		color =	_cairo_stock_color (CAIRO_STOCK_TRANSPARENT);
		_cairo_pattern_init_solid (&solid, color,
					   CAIRO_CONTENT_ALPHA);
	    }

	    status = _cairo_pattern_acquire_surface_for_solid (&solid, dst,
							       x, y,
							       width, height,
							       surface_out,
							       attributes);
	}
	else
	{
	    status = _cairo_pattern_acquire_surface_for_gradient (src, dst,
								  x, y,
								  width, height,
								  surface_out,
								  attributes);
	}
    } break;
    case CAIRO_PATTERN_TYPE_SURFACE: {
	cairo_surface_pattern_t *src = (cairo_surface_pattern_t *) pattern;

	status = _cairo_pattern_acquire_surface_for_surface (src, dst,
							     x, y, width, height,
							     surface_out,
							     attributes);
    } break;
    default:
	ASSERT_NOT_REACHED;
	status = _cairo_error (CAIRO_STATUS_PATTERN_TYPE_MISMATCH);
    }

    return status;
}









void
_cairo_pattern_release_surface (cairo_pattern_t		   *pattern,
				cairo_surface_t		   *surface,
				cairo_surface_attributes_t *attributes)
{
    if (attributes->acquired)
    {
	cairo_surface_pattern_t *surface_pattern;

	assert (pattern->type == CAIRO_PATTERN_TYPE_SURFACE);
	surface_pattern = (cairo_surface_pattern_t *) pattern;

	_cairo_surface_release_source_image (surface_pattern->surface,
					     (cairo_image_surface_t *) surface,
					     attributes->extra);
    }
    else
    {
	cairo_surface_destroy (surface);
    }
}

cairo_int_status_t
_cairo_pattern_acquire_surfaces (cairo_pattern_t	    *src,
				 cairo_pattern_t	    *mask,
				 cairo_surface_t	    *dst,
				 int			    src_x,
				 int			    src_y,
				 int			    mask_x,
				 int			    mask_y,
				 unsigned int		    width,
				 unsigned int		    height,
				 cairo_surface_t	    **src_out,
				 cairo_surface_t	    **mask_out,
				 cairo_surface_attributes_t *src_attributes,
				 cairo_surface_attributes_t *mask_attributes)
{
    cairo_int_status_t	  status;
    cairo_pattern_union_t src_tmp, mask_tmp;

    if (src->status)
	return src->status;
    if (mask && mask->status)
	return mask->status;

    


    


    if (src->type == CAIRO_PATTERN_TYPE_SOLID &&
	mask && mask->type == CAIRO_PATTERN_TYPE_SOLID)
    {
	cairo_color_t combined;
	cairo_solid_pattern_t *src_solid = (cairo_solid_pattern_t *) src;
	cairo_solid_pattern_t *mask_solid = (cairo_solid_pattern_t *) mask;

	combined = src_solid->color;
	_cairo_color_multiply_alpha (&combined, mask_solid->color.alpha);

	_cairo_pattern_init_solid (&src_tmp.solid, &combined,
				   CAIRO_COLOR_IS_OPAQUE (&combined) ?
				   CAIRO_CONTENT_COLOR :
				   CAIRO_CONTENT_COLOR_ALPHA);

	mask = NULL;
    }
    else
    {
	status = _cairo_pattern_init_copy (&src_tmp.base, src);
	if (status)
	    return status;
    }

    status = _cairo_pattern_acquire_surface (&src_tmp.base, dst,
					     src_x, src_y,
					     width, height,
					     src_out, src_attributes);
    if (status) {
	_cairo_pattern_fini (&src_tmp.base);
	return status;
    }

    if (mask == NULL)
    {
	_cairo_pattern_fini (&src_tmp.base);
	*mask_out = NULL;
	return CAIRO_STATUS_SUCCESS;
    }

    status = _cairo_pattern_init_copy (&mask_tmp.base, mask);
    if (status)
	goto CLEANUP_SOURCE;

    status = _cairo_pattern_acquire_surface (&mask_tmp.base, dst,
					     mask_x, mask_y,
					     width, height,
					     mask_out, mask_attributes);

    _cairo_pattern_fini (&mask_tmp.base);

CLEANUP_SOURCE:
    if (status)
	_cairo_pattern_release_surface (&src_tmp.base,
					*src_out, src_attributes);

    _cairo_pattern_fini (&src_tmp.base);

    return status;
}













cairo_status_t
_cairo_pattern_get_extents (cairo_pattern_t         *pattern,
			    cairo_rectangle_int_t   *extents)
{
    if (pattern->extend == CAIRO_EXTEND_NONE &&
	pattern->type == CAIRO_PATTERN_TYPE_SURFACE)
    {
	cairo_status_t status;
	cairo_rectangle_int_t surface_extents;
	cairo_surface_pattern_t *surface_pattern =
	    (cairo_surface_pattern_t *) pattern;
	cairo_surface_t *surface = surface_pattern->surface;
	cairo_matrix_t imatrix;
	double x1, y1, x2, y2;

	status = _cairo_surface_get_extents (surface, &surface_extents);
	if (status)
	    return status;

	x1 = surface_extents.x;
	y1 = surface_extents.y;
	x2 = x1 + surface_extents.width;
	y2 = y1 + surface_extents.height;

	




	switch (pattern->filter) {
	case CAIRO_FILTER_GOOD:
	case CAIRO_FILTER_BEST:
	case CAIRO_FILTER_BILINEAR:
	    x1 -= 0.5;
	    y1 -= 0.5;
	    x2 += 0.5;
	    y2 += 0.5;
	    break;
	case CAIRO_FILTER_FAST:
	case CAIRO_FILTER_NEAREST:
	case CAIRO_FILTER_GAUSSIAN:
	default:
	    
	    break;
	}

	imatrix = pattern->matrix;
	status = cairo_matrix_invert (&imatrix);
	
	assert (status == CAIRO_STATUS_SUCCESS);

	_cairo_matrix_transform_bounding_box (&imatrix,
					      &x1, &y1, &x2, &y2,
					      NULL);

	x1 = floor (x1);
	if (x1 < 0)
	    x1 = 0;
	y1 = floor (y1);
	if (y1 < 0)
	    y1 = 0;

	x2 = ceil (x2);
	if (x2 > CAIRO_RECT_INT_MAX)
	    x2 = CAIRO_RECT_INT_MAX;
	y2 = ceil (y2);
	if (y2 > CAIRO_RECT_INT_MAX)
	    y2 = CAIRO_RECT_INT_MAX;

	extents->x = x1; extents->width = x2 - x1;
	extents->y = y1; extents->height = y2 - y1;

	return CAIRO_STATUS_SUCCESS;
    }

    




    




    extents->x = 0;
    extents->y = 0;
    extents->width = CAIRO_RECT_INT_MAX;
    extents->height = CAIRO_RECT_INT_MAX;

    return CAIRO_STATUS_SUCCESS;
}

















cairo_status_t
cairo_pattern_get_rgba (cairo_pattern_t *pattern,
			double *red, double *green,
			double *blue, double *alpha)
{
    cairo_solid_pattern_t *solid = (cairo_solid_pattern_t*) pattern;
    double r0, g0, b0, a0;

    if (pattern->type != CAIRO_PATTERN_TYPE_SOLID)
	return _cairo_error (CAIRO_STATUS_PATTERN_TYPE_MISMATCH);

    _cairo_color_get_rgba (&solid->color, &r0, &g0, &b0, &a0);

    if (red)
	*red = r0;
    if (green)
	*green = g0;
    if (blue)
	*blue = b0;
    if (alpha)
	*alpha = a0;

    return CAIRO_STATUS_SUCCESS;
}
















cairo_status_t
cairo_pattern_get_surface (cairo_pattern_t *pattern,
			   cairo_surface_t **surface)
{
    cairo_surface_pattern_t *spat = (cairo_surface_pattern_t*) pattern;

    if (pattern->type != CAIRO_PATTERN_TYPE_SURFACE)
	return _cairo_error (CAIRO_STATUS_PATTERN_TYPE_MISMATCH);

    if (surface)
	*surface = spat->surface;

    return CAIRO_STATUS_SUCCESS;
}






















cairo_status_t
cairo_pattern_get_color_stop_rgba (cairo_pattern_t *pattern,
				   int index, double *offset,
				   double *red, double *green,
				   double *blue, double *alpha)
{
    cairo_gradient_pattern_t *gradient = (cairo_gradient_pattern_t*) pattern;

    if (pattern->type != CAIRO_PATTERN_TYPE_LINEAR &&
	pattern->type != CAIRO_PATTERN_TYPE_RADIAL)
	return _cairo_error (CAIRO_STATUS_PATTERN_TYPE_MISMATCH);

    if (index < 0 || (unsigned int) index >= gradient->n_stops)
	return _cairo_error (CAIRO_STATUS_INVALID_INDEX);

    if (offset)
	*offset = gradient->stops[index].offset;
    if (red)
	*red = gradient->stops[index].color.red;
    if (green)
	*green = gradient->stops[index].color.green;
    if (blue)
	*blue = gradient->stops[index].color.blue;
    if (alpha)
	*alpha = gradient->stops[index].color.alpha;

    return CAIRO_STATUS_SUCCESS;
}















cairo_status_t
cairo_pattern_get_color_stop_count (cairo_pattern_t *pattern,
				    int *count)
{
    cairo_gradient_pattern_t *gradient = (cairo_gradient_pattern_t*) pattern;

    if (pattern->type != CAIRO_PATTERN_TYPE_LINEAR &&
	pattern->type != CAIRO_PATTERN_TYPE_RADIAL)
	return _cairo_error (CAIRO_STATUS_PATTERN_TYPE_MISMATCH);

    if (count)
	*count = gradient->n_stops;

    return CAIRO_STATUS_SUCCESS;
}

















cairo_status_t
cairo_pattern_get_linear_points (cairo_pattern_t *pattern,
				 double *x0, double *y0,
				 double *x1, double *y1)
{
    cairo_linear_pattern_t *linear = (cairo_linear_pattern_t*) pattern;

    if (pattern->type != CAIRO_PATTERN_TYPE_LINEAR)
	return _cairo_error (CAIRO_STATUS_PATTERN_TYPE_MISMATCH);

    if (x0)
	*x0 = _cairo_fixed_to_double (linear->p1.x);
    if (y0)
	*y0 = _cairo_fixed_to_double (linear->p1.y);
    if (x1)
	*x1 = _cairo_fixed_to_double (linear->p2.x);
    if (y1)
	*y1 = _cairo_fixed_to_double (linear->p2.y);

    return CAIRO_STATUS_SUCCESS;
}




















cairo_status_t
cairo_pattern_get_radial_circles (cairo_pattern_t *pattern,
				  double *x0, double *y0, double *r0,
				  double *x1, double *y1, double *r1)
{
    cairo_radial_pattern_t *radial = (cairo_radial_pattern_t*) pattern;

    if (pattern->type != CAIRO_PATTERN_TYPE_RADIAL)
	return _cairo_error (CAIRO_STATUS_PATTERN_TYPE_MISMATCH);

    if (x0)
	*x0 = _cairo_fixed_to_double (radial->c1.x);
    if (y0)
	*y0 = _cairo_fixed_to_double (radial->c1.y);
    if (r0)
	*r0 = _cairo_fixed_to_double (radial->r1);
    if (x1)
	*x1 = _cairo_fixed_to_double (radial->c2.x);
    if (y1)
	*y1 = _cairo_fixed_to_double (radial->c2.y);
    if (r1)
	*r1 = _cairo_fixed_to_double (radial->r2);

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_pattern_reset_static_data (void)
{
    _cairo_pattern_reset_solid_pattern_cache ();
    _cairo_pattern_reset_solid_surface_cache ();
}
