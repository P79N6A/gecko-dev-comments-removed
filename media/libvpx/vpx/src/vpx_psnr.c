









#include <math.h>

#include "vpx/internal/vpx_psnr.h"

#define MAX_PSNR 100.0

double vpx_sse_to_psnr(double samples, double peak, double sse) {
  if (sse > 0.0) {
    const double psnr = 10.0 * log10(samples * peak * peak / sse);
    return psnr > MAX_PSNR ? MAX_PSNR : psnr;
  } else {
    return MAX_PSNR;
  }
}
