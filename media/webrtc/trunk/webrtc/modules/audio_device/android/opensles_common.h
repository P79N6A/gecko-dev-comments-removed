









#ifndef WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_OPENSLES_COMMON_H_
#define WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_OPENSLES_COMMON_H_

#include <SLES/OpenSLES.h>

namespace webrtc_opensl {

SLDataFormat_PCM CreatePcmConfiguration(int sample_rate);

}  

#endif  
