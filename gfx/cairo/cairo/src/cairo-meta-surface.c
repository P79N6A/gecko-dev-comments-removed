






















































#include "cairoint.h"
#include "cairo-meta-surface-private.h"
#include "cairo-clip-private.h"

static const cairo_surface_backend_t cairo_meta_surface_backend;










cairo_surface_t *
_cairo_meta_surface_create (cairo_content_t	content,
			    int			width_pixels,
			    int			height_pixels)
{
    cairo_meta_surface_t *meta;

    meta = malloc (sizeof (cairo_meta_surface_t));
    if (meta == NULL) {
	_cairo_error (CAIRO_STATUS_NO_MEMORY);
	return (cairo_surface_t*) &_cairo_surface_nil;
    }

    _cairo_surface_init (&meta->base, &cairo_meta_surface_backend,
			 content);

    meta->content = content;
    meta->width_pixels = width_pixels;
    meta->height_pixels = height_pixels;

    _cairo_array_init (&meta->commands, sizeof (cairo_command_t *));
    meta->commands_owner = NULL;

    meta->is_clipped = FALSE;
    meta->replay_start_idx = 0;

    return &meta->base;
}

static cairo_surface_t *
_cairo_meta_surface_create_similar (void	       *abstract_surface,
				    cairo_content_t	content,
				    int			width,
				    int			height)
{
    return _cairo_meta_surface_create (content, width, height);
}

static cairo_status_t
_cairo_meta_surface_finish (void *abstract_surface)
{
    cairo_meta_surface_t *meta = abstract_surface;
    cairo_command_t *command;
    cairo_command_t **elements;
    int i, num_elements;

    if (meta->commands_owner) {
	cairo_surface_destroy (meta->commands_owner);
	return CAIRO_STATUS_SUCCESS;
    }

    num_elements = meta->commands.num_elements;
    elements = _cairo_array_index (&meta->commands, 0);
    for (i = 0; i < num_elements; i++) {
	command = elements[i];
	switch (command->type) {

	

	case CAIRO_COMMAND_PAINT:
	    _cairo_pattern_fini (&command->paint.source.base);
	    free (command);
	    break;

	case CAIRO_COMMAND_MASK:
	    _cairo_pattern_fini (&command->mask.source.base);
	    _cairo_pattern_fini (&command->mask.mask.base);
	    free (command);
	    break;

	case CAIRO_COMMAND_STROKE:
	    _cairo_pattern_fini (&command->stroke.source.base);
	    _cairo_path_fixed_fini (&command->stroke.path);
	    _cairo_stroke_style_fini (&command->stroke.style);
	    free (command);
	    break;

	case CAIRO_COMMAND_FILL:
	    _cairo_pattern_fini (&command->fill.source.base);
	    _cairo_path_fixed_fini (&command->fill.path);
	    free (command);
	    break;

	case CAIRO_COMMAND_SHOW_GLYPHS:
	    _cairo_pattern_fini (&command->show_glyphs.source.base);
	    free (command->show_glyphs.glyphs);
	    cairo_scaled_font_destroy (command->show_glyphs.scaled_font);
	    free (command);
	    break;

	
	case CAIRO_COMMAND_INTERSECT_CLIP_PATH:
	    if (command->intersect_clip_path.path_pointer)
		_cairo_path_fixed_fini (&command->intersect_clip_path.path);
	    free (command);
	    break;

	default:
	    ASSERT_NOT_REACHED;
	}
    }

    _cairo_array_fini (&meta->commands);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_meta_surface_acquire_source_image (void			 *abstract_surface,
					  cairo_image_surface_t	**image_out,
					  void			**image_extra)
{
    cairo_status_t status;
    cairo_meta_surface_t *surface = abstract_surface;
    cairo_surface_t *image;

    image = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					surface->width_pixels,
					surface->height_pixels);

    status = _cairo_meta_surface_replay (&surface->base, image);
    if (status) {
	cairo_surface_destroy (image);
	return status;
    }

    *image_out = (cairo_image_surface_t *) image;
    *image_extra = NULL;

    return status;
}

static void
_cairo_meta_surface_release_source_image (void			*abstract_surface,
					  cairo_image_surface_t	*image,
					  void			*image_extra)
{
    cairo_surface_destroy (&image->base);
}

