


































#include "cairoint.h"

#include "cairo-surface-clipper-private.h"



static cairo_status_t
_cairo_surface_clipper_intersect_clip_path_recursive (cairo_surface_clipper_t *clipper,
						      cairo_clip_path_t *clip_path)
{
    cairo_status_t status;

    if (clip_path->prev != NULL) {
	status =
	    _cairo_surface_clipper_intersect_clip_path_recursive (clipper,
								  clip_path->prev);
	if (unlikely (status))
	    return status;
    }

    return clipper->intersect_clip_path (clipper,
					 &clip_path->path,
					 clip_path->fill_rule,
					 clip_path->tolerance,
					 clip_path->antialias);
}

cairo_status_t
_cairo_surface_clipper_set_clip (cairo_surface_clipper_t *clipper,
				 cairo_clip_t *clip)
{
    cairo_status_t status;
    cairo_bool_t clear;

    




    if (clip == NULL && clipper->clip.path == NULL)
	return CAIRO_STATUS_SUCCESS;

    if (clip != NULL && clip->path == clipper->clip.path)
	return CAIRO_STATUS_SUCCESS;

    if (clip != NULL && clipper->clip.path != NULL &&
	_cairo_path_fixed_equal (&clip->path->path, &clipper->clip.path->path))
    {
	return CAIRO_STATUS_SUCCESS;
    }

    
    assert (clip == NULL || clip->path != NULL);

    


    clear = clip == NULL || clip->path->prev != clipper->clip.path;

    _cairo_clip_reset (&clipper->clip);
    _cairo_clip_init_copy (&clipper->clip, clip);

    if (clear) {
	clipper->is_clipped = FALSE;
	status = clipper->intersect_clip_path (clipper, NULL, 0, 0, 0);
	if (unlikely (status))
	    return status;

	if (clip != NULL && clip->path != NULL) {
	    status =
		_cairo_surface_clipper_intersect_clip_path_recursive (clipper,
								      clip->path);
	    clipper->is_clipped = TRUE;
	}
    } else {
	cairo_clip_path_t *path = clip->path;

	clipper->is_clipped = TRUE;
	status = clipper->intersect_clip_path (clipper,
					       &path->path,
					       path->fill_rule,
					       path->tolerance,
					       path->antialias);
    }

    return status;
}

void
_cairo_surface_clipper_init (cairo_surface_clipper_t *clipper,
			     cairo_surface_clipper_intersect_clip_path_func_t func)
{
    _cairo_clip_init (&clipper->clip);
    clipper->is_clipped = FALSE;
    clipper->intersect_clip_path = func;
}

void
_cairo_surface_clipper_reset (cairo_surface_clipper_t *clipper)
{
    _cairo_clip_reset (&clipper->clip);
    clipper->is_clipped = FALSE;
}
