









#ifndef WEBRTC_COMMON_VIDEO_JPEG
#define WEBRTC_COMMON_VIDEO_JPEG

#include "typedefs.h"
#include "common_video/interface/i420_video_frame.h"
#include "common_video/interface/video_image.h"  


struct jpeg_compress_struct;

namespace webrtc
{



class JpegEncoder
{
public:
    JpegEncoder();
    ~JpegEncoder();








    WebRtc_Word32 SetFileName(const char* fileName);









    WebRtc_Word32 Encode(const I420VideoFrame& inputImage);

private:

    jpeg_compress_struct*   _cinfo;
    char                    _fileName[257];
};













int ConvertJpegToI420(const EncodedImage& input_image,
                      I420VideoFrame* output_image);
}
#endif 
