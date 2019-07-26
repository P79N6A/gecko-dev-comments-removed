









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_H

#include "typedefs.h"

namespace webrtc
{

class AudioDeviceUtility
{
public:
    static WebRtc_UWord32 GetTimeInMS();
	static void WaitForKey();
    static bool StringCompare(const char* str1,
                              const char* str2,
                              const WebRtc_UWord32 length);
	virtual WebRtc_Word32 Init() = 0;

	virtual ~AudioDeviceUtility() {}
};

}  

#endif  

