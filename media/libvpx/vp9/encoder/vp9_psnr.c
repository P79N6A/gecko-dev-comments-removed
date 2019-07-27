









#include <math.h>

#include "vpx_scale/yv12config.h"

#define MAX_PSNR 100

double vp9_mse2psnr(double samples, double peak, double mse) {
  double psnr;

  if (mse > 0.0)
    psnr = 10.0 * log10(peak * peak * samples / mse);
  else
    psnr = MAX_PSNR;  

  if (psnr > MAX_PSNR)
    psnr = MAX_PSNR;

  return psnr;
}
