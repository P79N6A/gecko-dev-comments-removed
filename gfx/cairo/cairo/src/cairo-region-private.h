



































#ifndef CAIRO_REGION_PRIVATE_H
#define CAIRO_REGION_PRIVATE_H

#include <pixman/pixman.h>



struct _cairo_region {
    pixman_region16_t rgn;
};

cairo_private void
_cairo_region_init (cairo_region_t *region);

cairo_private void
_cairo_region_init_rect (cairo_region_t *region,
			 cairo_rectangle_int_t *rect);

cairo_private cairo_int_status_t
_cairo_region_init_boxes (cairo_region_t *region,
			  cairo_box_int_t *boxes,
			  int count);

cairo_private void
_cairo_region_fini (cairo_region_t *region);

cairo_private cairo_int_status_t
_cairo_region_copy (cairo_region_t *dst,
		    cairo_region_t *src);

cairo_private int
_cairo_region_num_boxes (cairo_region_t *region);

cairo_private cairo_int_status_t
_cairo_region_get_boxes (cairo_region_t *region,
			 int *num_boxes,
			 cairo_box_int_t **boxes);

cairo_private void
_cairo_region_boxes_fini (cairo_region_t *region,
			  cairo_box_int_t *boxes);

cairo_private void
_cairo_region_get_extents (cairo_region_t *region,
			   cairo_rectangle_int_t *extents);

cairo_private cairo_int_status_t
_cairo_region_subtract (cairo_region_t *dst,
			cairo_region_t *a,
			cairo_region_t *b);

cairo_private cairo_int_status_t
_cairo_region_intersect (cairo_region_t *dst,
			 cairo_region_t *a,
			 cairo_region_t *b);

cairo_private cairo_int_status_t
_cairo_region_union_rect (cairo_region_t *dst,
			  cairo_region_t *src,
			  cairo_rectangle_int_t *rect);

cairo_private cairo_bool_t
_cairo_region_not_empty (cairo_region_t *region);

cairo_private void
_cairo_region_translate (cairo_region_t *region,
			 int x, int y);

#endif 
