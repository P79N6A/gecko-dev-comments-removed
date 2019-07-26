









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_VIDEO_CAPTURE_DELAY_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_VIDEO_CAPTURE_DELAY_H_

namespace webrtc
{
namespace videocapturemodule
{

struct DelayValue
{
    WebRtc_Word32 width;
    WebRtc_Word32 height;
    WebRtc_Word32 delay;
};

enum { NoOfDelayValues = 40 };
struct DelayValues
{
    char * deviceName;
    char* productId;
    DelayValue delayValues[NoOfDelayValues];
};

} 
} 
#endif 
