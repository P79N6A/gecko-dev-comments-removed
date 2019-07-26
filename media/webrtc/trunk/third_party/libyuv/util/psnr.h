











#ifndef UTIL_PSNR_H_  
#define UTIL_PSNR_H_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(INT_TYPES_DEFINED) && !defined(UINT8_TYPE_DEFINED)
typedef unsigned char uint8;
#define UINT8_TYPE_DEFINED
#endif

static const double kMaxPSNR = 128.0;



double ComputePSNR(double sse, double size);



double ComputeSumSquareError(const uint8* org, const uint8* rec, int size);

#ifdef __cplusplus
}  
#endif

#endif
