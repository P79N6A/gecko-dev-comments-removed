









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_H

#include "webrtc/typedefs.h"

namespace webrtc
{

class AudioDeviceUtility
{
public:
    static uint32_t GetTimeInMS();
	static void WaitForKey();
    static bool StringCompare(const char* str1,
                              const char* str2,
                              const uint32_t length);
	virtual int32_t Init() = 0;

	virtual ~AudioDeviceUtility() {}
};

}  

#endif  
