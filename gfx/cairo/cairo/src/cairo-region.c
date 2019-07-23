




































#include "cairoint.h"

void
_cairo_region_init (cairo_region_t *region)
{
    pixman_region32_init (&region->rgn);
}

void
_cairo_region_init_rect (cairo_region_t *region,
			 cairo_rectangle_int_t *rect)
{
    pixman_region32_init_rect (&region->rgn,
			     rect->x, rect->y,
			     rect->width, rect->height);
}

cairo_int_status_t
_cairo_region_init_boxes (cairo_region_t *region,
			  cairo_box_int_t *boxes,
			  int count)
{
    pixman_box32_t stack_pboxes[CAIRO_STACK_ARRAY_LENGTH (pixman_box32_t)];
    pixman_box32_t *pboxes = stack_pboxes;
    cairo_int_status_t status = CAIRO_STATUS_SUCCESS;
    int i;

    if (count > ARRAY_LENGTH (stack_pboxes)) {
	pboxes = _cairo_malloc_ab (count, sizeof (pixman_box32_t));
	if (unlikely (pboxes == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    for (i = 0; i < count; i++) {
	pboxes[i].x1 = boxes[i].p1.x;
	pboxes[i].y1 = boxes[i].p1.y;
	pboxes[i].x2 = boxes[i].p2.x;
	pboxes[i].y2 = boxes[i].p2.y;
    }

    if (! pixman_region32_init_rects (&region->rgn, pboxes, count))
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);

    if (pboxes != stack_pboxes)
	free (pboxes);

    return status;
}

void
_cairo_region_fini (cairo_region_t *region)
{
    pixman_region32_fini (&region->rgn);
}

cairo_int_status_t
_cairo_region_copy (cairo_region_t *dst, cairo_region_t *src)
{
    if (!pixman_region32_copy (&dst->rgn, &src->rgn))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    return CAIRO_STATUS_SUCCESS;
}

int
_cairo_region_num_boxes (cairo_region_t *region)
{
    return pixman_region32_n_rects (&region->rgn);
}

cairo_private void
_cairo_region_get_box (cairo_region_t *region,
		       int nth_box,
		       cairo_box_int_t *box)
{
    pixman_box32_t *pbox;

    pbox = pixman_region32_rectangles (&region->rgn, NULL) + nth_box;

    box->p1.x = pbox->x1;
    box->p1.y = pbox->y1;
    box->p2.x = pbox->x2;
    box->p2.y = pbox->y2;
}








void
_cairo_region_get_extents (cairo_region_t *region, cairo_rectangle_int_t *extents)
{
    pixman_box32_t *pextents = pixman_region32_extents (&region->rgn);

    extents->x = pextents->x1;
    extents->y = pextents->y1;
    extents->width = pextents->x2 - pextents->x1;
    extents->height = pextents->y2 - pextents->y1;
}

cairo_int_status_t
_cairo_region_subtract (cairo_region_t *dst, cairo_region_t *a, cairo_region_t *b)
{
    if (!pixman_region32_subtract (&dst->rgn, &a->rgn, &b->rgn))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    return CAIRO_STATUS_SUCCESS;
}

cairo_int_status_t
_cairo_region_intersect (cairo_region_t *dst, cairo_region_t *a, cairo_region_t *b)
{
    if (!pixman_region32_intersect (&dst->rgn, &a->rgn, &b->rgn))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    return CAIRO_STATUS_SUCCESS;
}

cairo_int_status_t
_cairo_region_union_rect (cairo_region_t *dst,
			  cairo_region_t *src,
			  cairo_rectangle_int_t *rect)
{
    if (!pixman_region32_union_rect (&dst->rgn, &src->rgn,
				   rect->x, rect->y,
				   rect->width, rect->height))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    return CAIRO_STATUS_SUCCESS;
}

cairo_bool_t
_cairo_region_not_empty (cairo_region_t *region)
{
    return (cairo_bool_t) pixman_region32_not_empty (&region->rgn);
}

void
_cairo_region_translate (cairo_region_t *region,
			 int x, int y)
{
    pixman_region32_translate (&region->rgn, x, y);
}

pixman_region_overlap_t
_cairo_region_contains_rectangle (cairo_region_t *region,
				  const cairo_rectangle_int_t *rect)
{
    pixman_box32_t pbox;

    pbox.x1 = rect->x;
    pbox.y1 = rect->y;
    pbox.x2 = rect->x + rect->width;
    pbox.y2 = rect->y + rect->height;

    return pixman_region32_contains_rectangle (&region->rgn, &pbox);
}
