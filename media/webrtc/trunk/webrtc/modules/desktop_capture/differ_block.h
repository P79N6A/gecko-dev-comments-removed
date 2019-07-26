









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_DIFFER_BLOCK_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_DIFFER_BLOCK_H_

#include "webrtc/typedefs.h"

namespace webrtc {



const int kBlockSize = 32;


const int kBytesPerPixel = 4;



int BlockDifference(const uint8_t* image1, const uint8_t* image2, int stride);

}  

#endif  
