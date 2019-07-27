









#ifndef VP9_ENCODER_VP9_SSIM_H_
#define VP9_ENCODER_VP9_SSIM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vpx_scale/yv12config.h"


typedef struct {
  
  uint64_t sum_s;

  
  uint64_t sum_r;

  
  uint64_t sum_sq_s;

  
  uint64_t sum_sq_r;

  
  uint64_t sum_sxr;

  
  double ssim;
} Ssimv;


typedef struct {
  
  double ssimc;

  
  double ssim;

  
  double ssim2;

  
  double dssim;

  
  double dssimd;

  
  double ssimcd;
} Metrics;

double vp9_get_ssim_metrics(uint8_t *img1, int img1_pitch, uint8_t *img2,
                      int img2_pitch, int width, int height, Ssimv *sv2,
                      Metrics *m, int do_inconsistency);

double vp9_calc_ssim(YV12_BUFFER_CONFIG *source, YV12_BUFFER_CONFIG *dest,
                     double *weight);

double vp9_calc_ssimg(YV12_BUFFER_CONFIG *source, YV12_BUFFER_CONFIG *dest,
                      double *ssim_y, double *ssim_u, double *ssim_v);

double vp9_calc_fastssim(YV12_BUFFER_CONFIG *source, YV12_BUFFER_CONFIG *dest,
                         double *ssim_y, double *ssim_u, double *ssim_v);

double vp9_psnrhvs(YV12_BUFFER_CONFIG *source, YV12_BUFFER_CONFIG *dest,
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
