









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_VIDEO_CAPTURE_DELAY_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_VIDEO_CAPTURE_DELAY_H_

namespace webrtc
{
namespace videocapturemodule
{

struct DelayValue
{
    int32_t width;
    int32_t height;
    int32_t delay;
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
