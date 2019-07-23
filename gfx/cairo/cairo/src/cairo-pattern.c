





























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

const cairo_solid_pattern_t _cairo_pattern_black = {
    { CAIRO_PATTERN_TYPE_SOLID,		
      CAIRO_REFERENCE_COUNT_INVALID,	
      CAIRO_STATUS_SUCCESS,		
      { 0, 0, 0, NULL },		
      { 1., 0., 0., 1., 0., 0., },	
      CAIRO_FILTER_DEFAULT,		
      CAIRO_EXTEND_GRADIENT_DEFAULT},	
    { 0., 0., 0., 1., 0, 0, 0, 0xffff },
    CAIRO_CONTENT_COLOR,                
};


















static cairo_status_t
_cairo_pattern_set_error (cairo_pattern_t *pattern,
			  cairo_status_t status)
{
    if (status == CAIRO_STATUS_SUCCESS)
	return status;

    

    _cairo_status_set_error (&pattern->status, status);

    return _cairo_error (status);
}

static void
_cairo_pattern_init (cairo_pattern_t *pattern, cairo_pattern_type_t type)
{
    pattern->type      = type;
    pattern->status    = CAIRO_STATUS_SUCCESS;

    

    CAIRO_REFERENCE_COUNT_INIT (&pattern->ref_count, 0);

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
    if (CAIRO_INJECT_FAULT ())
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

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
	if (unlikely (pattern->stops == NULL)) {
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
	if (unlikely (status))
	    return status;

    } break;
    }

    
    CAIRO_REFERENCE_COUNT_INIT (&pattern->ref_count, 0);
    _cairo_user_data_array_init (&pattern->user_data);

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_pattern_init_snapshot (cairo_pattern_t       *pattern,
			      const cairo_pattern_t *other)
{
    cairo_status_t status;

    

    status = _cairo_pattern_init_copy (pattern, other);
    if (unlikely (status))
	return status;

    

    if (pattern->type == CAIRO_PATTERN_TYPE_SURFACE) {
	cairo_surface_pattern_t *surface_pattern =
	    (cairo_surface_pattern_t *) pattern;
	cairo_surface_t *surface = surface_pattern->surface;

	surface_pattern->surface = _cairo_surface_snapshot (surface);

	cairo_surface_destroy (surface);

	if (surface_pattern->surface->status)
	    return surface_pattern->surface->status;
    }

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
_cairo_pattern_create_copy (cairo_pattern_t	  **pattern_out,
			    const cairo_pattern_t  *other)
{
    cairo_pattern_t *pattern;
    cairo_status_t status;

    if (other->status)
	return other->status;

    switch (other->type) {
    case CAIRO_PATTERN_TYPE_SOLID:
	pattern = malloc (sizeof (cairo_solid_pattern_t));
	break;
    case CAIRO_PATTERN_TYPE_SURFACE:
	pattern = malloc (sizeof (cairo_surface_pattern_t));
	break;
    case CAIRO_PATTERN_TYPE_LINEAR:
	pattern = malloc (sizeof (cairo_linear_pattern_t));
	break;
    case CAIRO_PATTERN_TYPE_RADIAL:
	pattern = malloc (sizeof (cairo_radial_pattern_t));
	break;
    default:
	ASSERT_NOT_REACHED;
	return _cairo_error (CAIRO_STATUS_PATTERN_TYPE_MISMATCH);
    }
    if (unlikely (pattern == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    status = _cairo_pattern_init_copy (pattern, other);
    if (unlikely (status)) {
	free (pattern);
	return status;
    }

    CAIRO_REFERENCE_COUNT_INIT (&pattern->ref_count, 1);
    *pattern_out = pattern;
    return CAIRO_STATUS_SUCCESS;
}


void
_cairo_pattern_init_solid (cairo_solid_pattern_t *pattern,
			   const cairo_color_t	 *color,
			   cairo_content_t	  content)
{
    if (content == CAIRO_CONTENT_COLOR_ALPHA && CAIRO_COLOR_IS_OPAQUE (color))
	content = CAIRO_CONTENT_COLOR;

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

    if (unlikely (pattern == NULL)) {
	
	pattern = malloc (sizeof (cairo_solid_pattern_t));
	if (unlikely (pattern == NULL)) {
	    _cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	    return (cairo_pattern_t *) &_cairo_pattern_nil;
	}
    }

    _cairo_pattern_init_solid (pattern, color, content);
    CAIRO_REFERENCE_COUNT_INIT (&pattern->base.ref_count, 1);

    return &pattern->base;
}

static void
_cairo_pattern_reset_solid_pattern_cache (void)
{
    int i;

    CAIRO_MUTEX_LOCK (_cairo_pattern_solid_pattern_cache_lock);

    for (i = 0; i < MIN (ARRAY_LENGTH (solid_pattern_cache.patterns), solid_pattern_cache.size); i++) {
	if (solid_pattern_cache.patterns[i])
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

    pattern = _cairo_pattern_create_solid (CAIRO_COLOR_BLACK, CAIRO_CONTENT_COLOR);
    if (pattern->status == CAIRO_STATUS_SUCCESS)
	status = _cairo_pattern_set_error (pattern, status);

    return pattern;
}





















cairo_pattern_t *
cairo_pattern_create_rgb (double red, double green, double blue)
{
    cairo_color_t color;

    red   = _cairo_restrict_value (red,   0.0, 1.0);
    green = _cairo_restrict_value (green, 0.0, 1.0);
    blue  = _cairo_restrict_value (blue,  0.0, 1.0);

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

    red   = _cairo_restrict_value (red,   0.0, 1.0);
    green = _cairo_restrict_value (green, 0.0, 1.0);
    blue  = _cairo_restrict_value (blue,  0.0, 1.0);
    alpha = _cairo_restrict_value (alpha, 0.0, 1.0);

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
    if (unlikely (pattern == NULL)) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_pattern_t *)&_cairo_pattern_nil.base;
    }

    CAIRO_MUTEX_INITIALIZE ();

    _cairo_pattern_init_for_surface (pattern, surface);
    CAIRO_REFERENCE_COUNT_INIT (&pattern->base.ref_count, 1);

    return &pattern->base;
}
slim_hidden_def (cairo_pattern_create_for_surface);



























cairo_pattern_t *
cairo_pattern_create_linear (double x0, double y0, double x1, double y1)
{
    cairo_linear_pattern_t *pattern;

    pattern = malloc (sizeof (cairo_linear_pattern_t));
    if (unlikely (pattern == NULL)) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_pattern_t *) &_cairo_pattern_nil.base;
    }

    CAIRO_MUTEX_INITIALIZE ();

    _cairo_pattern_init_linear (pattern, x0, y0, x1, y1);
    CAIRO_REFERENCE_COUNT_INIT (&pattern->base.base.ref_count, 1);

    return &pattern->base.base;
}





























