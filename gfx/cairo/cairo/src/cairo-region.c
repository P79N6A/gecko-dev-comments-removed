




































#include "cairoint.h"

void
_cairo_region_init (cairo_region_t *region)
{
    pixman_region_init (&region->rgn);
}

void
_cairo_region_init_rect (cairo_region_t *region,
			 cairo_rectangle_int_t *rect)
{
    pixman_region_init_rect (&region->rgn,
			     rect->x, rect->y,
			     rect->width, rect->height);
}

#define STACK_BOXES_LEN ((int) (CAIRO_STACK_BUFFER_SIZE / sizeof(pixman_box16_t)))

cairo_int_status_t
_cairo_region_init_boxes (cairo_region_t *region,
			  cairo_box_int_t *boxes,
			  int count)
{
    pixman_box16_t stack_pboxes[STACK_BOXES_LEN];
    pixman_box16_t *pboxes = stack_pboxes;
    cairo_int_status_t status = CAIRO_STATUS_SUCCESS;
    int i;

    if (count > ARRAY_LENGTH(stack_pboxes)) {
	pboxes = _cairo_malloc_ab (count, sizeof(pixman_box16_t));
	if (pboxes == NULL)
	    return CAIRO_STATUS_NO_MEMORY;
    }

    for (i = 0; i < count; i++) {
	pboxes[i].x1 = boxes[i].p1.x;
	pboxes[i].y1 = boxes[i].p1.y;
	pboxes[i].x2 = boxes[i].p2.x;
	pboxes[i].y2 = boxes[i].p2.y;
    }

    if (!pixman_region_init_rects (&region->rgn, pboxes, count))
	status = CAIRO_STATUS_NO_MEMORY;

    if (pboxes != stack_pboxes)
	free (pboxes);

    return status;
}

void
_cairo_region_fini (cairo_region_t *region)
{
    pixman_region_fini (&region->rgn);
}

cairo_int_status_t
_cairo_region_copy (cairo_region_t *dst, cairo_region_t *src)
{
    if (!pixman_region_copy (&dst->rgn, &src->rgn))
	return CAIRO_STATUS_NO_MEMORY;

    return CAIRO_STATUS_SUCCESS;
}

int
_cairo_region_num_boxes (cairo_region_t *region)
{
    return pixman_region_n_rects (&region->rgn);
}

cairo_int_status_t
_cairo_region_get_boxes (cairo_region_t *region, int *num_boxes, cairo_box_int_t **boxes)
{
    int nboxes;
    pixman_box16_t *pboxes;
    cairo_box_int_t *cboxes;
    int i;

    pboxes = pixman_region_rectangles (&region->rgn, &nboxes);

    if (nboxes == 0) {
	*num_boxes = 0;
	*boxes = NULL;
	return CAIRO_STATUS_SUCCESS;
    }

    cboxes = _cairo_malloc_ab (nboxes, sizeof(cairo_box_int_t));
    if (cboxes == NULL)
	return CAIRO_STATUS_NO_MEMORY;

    for (i = 0; i < nboxes; i++) {
	cboxes[i].p1.x = pboxes[i].x1;
	cboxes[i].p1.y = pboxes[i].y1;
	cboxes[i].p2.x = pboxes[i].x2;
	cboxes[i].p2.y = pboxes[i].y2;
    }

    *num_boxes = nboxes;
    *boxes = cboxes;

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_region_boxes_fini (cairo_region_t *region, cairo_box_int_t *boxes)
{
    free (boxes);
}








void
_cairo_region_get_extents (cairo_region_t *region, cairo_rectangle_int_t *extents)
{
    pixman_box16_t *pextents = pixman_region_extents (&region->rgn);

    extents->x = pextents->x1;
    extents->y = pextents->y1;
    extents->width = pextents->x2 - pextents->x1;
    extents->height = pextents->y2 - pextents->y1;
}

cairo_int_status_t
_cairo_region_subtract (cairo_region_t *dst, cairo_region_t *a, cairo_region_t *b)
{
    if (!pixman_region_subtract (&dst->rgn, &a->rgn, &b->rgn))
	return CAIRO_STATUS_NO_MEMORY;

    return CAIRO_STATUS_SUCCESS;
}

cairo_int_status_t
_cairo_region_intersect (cairo_region_t *dst, cairo_region_t *a, cairo_region_t *b)
{
    if (!pixman_region_intersect (&dst->rgn, &a->rgn, &b->rgn))
	return CAIRO_STATUS_NO_MEMORY;

    return CAIRO_STATUS_SUCCESS;
}

cairo_int_status_t
_cairo_region_union_rect (cairo_region_t *dst,
			  cairo_region_t *src,
			  cairo_rectangle_int_t *rect)
{
    if (!pixman_region_union_rect (&dst->rgn, &src->rgn,
				   rect->x, rect->y,
				   rect->width, rect->height))
	return CAIRO_STATUS_NO_MEMORY;

    return CAIRO_STATUS_SUCCESS;
}

cairo_bool_t
_cairo_region_not_empty (cairo_region_t *region)
{
    return (cairo_bool_t) pixman_region_not_empty (&region->rgn);
}

void
_cairo_region_translate (cairo_region_t *region,
			 int x, int y)
{
    pixman_region_translate (&region->rgn, x, y);
}
