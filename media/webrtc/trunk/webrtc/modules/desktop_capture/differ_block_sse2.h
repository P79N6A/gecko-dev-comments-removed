












#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_DIFFER_BLOCK_SSE2_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_DIFFER_BLOCK_SSE2_H_

#include <stdint.h>

namespace webrtc {


extern int BlockDifference_SSE2_W16(const uint8_t* image1,
                                    const uint8_t* image2,
                                    int stride);


extern int BlockDifference_SSE2_W32(const uint8_t* image1,
                                    const uint8_t* image2,
                                    int stride);

}  

#endif  
