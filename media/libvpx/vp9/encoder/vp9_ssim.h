









#ifndef VP9_ENCODER_VP9_SSIM_H_
#define VP9_ENCODER_VP9_SSIM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vpx_scale/yv12config.h"

double vp9_calc_ssim(YV12_BUFFER_CONFIG *source, YV12_BUFFER_CONFIG *dest,
                     double *weight);

double vp9_calc_ssimg(YV12_BUFFER_CONFIG *source, YV12_BUFFER_CONFIG *dest,
                      double *ssim_y, double *ssim_u, double *ssim_v);

#ifdef __cplusplus
}  
#endif

#endif
