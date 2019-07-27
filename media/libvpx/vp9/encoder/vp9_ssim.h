









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

#if CONFIG_VP9_HIGHBITDEPTH
double vp9_highbd_calc_ssim(YV12_BUFFER_CONFIG *source,
                            YV12_BUFFER_CONFIG *dest,
                            double *weight,
                            unsigned int bd);

double vp9_highbd_calc_ssimg(YV12_BUFFER_CONFIG *source,
                             YV12_BUFFER_CONFIG *dest,
                             double *ssim_y,
                             double *ssim_u,
                             double *ssim_v,
                             unsigned int bd);
#endif  

#ifdef __cplusplus
}  
#endif

#endif