cairo_pattern_t *
cairo_pattern_create_radial (double cx0, double cy0, double radius0,
			     double cx1, double cy1, double radius1)
{
    cairo_radial_pattern_t *pattern;

    pattern = malloc (sizeof (cairo_radial_pattern_t));
    if (unlikely (pattern == NULL)) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_pattern_t *) &_cairo_pattern_nil.base;
    }

    CAIRO_MUTEX_INITIALIZE ();

    _cairo_pattern_init_radial (pattern, cx0, cy0, radius0, cx1, cy1, radius1);
    CAIRO_REFERENCE_COUNT_INIT (&pattern->base.base.ref_count, 1);

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
	return pattern->status;

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

    if (CAIRO_INJECT_FAULT ())
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

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

    if (unlikely (new_stops == NULL))
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
	if (unlikely (status)) {
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

    offset = _cairo_restrict_value (offset, 0.0, 1.0);
    red    = _cairo_restrict_value (red,    0.0, 1.0);
    green  = _cairo_restrict_value (green,  0.0, 1.0);
    blue   = _cairo_restrict_value (blue,   0.0, 1.0);

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

    offset = _cairo_restrict_value (offset, 0.0, 1.0);
    red    = _cairo_restrict_value (red,    0.0, 1.0);
    green  = _cairo_restrict_value (green,  0.0, 1.0);
    blue   = _cairo_restrict_value (blue,   0.0, 1.0);
    alpha  = _cairo_restrict_value (alpha,  0.0, 1.0);

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

    if (memcmp (&pattern->matrix, matrix, sizeof (cairo_matrix_t)) == 0)
	return;

    pattern->matrix = *matrix;

    inverse = *matrix;
    status = cairo_matrix_invert (&inverse);
    if (unlikely (status))
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
_cairo_pattern_acquire_surface_for_gradient (const cairo_gradient_pattern_t *pattern,
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
    cairo_bool_t          opaque = TRUE;

    pixman_gradient_stop_t pixman_stops_static[2];
    pixman_gradient_stop_t *pixman_stops = pixman_stops_static;
    unsigned int i;
    int clone_offset_x, clone_offset_y;
    cairo_matrix_t matrix = pattern->base.matrix;

    if (CAIRO_INJECT_FAULT ())
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    if (pattern->n_stops > ARRAY_LENGTH(pixman_stops_static)) {
	pixman_stops = _cairo_malloc_ab (pattern->n_stops,
					 sizeof(pixman_gradient_stop_t));
	if (unlikely (pixman_stops == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    for (i = 0; i < pattern->n_stops; i++) {
	pixman_stops[i].x = _cairo_fixed_16_16_from_double (pattern->stops[i].offset);
	pixman_stops[i].color.red = pattern->stops[i].color.red_short;
	pixman_stops[i].color.green = pattern->stops[i].color.green_short;
	pixman_stops[i].color.blue = pattern->stops[i].color.blue_short;
	pixman_stops[i].color.alpha = pattern->stops[i].color.alpha_short;
	if (! CAIRO_ALPHA_SHORT_IS_OPAQUE (pixman_stops[i].color.alpha))
	    opaque = FALSE;
    }

    if (pattern->base.type == CAIRO_PATTERN_TYPE_LINEAR)
    {
	cairo_linear_pattern_t *linear = (cairo_linear_pattern_t *) pattern;
	pixman_point_fixed_t p1, p2;
	cairo_fixed_t xdim, ydim;

	xdim = linear->p2.x - linear->p1.x;
	ydim = linear->p2.y - linear->p1.y;

	









#define PIXMAN_MAX_INT ((pixman_fixed_1 >> 1) - pixman_fixed_e) /* need to ensure deltas also fit */
	if (_cairo_fixed_integer_ceil (xdim) > PIXMAN_MAX_INT ||
	    _cairo_fixed_integer_ceil (ydim) > PIXMAN_MAX_INT)
	{
	    double sf;

	    if (xdim > ydim)
		sf = PIXMAN_MAX_INT / _cairo_fixed_to_double (xdim);
	    else
		sf = PIXMAN_MAX_INT / _cairo_fixed_to_double (ydim);

	    p1.x = _cairo_fixed_16_16_from_double (_cairo_fixed_to_double (linear->p1.x) * sf);
	    p1.y = _cairo_fixed_16_16_from_double (_cairo_fixed_to_double (linear->p1.y) * sf);
	    p2.x = _cairo_fixed_16_16_from_double (_cairo_fixed_to_double (linear->p2.x) * sf);
	    p2.y = _cairo_fixed_16_16_from_double (_cairo_fixed_to_double (linear->p2.y) * sf);

	    cairo_matrix_scale (&matrix, sf, sf);
	}
	else
	{
	    p1.x = _cairo_fixed_to_16_16 (linear->p1.x);
	    p1.y = _cairo_fixed_to_16_16 (linear->p1.y);
	    p2.x = _cairo_fixed_to_16_16 (linear->p2.x);
	    p2.y = _cairo_fixed_to_16_16 (linear->p2.y);
	}

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

    if (unlikely (pixman_image == NULL))
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
	attr->matrix = matrix;
	attr->extend = pattern->base.extend;
	attr->filter = CAIRO_FILTER_NEAREST;

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

    if (! pixman_image_set_filter (pixman_image, PIXMAN_FILTER_BILINEAR,
				   NULL, 0))
    {
	pixman_image_unref (pixman_image);
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    image = (cairo_image_surface_t *)
	cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    if (image->base.status) {
	pixman_image_unref (pixman_image);
	return image->base.status;
    }

    _cairo_matrix_to_pixman_matrix (&matrix, &pixman_transform,
				    width/2., height/2.);
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

    _cairo_debug_check_image_surface_is_defined (&image->base);

    status = _cairo_surface_clone_similar (dst, &image->base,
					   opaque ?
					   CAIRO_CONTENT_COLOR :
					   CAIRO_CONTENT_COLOR_ALPHA,
					   0, 0, width, height,
					   &clone_offset_x,
					   &clone_offset_y,
					   out);

    cairo_surface_destroy (&image->base);

    attr->x_offset = -x;
    attr->y_offset = -y;
    cairo_matrix_init_identity (&attr->matrix);
    attr->extend = repeat ? CAIRO_EXTEND_REPEAT : CAIRO_EXTEND_NONE;
    attr->filter = CAIRO_FILTER_NEAREST;

    return status;
}



#define MAX_SURFACE_CACHE_SIZE 16
static struct {
    struct _cairo_pattern_solid_surface_cache{
	cairo_content_t  content;
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
    if (cache->content != pattern->content)
	return FALSE;

    if (CAIRO_REFERENCE_COUNT_GET_VALUE (&cache->surface->ref_count) != 1)
	return FALSE;

    if (! _cairo_surface_is_similar (cache->surface, dst, pattern->content))
	return FALSE;

    return TRUE;
}

static cairo_bool_t
_cairo_pattern_solid_surface_matches_color (
	const struct _cairo_pattern_solid_surface_cache	    *cache,
	const cairo_solid_pattern_t			    *pattern,
	cairo_surface_t					    *dst)
{
    if (! _cairo_color_equal (&cache->color, &pattern->color))
	return FALSE;

    return _cairo_pattern_solid_surface_matches (cache, pattern, dst);
}

static cairo_int_status_t
_cairo_pattern_acquire_surface_for_solid (const cairo_solid_pattern_t	     *pattern,
					  cairo_surface_t	     *dst,
					  int			     x,
					  int			     y,
					  unsigned int		     width,
					  unsigned int		     height,
					  cairo_surface_t	     **out,
					  cairo_surface_attributes_t *attribs)
{
    static int i;

    cairo_surface_t *surface, *to_destroy = NULL;
    cairo_status_t   status;

    CAIRO_MUTEX_LOCK (_cairo_pattern_solid_surface_cache_lock);

    
    if (i < solid_surface_cache.size &&
	_cairo_pattern_solid_surface_matches_color (&solid_surface_cache.cache[i],
						    pattern,
						    dst))
    {
	status = _cairo_surface_reset (solid_surface_cache.cache[i].surface);
	if (unlikely (status))
	    goto UNLOCK;

	goto DONE;
    }

    for (i = 0 ; i < solid_surface_cache.size; i++) {
	if (_cairo_pattern_solid_surface_matches_color (&solid_surface_cache.cache[i],
							pattern,
							dst))
	{
	    status = _cairo_surface_reset (solid_surface_cache.cache[i].surface);
	    if (unlikely (status))
		goto UNLOCK;

	    goto DONE;
	}
    }

    
    surface = NULL;
    if (solid_surface_cache.size == MAX_SURFACE_CACHE_SIZE) {
	i = rand () % MAX_SURFACE_CACHE_SIZE;
	surface = solid_surface_cache.cache[i].surface;

	if (_cairo_pattern_solid_surface_matches (&solid_surface_cache.cache[i],
						  pattern,
						  dst))
	{
	    

	    status = _cairo_surface_reset (surface);
	    if (unlikely (status))
		goto EVICT;

	    status = _cairo_surface_repaint_solid_pattern_surface (dst, surface, pattern);
	    if (unlikely (status))
		goto EVICT;

	    cairo_surface_reference (surface);
	}
	else
	{
	  EVICT:
	    surface = NULL;
	}
    }

    if (surface == NULL) {
	
	surface = _cairo_surface_create_solid_pattern_surface (dst, pattern);
	if (surface->status) {
	    status = surface->status;
	    goto UNLOCK;
	}

	if (! _cairo_surface_is_similar (surface, dst, pattern->content)) {
	    

	    *out = surface;
	    goto NOCACHE;
	}
    }

    if (i == solid_surface_cache.size)
	solid_surface_cache.size++;

    to_destroy = solid_surface_cache.cache[i].surface;
    solid_surface_cache.cache[i].surface = surface;
    solid_surface_cache.cache[i].color   = pattern->color;
    solid_surface_cache.cache[i].content = pattern->content;

DONE:
    *out = cairo_surface_reference (solid_surface_cache.cache[i].surface);

NOCACHE:
    attribs->x_offset = attribs->y_offset = 0;
    cairo_matrix_init_identity (&attribs->matrix);
    attribs->extend = CAIRO_EXTEND_REPEAT;
    attribs->filter = CAIRO_FILTER_NEAREST;

    status = CAIRO_STATUS_SUCCESS;

UNLOCK:
    CAIRO_MUTEX_UNLOCK (_cairo_pattern_solid_surface_cache_lock);

    if (to_destroy)
      cairo_surface_destroy (to_destroy);

    return status;
}

static void
_cairo_pattern_reset_solid_surface_cache (void)
{
    CAIRO_MUTEX_LOCK (_cairo_pattern_solid_surface_cache_lock);

    

    while (solid_surface_cache.size) {
	cairo_surface_t *surface;

	solid_surface_cache.size--;
	surface = solid_surface_cache.cache[solid_surface_cache.size].surface;
	solid_surface_cache.cache[solid_surface_cache.size].surface = NULL;

	

	CAIRO_MUTEX_UNLOCK (_cairo_pattern_solid_surface_cache_lock);
	cairo_surface_destroy (surface);
	CAIRO_MUTEX_LOCK (_cairo_pattern_solid_surface_cache_lock);
    }

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















static cairo_filter_t
_cairo_pattern_analyze_filter (const cairo_surface_pattern_t *pattern,
			       double                        *pad_out)
{
    double pad;
    cairo_filter_t optimized_filter;

    switch (pattern->base.filter) {
    case CAIRO_FILTER_GOOD:
    case CAIRO_FILTER_BEST:
    case CAIRO_FILTER_BILINEAR:
	



	if (_cairo_matrix_is_pixel_exact (&pattern->base.matrix)) {
	    pad = 0.;
	    optimized_filter = CAIRO_FILTER_NEAREST;
	} else {
	    




	    pad = 0.5;
	    optimized_filter = pattern->base.filter;
	}
	break;

    case CAIRO_FILTER_FAST:
    case CAIRO_FILTER_NEAREST:
    case CAIRO_FILTER_GAUSSIAN:
    default:
	pad = 0.;
	optimized_filter = pattern->base.filter;
	break;
    }

    if (pad_out)
	*pad_out = pad;

    return optimized_filter;
}


static double
_pixman_nearest_sample (double d)
{
    return ceil (d - .5);
}

static cairo_int_status_t
_cairo_pattern_acquire_surface_for_surface (const cairo_surface_pattern_t   *pattern,
					    cairo_surface_t	       *dst,
					    cairo_content_t	    content,
					    int			       x,
					    int			       y,
					    unsigned int	       width,
					    unsigned int	       height,
					    cairo_surface_t	       **out,
					    cairo_surface_attributes_t *attr)
{
    cairo_surface_t *surface;
    cairo_rectangle_int_t extents;
    cairo_rectangle_int_t sampled_area;
    double x1, y1, x2, y2;
    int tx, ty;
    double pad;
    cairo_bool_t is_identity;
    cairo_bool_t is_empty;
    cairo_int_status_t status;

    surface = cairo_surface_reference (pattern->surface);

    is_identity = FALSE;
    attr->matrix = pattern->base.matrix;
    attr->extend = pattern->base.extend;
    attr->filter = _cairo_pattern_analyze_filter (pattern, &pad);

    attr->x_offset = attr->y_offset = tx = ty = 0;
    if (_cairo_matrix_is_integer_translation (&attr->matrix, &tx, &ty)) {
	cairo_matrix_init_identity (&attr->matrix);
	attr->x_offset = tx;
	attr->y_offset = ty;
	is_identity = TRUE;
    } else if (attr->filter == CAIRO_FILTER_NEAREST) {
	





	attr->matrix.x0 = 0;
	attr->matrix.y0 = 0;
	if (_cairo_matrix_is_pixel_exact (&attr->matrix)) {
	    


	    attr->matrix.x0 = _pixman_nearest_sample (pattern->base.matrix.x0);
	    attr->matrix.y0 = _pixman_nearest_sample (pattern->base.matrix.y0);
	} else {
	    attr->matrix.x0 = pattern->base.matrix.x0;
	    attr->matrix.y0 = pattern->base.matrix.y0;
	}

	if (_cairo_matrix_is_integer_translation (&attr->matrix, &tx, &ty)) {
	    cairo_matrix_init_identity (&attr->matrix);
	    attr->x_offset = tx;
	    attr->y_offset = ty;
	    is_identity = TRUE;
	}
    }

    






    if (attr->extend == CAIRO_EXTEND_REFLECT) {
	cairo_t *cr;
	cairo_surface_t *src;
	int w, h;

	status = _cairo_surface_get_extents (surface, &extents);
	if (unlikely (status))
	    goto BAIL;

	status = _cairo_surface_clone_similar (dst, surface, content,
					       extents.x, extents.y,
					       extents.width, extents.height,
					       &extents.x, &extents.y, &src);
	if (unlikely (status))
	    goto BAIL;

	w = 2 * extents.width;
	h = 2 * extents.height;

	if (is_identity) {
	    attr->x_offset = -x;
	    x += tx;
	    while (x <= -w)
		x += w;
	    while (x >= w)
		x -= w;
	    extents.x += x;
	    tx = x = 0;

	    attr->y_offset = -y;
	    y += ty;
	    while (y <= -h)
		y += h;
	    while (y >= h)
		y -= h;
	    extents.y += y;
	    ty = y = 0;
	}

	cairo_surface_destroy (surface);
	surface = cairo_surface_create_similar (dst, dst->content, w, h);
	if (surface->status) {
	    cairo_surface_destroy (src);
	    return surface->status;
	}

	surface->device_transform = pattern->surface->device_transform;
	surface->device_transform_inverse = pattern->surface->device_transform_inverse;

	cr = cairo_create (surface);

	cairo_set_source_surface (cr, src, -extents.x, -extents.y);
	cairo_paint (cr);

	cairo_scale (cr, -1, +1);
	cairo_set_source_surface (cr, src, extents.x-w, -extents.y);
	cairo_paint (cr);
	cairo_set_source_surface (cr, src, extents.x, -extents.y);
	cairo_paint (cr);

	cairo_scale (cr, +1, -1);
	cairo_set_source_surface (cr, src, extents.x-w, extents.y-h);
	cairo_paint (cr);
	cairo_set_source_surface (cr, src, extents.x, extents.y-h);
	cairo_paint (cr);
	cairo_set_source_surface (cr, src, extents.x-w, extents.y);
	cairo_paint (cr);
	cairo_set_source_surface (cr, src, extents.x, extents.y);
	cairo_paint (cr);

	cairo_scale (cr, -1, +1);
	cairo_set_source_surface (cr, src, -extents.x, extents.y-h);
	cairo_paint (cr);
	cairo_set_source_surface (cr, src, -extents.x, extents.y);
	cairo_paint (cr);

	status = cairo_status (cr);
	cairo_destroy (cr);

	cairo_surface_destroy (src);

	if (unlikely (status))
	    goto BAIL;

	attr->extend = CAIRO_EXTEND_REPEAT;
    }

    status = _cairo_surface_get_extents (surface, &extents);
    if (unlikely (status))
	goto BAIL;

    



    x1 = x;
    y1 = y;
    x2 = x + (int) width;
    y2 = y + (int) height;
    if (! is_identity) {
	_cairo_matrix_transform_bounding_box (&attr->matrix,
					      &x1, &y1, &x2, &y2,
					      NULL);
    }

    sampled_area.x = floor (x1 - pad);
    sampled_area.y = floor (y1 - pad);
    sampled_area.width  = ceil (x2 + pad) - sampled_area.x;
    sampled_area.height = ceil (y2 + pad) - sampled_area.y;

    sampled_area.x += tx;
    sampled_area.y += ty;

    if (attr->extend != CAIRO_EXTEND_REPEAT) {
	
	is_empty = _cairo_rectangle_intersect (&extents, &sampled_area);
    } else {
	if (sampled_area.x >= extents.x &&
	    sampled_area.y >= extents.y &&
	    sampled_area.x + (int) sampled_area.width <= extents.x + (int) extents.width &&
	    sampled_area.y + (int) sampled_area.height <= extents.y + (int) extents.height)
	{
	    
	    extents = sampled_area;
	    attr->extend = CAIRO_EXTEND_NONE;
	}

	is_empty = extents.width == 0 || extents.height == 0;
    }

    

    status = _cairo_surface_clone_similar (dst, surface, content,
					   extents.x, extents.y,
					   extents.width, extents.height,
					   &x, &y, out);
    if (unlikely (status))
	goto BAIL;

    if (x != 0 || y != 0) {
	if (is_identity) {
	    attr->x_offset -= x;
	    attr->y_offset -= y;
	} else {
	    cairo_matrix_t m;

	    x -= attr->x_offset;
	    y -= attr->y_offset;
	    attr->x_offset = 0;
	    attr->y_offset = 0;

	    cairo_matrix_init_translate (&m, -x, -y);
	    cairo_matrix_multiply (&attr->matrix, &attr->matrix, &m);
	}
    }

    
    if (! is_identity) {
	cairo_matrix_t m;
	cairo_status_t invert_status;

	m = attr->matrix;
	invert_status = cairo_matrix_invert (&m);
	assert (invert_status == CAIRO_STATUS_SUCCESS);

	if (m.x0 != 0. || m.y0 != 0.) {
	    


	    x = floor (m.x0 / 2);
	    y = floor (m.y0 / 2);
	    attr->x_offset -= x;
	    attr->y_offset -= y;
	    cairo_matrix_init_translate (&m, x, y);
	    cairo_matrix_multiply (&attr->matrix, &m, &attr->matrix);
	}
    }

  BAIL:
    cairo_surface_destroy (surface);
    return status;
}





















cairo_int_status_t
_cairo_pattern_acquire_surface (const cairo_pattern_t	   *pattern,
				cairo_surface_t		   *dst,
				cairo_content_t		    content,
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
		_cairo_pattern_init_solid (&solid,
					   &src->stops->color,
					   CAIRO_CONTENT_COLOR_ALPHA);
	    }
	    else
	    {
		_cairo_pattern_init_solid (&solid,
					   CAIRO_COLOR_TRANSPARENT,
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
	    unsigned int i;

	    


	    for (i = 1; i < src->n_stops; i++) {
		if (! _cairo_color_equal (&src->stops[0].color,
					  &src->stops[i].color))
		{
		    break;
		}
	    }
	    if (i == src->n_stops) {
		cairo_solid_pattern_t solid;

		_cairo_pattern_init_solid (&solid,
					   &src->stops->color,
					   CAIRO_CONTENT_COLOR_ALPHA);

		status =
		    _cairo_pattern_acquire_surface_for_solid (&solid, dst,
							      x, y,
							      width, height,
							      surface_out,
							      attributes);
	    } else {
		status =
		    _cairo_pattern_acquire_surface_for_gradient (src, dst,
								 x, y,
								 width, height,
								 surface_out,
								 attributes);
	    }
	}
    } break;
    case CAIRO_PATTERN_TYPE_SURFACE: {
	cairo_surface_pattern_t *src = (cairo_surface_pattern_t *) pattern;

	status = _cairo_pattern_acquire_surface_for_surface (src, dst,
							     content,
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
_cairo_pattern_release_surface (const cairo_pattern_t *pattern,
				cairo_surface_t		   *surface,
				cairo_surface_attributes_t *attributes)
{
    cairo_surface_destroy (surface);
}

cairo_int_status_t
_cairo_pattern_acquire_surfaces (const cairo_pattern_t	    *src,
				 const cairo_pattern_t	    *mask,
				 cairo_surface_t	    *dst,
				 cairo_content_t	    src_content,
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
    cairo_pattern_union_t src_tmp;

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
				   (src_solid->content | mask_solid->content) & src_content);

	src = &src_tmp.base;
	mask = NULL;
    }

    status = _cairo_pattern_acquire_surface (src, dst,
					     src_content,
					     src_x, src_y,
					     width, height,
					     src_out, src_attributes);
    if (unlikely (status))
	goto BAIL;

    if (mask == NULL) {
	*mask_out = NULL;
	goto BAIL;
    }

    status = _cairo_pattern_acquire_surface (mask, dst,
					     CAIRO_CONTENT_ALPHA,
					     mask_x, mask_y,
					     width, height,
					     mask_out, mask_attributes);
    if (unlikely (status))
	_cairo_pattern_release_surface (src, *src_out, src_attributes);

  BAIL:
    if (src == &src_tmp.base)
	_cairo_pattern_fini (&src_tmp.base);

    return status;
}













cairo_status_t
_cairo_pattern_get_extents (const cairo_pattern_t         *pattern,
			    cairo_rectangle_int_t         *extents)
{
    if (pattern->extend == CAIRO_EXTEND_NONE &&
	pattern->type == CAIRO_PATTERN_TYPE_SURFACE)
    {
	cairo_status_t status;
	cairo_rectangle_int_t surface_extents;
	const cairo_surface_pattern_t *surface_pattern =
	    (const cairo_surface_pattern_t *) pattern;
	cairo_surface_t *surface = surface_pattern->surface;
	cairo_matrix_t imatrix;
	double x1, y1, x2, y2;
	double pad;

	status = _cairo_surface_get_extents (surface, &surface_extents);
	if (status == CAIRO_INT_STATUS_UNSUPPORTED)
	    goto UNBOUNDED;
	if (unlikely (status))
	    return status;

	


	_cairo_pattern_analyze_filter (surface_pattern, &pad);
	x1 = surface_extents.x - pad;
	y1 = surface_extents.y - pad;
	x2 = surface_extents.x + (int) surface_extents.width  + pad;
	y2 = surface_extents.y + (int) surface_extents.height + pad;

	imatrix = pattern->matrix;
	status = cairo_matrix_invert (&imatrix);
	
	assert (status == CAIRO_STATUS_SUCCESS);

	_cairo_matrix_transform_bounding_box (&imatrix,
					      &x1, &y1, &x2, &y2,
					      NULL);

	x1 = floor (x1);
	if (x1 < CAIRO_RECT_INT_MIN)
	    x1 = CAIRO_RECT_INT_MIN;
	y1 = floor (y1);
	if (y1 < CAIRO_RECT_INT_MIN)
	    y1 = CAIRO_RECT_INT_MIN;

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

    




  UNBOUNDED:
    
    extents->x = CAIRO_RECT_INT_MIN;
    extents->y = CAIRO_RECT_INT_MIN;
    extents->width = CAIRO_RECT_INT_MAX - CAIRO_RECT_INT_MIN;
    extents->height = CAIRO_RECT_INT_MAX - CAIRO_RECT_INT_MIN;

    return CAIRO_STATUS_SUCCESS;
}


static unsigned long
_cairo_solid_pattern_hash (unsigned long hash,
			   const cairo_pattern_t *pattern)
{
    const cairo_solid_pattern_t *solid = (cairo_solid_pattern_t *) pattern;

    hash = _cairo_hash_bytes (hash, &solid->content, sizeof (solid->content));
    hash = _cairo_hash_bytes (hash, &solid->color, sizeof (solid->color));

    return hash;
}

static unsigned long
_cairo_gradient_color_stops_hash (unsigned long hash,
				  const cairo_gradient_pattern_t *gradient)
{
    unsigned int n;

    hash = _cairo_hash_bytes (hash,
			      &gradient->n_stops,
			      sizeof (gradient->n_stops));

    for (n = 0; n < gradient->n_stops; n++) {
	hash = _cairo_hash_bytes (hash,
				  &gradient->stops[n].offset,
				  sizeof (double));
	hash = _cairo_hash_bytes (hash,
				  &gradient->stops[n].color,
				  sizeof (cairo_color_t));
    }

    return hash;
}

static unsigned long
_cairo_linear_pattern_hash (unsigned long hash,
			    const cairo_pattern_t *pattern)
{
    const cairo_linear_pattern_t *linear = (cairo_linear_pattern_t *) pattern;

    hash = _cairo_hash_bytes (hash, &linear->p1, sizeof (linear->p1));
    hash = _cairo_hash_bytes (hash, &linear->p2, sizeof (linear->p2));

    return _cairo_gradient_color_stops_hash (hash, &linear->base);
}

static unsigned long
_cairo_radial_pattern_hash (unsigned long hash, const cairo_pattern_t *pattern)
{
    const cairo_radial_pattern_t *radial = (cairo_radial_pattern_t *) pattern;

    hash = _cairo_hash_bytes (hash, &radial->c1, sizeof (radial->c1));
    hash = _cairo_hash_bytes (hash, &radial->r1, sizeof (radial->r1));
    hash = _cairo_hash_bytes (hash, &radial->c2, sizeof (radial->c2));
    hash = _cairo_hash_bytes (hash, &radial->r2, sizeof (radial->r2));

    return _cairo_gradient_color_stops_hash (hash, &radial->base);
}

static unsigned long
_cairo_surface_pattern_hash (unsigned long hash,
			     const cairo_pattern_t *pattern)
{
    
    return hash;
}

unsigned long
_cairo_pattern_hash (const cairo_pattern_t *pattern)
{
    unsigned long hash = _CAIRO_HASH_INIT_VALUE;

    if (pattern->status)
	return 0;

    hash = _cairo_hash_bytes (hash, &pattern->type, sizeof (pattern->type));
    hash = _cairo_hash_bytes (hash, &pattern->matrix, sizeof (pattern->matrix));
    hash = _cairo_hash_bytes (hash, &pattern->filter, sizeof (pattern->filter));
    hash = _cairo_hash_bytes (hash, &pattern->extend, sizeof (pattern->extend));

    switch (pattern->type) {
    case CAIRO_PATTERN_TYPE_SOLID:
	return _cairo_solid_pattern_hash (hash, pattern);
    case CAIRO_PATTERN_TYPE_LINEAR:
	return _cairo_linear_pattern_hash (hash, pattern);
    case CAIRO_PATTERN_TYPE_RADIAL:
	return _cairo_radial_pattern_hash (hash, pattern);
    case CAIRO_PATTERN_TYPE_SURFACE:
	return _cairo_surface_pattern_hash (hash, pattern);
    default:
	ASSERT_NOT_REACHED;
	return FALSE;
    }
}

static unsigned long
_cairo_gradient_pattern_color_stops_size (const cairo_pattern_t *pattern)
{
    cairo_gradient_pattern_t *gradient = (cairo_gradient_pattern_t *) pattern;

    return gradient->n_stops * (sizeof (double) + sizeof (cairo_color_t));
}

unsigned long
_cairo_pattern_size (const cairo_pattern_t *pattern)
{
    if (pattern->status)
	return 0;

    
    switch (pattern->type) {
    case CAIRO_PATTERN_TYPE_SOLID:
	return sizeof (cairo_solid_pattern_t);
	break;
    case CAIRO_PATTERN_TYPE_SURFACE:
	return sizeof (cairo_surface_pattern_t);
	break;
    case CAIRO_PATTERN_TYPE_LINEAR:
	return sizeof (cairo_linear_pattern_t) +
	    _cairo_gradient_pattern_color_stops_size (pattern);
	break;
    case CAIRO_PATTERN_TYPE_RADIAL:
	return sizeof (cairo_radial_pattern_t) +
	    _cairo_gradient_pattern_color_stops_size (pattern);
    default:
	ASSERT_NOT_REACHED;
	return 0;
    }
}


static cairo_bool_t
_cairo_solid_pattern_equal (const cairo_pattern_t *A,
			    const cairo_pattern_t *B)
{
    const cairo_solid_pattern_t *a = (cairo_solid_pattern_t *) A;
    const cairo_solid_pattern_t *b = (cairo_solid_pattern_t *) B;

    if (a->content != b->content)
	return FALSE;

    return _cairo_color_equal (&a->color, &b->color);
}

static cairo_bool_t
_cairo_gradient_color_stops_equal (const cairo_gradient_pattern_t *a,
				   const cairo_gradient_pattern_t *b)
{
    unsigned int n;

    if (a->n_stops != b->n_stops)
	return FALSE;

    for (n = 0; n < a->n_stops; n++) {
	if (a->stops[n].offset != b->stops[n].offset)
	    return FALSE;
	if (! _cairo_color_equal (&a->stops[n].color, &b->stops[n].color))
	    return FALSE;
    }

    return TRUE;
}

static cairo_bool_t
_cairo_linear_pattern_equal (const cairo_pattern_t *A,
			     const cairo_pattern_t *B)
{
    const cairo_linear_pattern_t *a = (cairo_linear_pattern_t *) A;
    const cairo_linear_pattern_t *b = (cairo_linear_pattern_t *) B;

    if (a->p1.x != b->p1.x)
	return FALSE;

    if (a->p1.y != b->p1.y)
	return FALSE;

    if (a->p2.x != b->p2.x)
	return FALSE;

    if (a->p2.y != b->p2.y)
	return FALSE;

    return _cairo_gradient_color_stops_equal (&a->base, &b->base);
}

static cairo_bool_t
_cairo_radial_pattern_equal (const cairo_pattern_t *A,
			     const cairo_pattern_t *B)
{
    const cairo_radial_pattern_t *a = (cairo_radial_pattern_t *) A;
    const cairo_radial_pattern_t *b = (cairo_radial_pattern_t *) B;

    if (a->c1.x != b->c1.x)
	return FALSE;

    if (a->c1.y != b->c1.y)
	return FALSE;

    if (a->r1 != b->r1)
	return FALSE;

    if (a->c2.x != b->c2.x)
	return FALSE;

    if (a->c2.y != b->c2.y)
	return FALSE;

    if (a->r2 != b->r2)
	return FALSE;

    return _cairo_gradient_color_stops_equal (&a->base, &b->base);
}

static cairo_bool_t
_cairo_surface_pattern_equal (const cairo_pattern_t *A,
			      const cairo_pattern_t *B)
{
    
    return FALSE;
}

cairo_bool_t
_cairo_pattern_equal (const cairo_pattern_t *a, const cairo_pattern_t *b)
{
    if (a->status || b->status)
	return FALSE;

    if (a->type != b->type)
	return FALSE;

    if (memcmp (&a->matrix, &b->matrix, sizeof (cairo_matrix_t)))
	return FALSE;

    if (a->filter != b->filter)
	return FALSE;

    if (a->extend != b->extend)
	return FALSE;

    switch (a->type) {
    case CAIRO_PATTERN_TYPE_SOLID:
	return _cairo_solid_pattern_equal (a, b);
    case CAIRO_PATTERN_TYPE_LINEAR:
	return _cairo_linear_pattern_equal (a, b);
    case CAIRO_PATTERN_TYPE_RADIAL:
	return _cairo_radial_pattern_equal (a, b);
    case CAIRO_PATTERN_TYPE_SURFACE:
	return _cairo_surface_pattern_equal (a, b);
    default:
	ASSERT_NOT_REACHED;
	return FALSE;
    }
}

















cairo_status_t
cairo_pattern_get_rgba (cairo_pattern_t *pattern,
			double *red, double *green,
			double *blue, double *alpha)
{
    cairo_solid_pattern_t *solid = (cairo_solid_pattern_t*) pattern;
    double r0, g0, b0, a0;

    if (pattern->status)
	return pattern->status;

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

    if (pattern->status)
	return pattern->status;

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

    if (pattern->status)
	return pattern->status;

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

    if (pattern->status)
	return pattern->status;

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

    if (pattern->status)
	return pattern->status;

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

    if (pattern->status)
	return pattern->status;

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
