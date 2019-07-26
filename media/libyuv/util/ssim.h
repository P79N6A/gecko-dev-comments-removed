











#ifndef UTIL_SSIM_H_  
#define UTIL_SSIM_H_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(INT_TYPES_DEFINED) && !defined(UINT8_TYPE_DEFINED)
typedef unsigned char uint8;
#define UINT8_TYPE_DEFINED
#endif

double CalcSSIM(const uint8* org, const uint8* rec,
                const int image_width, const int image_height);


double CalcLSSIM(double ssim);

#ifdef __cplusplus
}  
#endif

#endif
