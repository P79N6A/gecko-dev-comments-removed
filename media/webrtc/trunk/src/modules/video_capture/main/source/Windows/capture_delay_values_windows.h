









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_CAPTURE_DELAY_VALUES_WINDOWS_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_CAPTURE_DELAY_VALUES_WINDOWS_H_

#include "../video_capture_delay.h"

namespace webrtc
{
namespace videocapturemodule
{
const WebRtc_Word32 NoWindowsCaptureDelays=1;
const DelayValues WindowsCaptureDelays[NoWindowsCaptureDelays]=
{ 
    "Microsoft LifeCam Cinema","usb#vid_045e&pid_075d",{{640,480,125},{640,360,117},{424,240,111},{352,288,111},{320,240,116},{176,144,101},{160,120,109},{1280,720,166},{960,544,126},{800,448,120},{800,600,127}},
};

} 
} 
#endif 
