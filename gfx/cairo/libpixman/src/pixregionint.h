












































#ifndef _PIXREGIONINT_H_
#define _PIXREGIONINT_H_

#include "pixman.h"

struct pixman_region16_data {
    long		size;
    long		numRects;
    

};

typedef struct pixman_region16_point {
    int x, y;
} pixman_region16_point_t;

#define PIXREGION_NIL(reg) ((reg)->data && !(reg)->data->numRects)

#define PIXREGION_NAR(reg)	((reg)->data == &pixman_brokendata)
#define PIXREGION_NUM_RECTS(reg) ((reg)->data ? (reg)->data->numRects : 1)
#define PIXREGION_SIZE(reg) ((reg)->data ? (reg)->data->size : 0)
#define PIXREGION_RECTS(reg) ((reg)->data ? (pixman_box16_t *)((reg)->data + 1) \
			               : &(reg)->extents)
#define PIXREGION_BOXPTR(reg) ((pixman_box16_t *)((reg)->data + 1))
#define PIXREGION_BOX(reg,i) (&PIXREGION_BOXPTR(reg)[i])
#define PIXREGION_TOP(reg) PIXREGION_BOX(reg, (reg)->data->numRects)
#define PIXREGION_END(reg) PIXREGION_BOX(reg, (reg)->data->numRects - 1)
#define PIXREGION_SZOF(n) (sizeof(pixman_region16_data_t) + ((n) * sizeof(pixman_box16_t)))

#endif 
