























#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pixman-private.h"

#include <stdlib.h>

typedef pixman_box32_t		box_type_t;
typedef pixman_region32_data_t	region_data_type_t;
typedef pixman_region32_t	region_type_t;

typedef struct {
    int x, y;
} point_type_t;

#define PREFIX(x) pixman_region32##x

pixman_bool_t
pixman_region32_copy_from_region16 (pixman_region32_t *dst,
				    pixman_region16_t *src)
{
    int n_boxes, i;
    pixman_box16_t *boxes16;
    pixman_box32_t *boxes32;
    pixman_bool_t retval;
    
    boxes16 = pixman_region_rectangles (src, &n_boxes);

    boxes32 = pixman_malloc_ab (n_boxes, sizeof (pixman_box32_t));

    if (!boxes32)
	return FALSE;
    
    for (i = 0; i < n_boxes; ++i)
    {
	boxes32[i].x1 = boxes16[i].x1;
	boxes32[i].y1 = boxes16[i].y1;
	boxes32[i].x2 = boxes16[i].x2;
	boxes32[i].y2 = boxes16[i].y2;
    }

    pixman_region32_fini (dst);
    retval = pixman_region32_init_rects (dst, boxes32, n_boxes);
    free (boxes32);
    return retval;
}

#include "pixman-region.c"