static cairo_status_t
_init_pattern_with_snapshot (cairo_pattern_t       *pattern,
			     const cairo_pattern_t *other)
{
    cairo_status_t status;

    status = _cairo_pattern_init_copy (pattern, other);
    if (status)
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

static cairo_int_status_t
_cairo_meta_surface_paint (void			*abstract_surface,
			   cairo_operator_t	 op,
			   cairo_pattern_t	*source)
{
    cairo_status_t status;
    cairo_meta_surface_t *meta = abstract_surface;
    cairo_command_paint_t *command;

    


    if (op == CAIRO_OPERATOR_CLEAR && !meta->is_clipped)
	meta->replay_start_idx = meta->commands.num_elements;

    command = malloc (sizeof (cairo_command_paint_t));
    if (command == NULL)
	return CAIRO_STATUS_NO_MEMORY;

    command->type = CAIRO_COMMAND_PAINT;
    command->op = op;

    status = _init_pattern_with_snapshot (&command->source.base, source);
    if (status)
	goto CLEANUP_COMMAND;

    status = _cairo_array_append (&meta->commands, &command);
    if (status)
	goto CLEANUP_SOURCE;

    return CAIRO_STATUS_SUCCESS;

  CLEANUP_SOURCE:
    _cairo_pattern_fini (&command->source.base);
  CLEANUP_COMMAND:
    free (command);
    return status;
}

static cairo_int_status_t
_cairo_meta_surface_mask (void			*abstract_surface,
			  cairo_operator_t	 op,
			  cairo_pattern_t	*source,
			  cairo_pattern_t	*mask)
{
    cairo_status_t status;
    cairo_meta_surface_t *meta = abstract_surface;
    cairo_command_mask_t *command;

    command = malloc (sizeof (cairo_command_mask_t));
    if (command == NULL)
	return CAIRO_STATUS_NO_MEMORY;

    command->type = CAIRO_COMMAND_MASK;
    command->op = op;

    status = _init_pattern_with_snapshot (&command->source.base, source);
    if (status)
	goto CLEANUP_COMMAND;

    status = _init_pattern_with_snapshot (&command->mask.base, mask);
    if (status)
	goto CLEANUP_SOURCE;

    status = _cairo_array_append (&meta->commands, &command);
    if (status)
	goto CLEANUP_MASK;

    return CAIRO_STATUS_SUCCESS;

  CLEANUP_MASK:
    _cairo_pattern_fini (&command->mask.base);
  CLEANUP_SOURCE:
    _cairo_pattern_fini (&command->source.base);
  CLEANUP_COMMAND:
    free (command);
    return status;
}

static cairo_int_status_t
_cairo_meta_surface_stroke (void			*abstract_surface,
			    cairo_operator_t		 op,
			    cairo_pattern_t		*source,
			    cairo_path_fixed_t		*path,
			    cairo_stroke_style_t	*style,
			    cairo_matrix_t		*ctm,
			    cairo_matrix_t		*ctm_inverse,
			    double			 tolerance,
			    cairo_antialias_t		 antialias)
{
    cairo_status_t status;
    cairo_meta_surface_t *meta = abstract_surface;
    cairo_command_stroke_t *command;

    command = malloc (sizeof (cairo_command_stroke_t));
    if (command == NULL)
	return CAIRO_STATUS_NO_MEMORY;

    command->type = CAIRO_COMMAND_STROKE;
    command->op = op;

    status = _init_pattern_with_snapshot (&command->source.base, source);
    if (status)
	goto CLEANUP_COMMAND;

    status = _cairo_path_fixed_init_copy (&command->path, path);
    if (status)
	goto CLEANUP_SOURCE;

    status = _cairo_stroke_style_init_copy (&command->style, style);
    if (status)
	goto CLEANUP_PATH;

    command->ctm = *ctm;
    command->ctm_inverse = *ctm_inverse;
    command->tolerance = tolerance;
    command->antialias = antialias;

    status = _cairo_array_append (&meta->commands, &command);
    if (status)
	goto CLEANUP_STYLE;

    return CAIRO_STATUS_SUCCESS;

  CLEANUP_STYLE:
    _cairo_stroke_style_fini (&command->style);
  CLEANUP_PATH:
    _cairo_path_fixed_fini (&command->path);
  CLEANUP_SOURCE:
    _cairo_pattern_fini (&command->source.base);
  CLEANUP_COMMAND:
    free (command);
    return status;
}

static cairo_int_status_t
_cairo_meta_surface_fill (void			*abstract_surface,
			  cairo_operator_t	 op,
			  cairo_pattern_t	*source,
			  cairo_path_fixed_t	*path,
			  cairo_fill_rule_t	 fill_rule,
			  double		 tolerance,
			  cairo_antialias_t	 antialias)
{
    cairo_status_t status;
    cairo_meta_surface_t *meta = abstract_surface;
    cairo_command_fill_t *command;

    command = malloc (sizeof (cairo_command_fill_t));
    if (command == NULL)
	return CAIRO_STATUS_NO_MEMORY;

    command->type = CAIRO_COMMAND_FILL;
    command->op = op;

    status = _init_pattern_with_snapshot (&command->source.base, source);
    if (status)
	goto CLEANUP_COMMAND;

    status = _cairo_path_fixed_init_copy (&command->path, path);
    if (status)
	goto CLEANUP_SOURCE;

    command->fill_rule = fill_rule;
    command->tolerance = tolerance;
    command->antialias = antialias;

    status = _cairo_array_append (&meta->commands, &command);
    if (status)
	goto CLEANUP_PATH;

    return CAIRO_STATUS_SUCCESS;

  CLEANUP_PATH:
    _cairo_path_fixed_fini (&command->path);
  CLEANUP_SOURCE:
    _cairo_pattern_fini (&command->source.base);
  CLEANUP_COMMAND:
    free (command);
    return status;
}

static cairo_int_status_t
_cairo_meta_surface_show_glyphs (void			*abstract_surface,
				 cairo_operator_t	 op,
				 cairo_pattern_t	*source,
				 cairo_glyph_t		*glyphs,
				 int			 num_glyphs,
				 cairo_scaled_font_t	*scaled_font)
{
    cairo_status_t status;
    cairo_meta_surface_t *meta = abstract_surface;
    cairo_command_show_glyphs_t *command;

    command = malloc (sizeof (cairo_command_show_glyphs_t));
    if (command == NULL)
	return CAIRO_STATUS_NO_MEMORY;

    command->type = CAIRO_COMMAND_SHOW_GLYPHS;
    command->op = op;

    status = _init_pattern_with_snapshot (&command->source.base, source);
    if (status)
	goto CLEANUP_COMMAND;

    command->glyphs = _cairo_malloc_ab (num_glyphs, sizeof (cairo_glyph_t));
    if (command->glyphs == NULL) {
	status = CAIRO_STATUS_NO_MEMORY;
	goto CLEANUP_SOURCE;
    }
    memcpy (command->glyphs, glyphs, sizeof (cairo_glyph_t) * num_glyphs);

    command->num_glyphs = num_glyphs;

    command->scaled_font = cairo_scaled_font_reference (scaled_font);

    status = _cairo_array_append (&meta->commands, &command);
    if (status)
	goto CLEANUP_SCALED_FONT;

    return CAIRO_STATUS_SUCCESS;

  CLEANUP_SCALED_FONT:
    cairo_scaled_font_destroy (command->scaled_font);
    free (command->glyphs);
  CLEANUP_SOURCE:
    _cairo_pattern_fini (&command->source.base);
  CLEANUP_COMMAND:
    free (command);
    return status;
}














static cairo_surface_t *
_cairo_meta_surface_snapshot (void *abstract_other)
{
    cairo_meta_surface_t *other = abstract_other;
    cairo_meta_surface_t *meta;

    meta = malloc (sizeof (cairo_meta_surface_t));
    if (meta == NULL) {
	_cairo_error (CAIRO_STATUS_NO_MEMORY);
	return (cairo_surface_t*) &_cairo_surface_nil;
    }

    _cairo_surface_init (&meta->base, &cairo_meta_surface_backend,
			 other->base.content);
    meta->base.is_snapshot = TRUE;

    meta->width_pixels = other->width_pixels;
    meta->height_pixels = other->height_pixels;
    meta->replay_start_idx = other->replay_start_idx;
    meta->content = other->content;

    _cairo_array_init_snapshot (&meta->commands, &other->commands);
    meta->commands_owner = cairo_surface_reference (&other->base);

    return &meta->base;
}

static cairo_int_status_t
_cairo_meta_surface_intersect_clip_path (void		    *dst,
					 cairo_path_fixed_t *path,
					 cairo_fill_rule_t   fill_rule,
					 double		     tolerance,
					 cairo_antialias_t   antialias)
{
    cairo_meta_surface_t *meta = dst;
    cairo_command_intersect_clip_path_t *command;
    cairo_status_t status;

    command = malloc (sizeof (cairo_command_intersect_clip_path_t));
    if (command == NULL)
	return CAIRO_STATUS_NO_MEMORY;

    command->type = CAIRO_COMMAND_INTERSECT_CLIP_PATH;

    if (path) {
	status = _cairo_path_fixed_init_copy (&command->path, path);
	if (status) {
	    free (command);
	    return status;
	}
	command->path_pointer = &command->path;
	meta->is_clipped = TRUE;
    } else {
	command->path_pointer = NULL;
	meta->is_clipped = FALSE;
    }
    command->fill_rule = fill_rule;
    command->tolerance = tolerance;
    command->antialias = antialias;

    status = _cairo_array_append (&meta->commands, &command);
    if (status) {
	if (path)
	    _cairo_path_fixed_fini (&command->path);
	free (command);
	return status;
    }

    return CAIRO_STATUS_SUCCESS;
}





static cairo_int_status_t
_cairo_meta_surface_get_extents (void			 *abstract_surface,
				 cairo_rectangle_int_t   *rectangle)
{
    cairo_meta_surface_t *surface = abstract_surface;

    rectangle->x = 0;
    rectangle->y = 0;
    rectangle->width = surface->width_pixels;
    rectangle->height = surface->height_pixels;

    return CAIRO_STATUS_SUCCESS;
}









cairo_bool_t
_cairo_surface_is_meta (const cairo_surface_t *surface)
{
    return surface->backend == &cairo_meta_surface_backend;
}

static const cairo_surface_backend_t cairo_meta_surface_backend = {
    CAIRO_INTERNAL_SURFACE_TYPE_META,
    _cairo_meta_surface_create_similar,
    _cairo_meta_surface_finish,
    _cairo_meta_surface_acquire_source_image,
    _cairo_meta_surface_release_source_image,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    _cairo_meta_surface_intersect_clip_path,
    _cairo_meta_surface_get_extents,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 

    



    _cairo_meta_surface_paint,
    _cairo_meta_surface_mask,
    _cairo_meta_surface_stroke,
    _cairo_meta_surface_fill,
    _cairo_meta_surface_show_glyphs,

    _cairo_meta_surface_snapshot
};

static cairo_path_fixed_t *
_cairo_command_get_path (cairo_command_t *command)
{
    switch (command->type) {
    case CAIRO_COMMAND_PAINT:
    case CAIRO_COMMAND_MASK:
    case CAIRO_COMMAND_SHOW_GLYPHS:
	return NULL;
    case CAIRO_COMMAND_STROKE:
	return &command->stroke.path;
    case CAIRO_COMMAND_FILL:
	return &command->fill.path;
    case CAIRO_COMMAND_INTERSECT_CLIP_PATH:
	return command->intersect_clip_path.path_pointer;
    }

    ASSERT_NOT_REACHED;
    return NULL;
}

cairo_status_t
_cairo_meta_surface_replay (cairo_surface_t *surface,
			    cairo_surface_t *target)
{
    cairo_meta_surface_t *meta;
    cairo_command_t *command, **elements;
    int i, num_elements;
    cairo_int_status_t status;
    cairo_clip_t clip;
    cairo_bool_t has_device_transform = _cairo_surface_has_device_transform (target);
    cairo_matrix_t *device_transform = &target->device_transform;
    cairo_path_fixed_t path_copy, *dev_path;

    if (surface->status)
	return surface->status;

    meta = (cairo_meta_surface_t *) surface;
    status = CAIRO_STATUS_SUCCESS;

    _cairo_clip_init (&clip, target);

    num_elements = meta->commands.num_elements;
    elements = _cairo_array_index (&meta->commands, 0);
    for (i = meta->replay_start_idx; i < num_elements; i++) {
	command = elements[i];

	

	if (command->type != CAIRO_COMMAND_INTERSECT_CLIP_PATH) {
	    status = _cairo_surface_set_clip (target, &clip);
	    if (status)
		break;
	}

	dev_path = _cairo_command_get_path (command);
	if (dev_path && has_device_transform) {
	    status = _cairo_path_fixed_init_copy (&path_copy, dev_path);
	    if (status)
		break;
	    _cairo_path_fixed_device_transform (&path_copy, device_transform);
	    dev_path = &path_copy;
	}

	switch (command->type) {
	case CAIRO_COMMAND_PAINT:
	    status = _cairo_surface_paint (target,
					   command->paint.op,
					   &command->paint.source.base);
	    break;
	case CAIRO_COMMAND_MASK:
	    status = _cairo_surface_mask (target,
					  command->mask.op,
					  &command->mask.source.base,
					  &command->mask.mask.base);
	    break;
	case CAIRO_COMMAND_STROKE:
	{
	    cairo_matrix_t dev_ctm = command->stroke.ctm;
	    cairo_matrix_t dev_ctm_inverse = command->stroke.ctm_inverse;
	    cairo_matrix_t tmp;

	    if (has_device_transform) {
		cairo_matrix_multiply (&dev_ctm, &dev_ctm, device_transform);
		tmp = surface->device_transform;
		status = cairo_matrix_invert (&tmp);
		assert (status == CAIRO_STATUS_SUCCESS);
		cairo_matrix_multiply (&dev_ctm_inverse, &tmp, &dev_ctm_inverse);
	    }

	    status = _cairo_surface_stroke (target,
					    command->stroke.op,
					    &command->stroke.source.base,
					    dev_path,
					    &command->stroke.style,
					    &dev_ctm,
					    &dev_ctm_inverse,
					    command->stroke.tolerance,
					    command->stroke.antialias);
	    break;
	}
	case CAIRO_COMMAND_FILL:
	    status = _cairo_surface_fill (target,
					  command->fill.op,
					  &command->fill.source.base,
					  dev_path,
					  command->fill.fill_rule,
					  command->fill.tolerance,
					  command->fill.antialias);
	    break;
	case CAIRO_COMMAND_SHOW_GLYPHS:
	{
	    cairo_glyph_t *glyphs = command->show_glyphs.glyphs;
	    cairo_glyph_t *dev_glyphs = glyphs;
	    int i, num_glyphs = command->show_glyphs.num_glyphs;

	    if (has_device_transform) {
		dev_glyphs = _cairo_malloc_ab (num_glyphs, sizeof (cairo_glyph_t));
		if (dev_glyphs == NULL) {
		    status = CAIRO_STATUS_NO_MEMORY;
		    break;
		}
		for (i = 0; i < num_glyphs; i++) {
		    dev_glyphs[i] = glyphs[i];
		    cairo_matrix_transform_point (device_transform,
						  &dev_glyphs[i].x,
						  &dev_glyphs[i].y);
		}
	    }

	    status = _cairo_surface_show_glyphs	(target,
						 command->show_glyphs.op,
						 &command->show_glyphs.source.base,
						 dev_glyphs, num_glyphs,
						 command->show_glyphs.scaled_font);

	    if (dev_glyphs != glyphs)
		free (dev_glyphs);

	    break;
	}
	case CAIRO_COMMAND_INTERSECT_CLIP_PATH:
	    

	    if (dev_path == NULL)
		_cairo_clip_reset (&clip);
	    else
		status = _cairo_clip_clip (&clip, dev_path,
					   command->intersect_clip_path.fill_rule,
					   command->intersect_clip_path.tolerance,
					   command->intersect_clip_path.antialias,
					   target);
	    break;
	default:
	    ASSERT_NOT_REACHED;
	}

	if (dev_path == &path_copy)
	    _cairo_path_fixed_fini (&path_copy);

	if (status)
	    break;
    }

    _cairo_clip_reset (&clip);

    return status;
}
