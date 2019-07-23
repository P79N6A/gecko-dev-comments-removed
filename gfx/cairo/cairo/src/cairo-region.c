





































#include "cairoint.h"

static const cairo_region_t _cairo_region_nil = {
    CAIRO_STATUS_NO_MEMORY,		
};





















static cairo_status_t
_cairo_region_set_error (cairo_region_t *region,
			cairo_status_t status)
{
    if (! _cairo_status_is_error (status))
	return status;

    

    _cairo_status_set_error (&region->status, status);

    return _cairo_error (status);
}

void
_cairo_region_init (cairo_region_t *region)
{
    VG (VALGRIND_MAKE_MEM_UNDEFINED (region, sizeof (cairo_region_t)));

    region->status = CAIRO_STATUS_SUCCESS;
    pixman_region32_init (&region->rgn);
}

void
_cairo_region_init_rectangle (cairo_region_t *region,
			      const cairo_rectangle_int_t *rectangle)
{
    VG (VALGRIND_MAKE_MEM_UNDEFINED (region, sizeof (cairo_region_t)));

    region->status = CAIRO_STATUS_SUCCESS;
    pixman_region32_init_rect (&region->rgn,
			       rectangle->x, rectangle->y,
			       rectangle->width, rectangle->height);
}

void
_cairo_region_fini (cairo_region_t *region)
{
    pixman_region32_fini (&region->rgn);
    VG (VALGRIND_MAKE_MEM_NOACCESS (region, sizeof (cairo_region_t)));
}














cairo_region_t *
cairo_region_create (void)
{
    cairo_region_t *region;

    region = _cairo_malloc (sizeof (cairo_region_t));
    if (region == NULL)
	return (cairo_region_t *) &_cairo_region_nil;

    region->status = CAIRO_STATUS_SUCCESS;

    pixman_region32_init (&region->rgn);

    return region;
}
slim_hidden_def (cairo_region_create);

cairo_region_t *
cairo_region_create_rectangles (cairo_rectangle_int_t *rects,
				int count)
{
    pixman_box32_t stack_pboxes[CAIRO_STACK_ARRAY_LENGTH (pixman_box32_t)];
    pixman_box32_t *pboxes = stack_pboxes;
    cairo_region_t *region;
    int i;

    region = _cairo_malloc (sizeof (cairo_region_t));

    if (!region)
	return (cairo_region_t *)&_cairo_region_nil;

    region->status = CAIRO_STATUS_SUCCESS;

    if (count > ARRAY_LENGTH (stack_pboxes)) {
	pboxes = _cairo_malloc_ab (count, sizeof (pixman_box32_t));

	if (unlikely (pboxes == NULL)) {
	    free (region);
	    return (cairo_region_t *)&_cairo_region_nil;
	}
    }

    for (i = 0; i < count; i++) {
	pboxes[i].x1 = rects[i].x;
	pboxes[i].y1 = rects[i].y;
	pboxes[i].x2 = rects[i].x + rects[i].width;
	pboxes[i].y2 = rects[i].y + rects[i].height;
    }

    if (! pixman_region32_init_rects (&region->rgn, pboxes, count)) {
	free (region);

	region = (cairo_region_t *)&_cairo_region_nil;
    }

    if (pboxes != stack_pboxes)
	free (pboxes);

    return region;
}
slim_hidden_def (cairo_region_create_rectangles);















cairo_region_t *
cairo_region_create_rectangle (const cairo_rectangle_int_t *rectangle)
{
    cairo_region_t *region;

    region = _cairo_malloc (sizeof (cairo_region_t));
    if (region == NULL)
	return (cairo_region_t *) &_cairo_region_nil;

    region->status = CAIRO_STATUS_SUCCESS;

    pixman_region32_init_rect (&region->rgn,
			       rectangle->x, rectangle->y,
			       rectangle->width, rectangle->height);

    return region;
}
slim_hidden_def (cairo_region_create_rectangle);















cairo_region_t *
cairo_region_copy (cairo_region_t *original)
{
    cairo_region_t *copy;

    if (original->status)
	return (cairo_region_t *) &_cairo_region_nil;

    copy = cairo_region_create ();
    if (copy->status)
	return copy;

    if (! pixman_region32_copy (&copy->rgn, &original->rgn)) {
	cairo_region_destroy (copy);
	return (cairo_region_t *) &_cairo_region_nil;
    }

    return copy;
}
slim_hidden_def (cairo_region_copy);











void
cairo_region_destroy (cairo_region_t *region)
{
    if (region == (cairo_region_t *) &_cairo_region_nil)
	return;

    pixman_region32_fini (&region->rgn);
    free (region);
}
slim_hidden_def (cairo_region_destroy);











int
cairo_region_num_rectangles (cairo_region_t *region)
{
    if (region->status)
	return 0;

    return pixman_region32_n_rects (&region->rgn);
}
slim_hidden_def (cairo_region_num_rectangles);











void
cairo_region_get_rectangle (cairo_region_t *region,
			    int nth,
			    cairo_rectangle_int_t *rectangle)
{
    pixman_box32_t *pbox;

    if (region->status) {
	rectangle->x = rectangle->y = 0;
	rectangle->width = rectangle->height = 0;
	return;
    }

    pbox = pixman_region32_rectangles (&region->rgn, NULL) + nth;

    rectangle->x = pbox->x1;
    rectangle->y = pbox->y1;
    rectangle->width = pbox->x2 - pbox->x1;
    rectangle->height = pbox->y2 - pbox->y1;
}
slim_hidden_def (cairo_region_get_rectangle);










