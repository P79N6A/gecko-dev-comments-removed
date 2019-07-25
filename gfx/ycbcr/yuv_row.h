








#ifndef MEDIA_BASE_YUV_ROW_H_
#define MEDIA_BASE_YUV_ROW_H_

#include "chromium_types.h"

extern "C" {


void FastConvertYUVToRGB32Row(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width);

void FastConvertYUVToRGB32Row_C(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* rgb_buf,
                                int width,
                                unsigned int x_shift);


}  


#if defined(ARCH_CPU_X86)
#if defined(_MSC_VER)
#define EMMS() __asm emms
#else
#define EMMS() asm("emms")
#endif
#else
#define EMMS()
#endif

#endif  
