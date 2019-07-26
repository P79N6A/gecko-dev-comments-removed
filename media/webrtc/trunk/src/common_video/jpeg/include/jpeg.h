









#ifndef WEBRTC_COMMON_VIDEO_JPEG
#define WEBRTC_COMMON_VIDEO_JPEG

#include "typedefs.h"
#include "modules/interface/module_common_types.h"  
#include "common_video/interface/video_image.h"  


struct jpeg_compress_struct;
struct jpeg_decompress_struct;

namespace webrtc
{

class JpegEncoder
{
public:
    JpegEncoder();
    ~JpegEncoder();








    WebRtc_Word32 SetFileName(const char* fileName);









    WebRtc_Word32 Encode(const VideoFrame& inputImage);

private:

    jpeg_compress_struct*   _cinfo;
    char                    _fileName[257];
};

class JpegDecoder
{
 public:
    JpegDecoder();
    ~JpegDecoder();












    WebRtc_Word32 Decode(const EncodedImage& inputImage,
                         VideoFrame& outputImage);
 private:
    jpeg_decompress_struct*    _cinfo;
};


}
#endif 
