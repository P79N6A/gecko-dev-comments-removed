









#ifndef VPX_INTERNAL_VPX_PSNR_H_
#define VPX_INTERNAL_VPX_PSNR_H_

#ifdef __cplusplus
extern "C" {
#endif











double vpx_sse_to_psnr(double samples, double peak, double sse);

#ifdef __cplusplus
}  
#endif

#endif
