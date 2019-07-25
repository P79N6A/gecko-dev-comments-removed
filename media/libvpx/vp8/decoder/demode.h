










#include "onyxd_int.h"



void vp8_kfread_modes(VP8D_COMP *pbi);



int vp8_read_bmode(vp8_reader *, const vp8_prob *);



int   vp8_read_ymode(vp8_reader *, const vp8_prob *);
int vp8_kfread_ymode(vp8_reader *, const vp8_prob *);



int vp8_read_uv_mode(vp8_reader *, const vp8_prob *);



void vp8_read_mb_features(vp8_reader *, MB_MODE_INFO *, MACROBLOCKD *);
