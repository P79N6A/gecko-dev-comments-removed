










#ifndef FILTER_H
#define FILTER_H

#include "vpx_ports/mem.h"

#define BLOCK_HEIGHT_WIDTH 4
#define VP8_FILTER_WEIGHT 128
#define VP8_FILTER_SHIFT  7

extern DECLARE_ALIGNED(16, const short, vp8_bilinear_filters[8][2]);
extern DECLARE_ALIGNED(16, const short, vp8_sub_pel_filters[8][6]);

#endif
