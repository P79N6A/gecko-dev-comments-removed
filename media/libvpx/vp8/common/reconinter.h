










#ifndef __INC_RECONINTER_H
#define __INC_RECONINTER_H

extern void vp8_build_inter_predictors_mb(MACROBLOCKD *x);
extern void vp8_build_inter16x16_predictors_mb(MACROBLOCKD *x,
                                               unsigned char *dst_y,
                                               unsigned char *dst_u,
                                               unsigned char *dst_v,
                                               int dst_ystride,
                                               int dst_uvstride);


extern void vp8_build_inter16x16_predictors_mby(MACROBLOCKD *x);
extern void vp8_build_uvmvs(MACROBLOCKD *x, int fullpixel);
extern void vp8_build_inter_predictors_b(BLOCKD *d, int pitch, vp8_subpix_fn_t sppf);
extern void vp8_build_inter_predictors_mbuv(MACROBLOCKD *x);

#endif
