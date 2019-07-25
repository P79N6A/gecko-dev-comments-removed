










#ifndef FILTER_H
#define FILTER_H

#define BLOCK_HEIGHT_WIDTH 4
#define VP8_FILTER_WEIGHT 128
#define VP8_FILTER_SHIFT  7

extern const short vp8_bilinear_filters[8][2];
extern const short vp8_sub_pel_filters[8][6];

#endif 
