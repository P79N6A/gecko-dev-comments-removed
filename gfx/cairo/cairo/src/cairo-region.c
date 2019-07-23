


































#include "cairoint.h"








void
_cairo_region_extents_rectangle (pixman_region16_t       *region,
				 cairo_rectangle_int16_t *rect)
{
    pixman_box16_t *region_extents = pixman_region_extents (region);

    rect->x = region_extents->x1;
    rect->y = region_extents->y1;
    rect->width = region_extents->x2 - region_extents->x1;
    rect->height = region_extents->y2 - region_extents->y1;
}
