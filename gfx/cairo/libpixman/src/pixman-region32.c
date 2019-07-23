























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

#include "pixman-region.c"