void
cairo_region_get_extents (cairo_region_t *region,
			  cairo_rectangle_int_t *extents)
{
    pixman_box32_t *pextents;

    if (region->status) {
	extents->x = extents->y = 0;
	extents->width = extents->height = 0;
	return;
    }

    pextents = pixman_region32_extents (&region->rgn);

    extents->x = pextents->x1;
    extents->y = pextents->y1;
    extents->width = pextents->x2 - pextents->x1;
    extents->height = pextents->y2 - pextents->y1;
}
slim_hidden_def (cairo_region_get_extents);












cairo_status_t
cairo_region_status (cairo_region_t *region)
{
    return region->status;
}
slim_hidden_def (cairo_region_status);












cairo_status_t
cairo_region_subtract (cairo_region_t *dst, cairo_region_t *other)
{
    if (dst->status)
	return dst->status;

    if (other->status)
	return _cairo_region_set_error (dst, other->status);

    if (! pixman_region32_subtract (&dst->rgn, &dst->rgn, &other->rgn))
	return _cairo_region_set_error (dst, CAIRO_STATUS_NO_MEMORY);

    return CAIRO_STATUS_SUCCESS;
}
slim_hidden_def (cairo_region_subtract);












cairo_status_t
cairo_region_subtract_rectangle (cairo_region_t *dst,
				 const cairo_rectangle_int_t *rectangle)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    pixman_region32_t region;

    if (dst->status)
	return dst->status;

    pixman_region32_init_rect (&region,
			       rectangle->x, rectangle->y,
			       rectangle->width, rectangle->height);

    if (! pixman_region32_subtract (&dst->rgn, &dst->rgn, &region))
	status = _cairo_region_set_error (dst, CAIRO_STATUS_NO_MEMORY);

    pixman_region32_fini (&region);

    return status;
}
slim_hidden_def (cairo_region_subtract_rectangle);












cairo_status_t
cairo_region_intersect (cairo_region_t *dst, cairo_region_t *other)
{
    if (dst->status)
	return dst->status;

    if (other->status)
	return _cairo_region_set_error (dst, other->status);

    if (! pixman_region32_intersect (&dst->rgn, &dst->rgn, &other->rgn))
	return _cairo_region_set_error (dst, CAIRO_STATUS_NO_MEMORY);

    return CAIRO_STATUS_SUCCESS;
}
slim_hidden_def (cairo_region_intersect);













cairo_status_t
cairo_region_intersect_rectangle (cairo_region_t *dst,
				  const cairo_rectangle_int_t *rectangle)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    pixman_region32_t region;

    if (dst->status)
	return dst->status;

    pixman_region32_init_rect (&region,
			       rectangle->x, rectangle->y,
			       rectangle->width, rectangle->height);

    if (! pixman_region32_intersect (&dst->rgn, &dst->rgn, &region))
	status = _cairo_region_set_error (dst, CAIRO_STATUS_NO_MEMORY);

    pixman_region32_fini (&region);

    return status;
}
slim_hidden_def (cairo_region_intersect_rectangle);












cairo_status_t
cairo_region_union (cairo_region_t *dst,
		    cairo_region_t *other)
{
    if (dst->status)
	return dst->status;

    if (other->status)
	return _cairo_region_set_error (dst, other->status);

    if (! pixman_region32_union (&dst->rgn, &dst->rgn, &other->rgn))
	return _cairo_region_set_error (dst, CAIRO_STATUS_NO_MEMORY);

    return CAIRO_STATUS_SUCCESS;
}
slim_hidden_def (cairo_region_union);












cairo_status_t
cairo_region_union_rectangle (cairo_region_t *dst,
			      const cairo_rectangle_int_t *rectangle)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    pixman_region32_t region;

    if (dst->status)
	return dst->status;

    pixman_region32_init_rect (&region,
			       rectangle->x, rectangle->y,
			       rectangle->width, rectangle->height);

    if (! pixman_region32_union (&dst->rgn, &dst->rgn, &region))
	status = _cairo_region_set_error (dst, CAIRO_STATUS_NO_MEMORY);

    pixman_region32_fini (&region);

    return status;
}
slim_hidden_def (cairo_region_union_rectangle);











cairo_bool_t
cairo_region_is_empty (cairo_region_t *region)
{
    if (region->status)
	return TRUE;

    return ! pixman_region32_not_empty (&region->rgn);
}
slim_hidden_def (cairo_region_is_empty);











void
cairo_region_translate (cairo_region_t *region,
			int dx, int dy)
{
    if (region->status)
	return;

    pixman_region32_translate (&region->rgn, dx, dy);
}
slim_hidden_def (cairo_region_translate);
















cairo_region_overlap_t
cairo_region_contains_rectangle (cairo_region_t *region,
				 const cairo_rectangle_int_t *rectangle)
{
    pixman_box32_t pbox;
    pixman_region_overlap_t poverlap;

    if (region->status)
	return CAIRO_REGION_OVERLAP_OUT;

    pbox.x1 = rectangle->x;
    pbox.y1 = rectangle->y;
    pbox.x2 = rectangle->x + rectangle->width;
    pbox.y2 = rectangle->y + rectangle->height;

    poverlap = pixman_region32_contains_rectangle (&region->rgn, &pbox);
    switch (poverlap) {
    default:
    case PIXMAN_REGION_OUT:  return CAIRO_REGION_OVERLAP_OUT;
    case PIXMAN_REGION_IN:   return CAIRO_REGION_OVERLAP_IN;
    case PIXMAN_REGION_PART: return CAIRO_REGION_OVERLAP_PART;
    }
}
slim_hidden_def (cairo_region_contains_rectangle);













cairo_bool_t
cairo_region_contains_point (cairo_region_t *region,
			     int x, int y)
{
    pixman_box32_t box;
    
    if (region->status)
	return FALSE;

    return pixman_region32_contains_point (&region->rgn, x, y, &box);
}
slim_hidden_def (cairo_region_contains_point);
