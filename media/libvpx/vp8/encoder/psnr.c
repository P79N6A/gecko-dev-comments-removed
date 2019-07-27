










#include "vpx_scale/yv12config.h"
#include "math.h"
#include "vp8/common/systemdependent.h" 

#define MAX_PSNR 100

double vp8_mse2psnr(double Samples, double Peak, double Mse)
{
    double psnr;

    if ((double)Mse > 0.0)
        psnr = 10.0 * log10(Peak * Peak * Samples / Mse);
    else
        psnr = MAX_PSNR;      

    if (psnr > MAX_PSNR)
        psnr = MAX_PSNR;

    return psnr;
}
