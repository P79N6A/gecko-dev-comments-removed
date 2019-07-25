













#ifndef WEBRTC_COMMON_VIDEO_JPEG_DATA_MANAGER
#define WEBRTC_COMMON_VIDEO_JPEG_DATA_MANAGER

#include <stdio.h>
extern "C" {
#if defined(USE_SYSTEM_LIBJPEG)
#include <jpeglib.h>
#else
#include "jpeglib.h"
#endif
}

namespace webrtc
{





void
jpegSetSrcBuffer(j_decompress_ptr cinfo, JOCTET* srcBuffer, size_t bufferSize);





void
initSrc(j_decompress_ptr cinfo);






boolean
fillInputBuffer(j_decompress_ptr cinfo);




void
skipInputData(j_decompress_ptr cinfo, long num_bytes);





void
termSource (j_decompress_ptr cinfo);

} 


#endif 
