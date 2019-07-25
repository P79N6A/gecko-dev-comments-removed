









#ifndef WEBRTC_COMMON_VIDEO_JPEG
#define WEBRTC_COMMON_VIDEO_JPEG

#include "typedefs.h"
#include "video_image.h"


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









    WebRtc_Word32 Encode(const RawImage& inputImage);

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
                         RawImage& outputImage);
 private:
    jpeg_decompress_struct*    _cinfo;
};


}
#endif 
