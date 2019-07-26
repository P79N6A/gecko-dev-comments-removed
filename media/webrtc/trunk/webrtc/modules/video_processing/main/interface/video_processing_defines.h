














#ifndef WEBRTC_MODULES_INTERFACE_VIDEO_PROCESSING_DEFINES_H
#define WEBRTC_MODULES_INTERFACE_VIDEO_PROCESSING_DEFINES_H

#include "typedefs.h"

namespace webrtc {


#define VPM_OK                   0
#define VPM_GENERAL_ERROR       -1
#define VPM_MEMORY              -2
#define VPM_PARAMETER_ERROR     -3
#define VPM_SCALE_ERROR         -4
#define VPM_UNINITIALIZED       -5
#define VPM_UNIMPLEMENTED       -6

enum VideoFrameResampling
{
  
    kNoRescaling,         
    kFastRescaling,       
    kBiLinear,            
    kBox,                 
};

} 

#endif
