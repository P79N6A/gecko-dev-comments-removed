









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_CONFIG_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_CONFIG_H



enum { kAdmMaxIdleTimeProcess = 1000 };
enum { GET_MIC_VOLUME_INTERVAL_MS = 1000 };



#if defined(_WIN32)
#if (_MSC_VER >= 1400)


#define WEBRTC_WINDOWS_CORE_AUDIO_BUILD
#endif
#endif

#if (defined(_DEBUG) && defined(_WIN32) && (_MSC_VER >= 1400))
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#define DEBUG_PRINT(...)		            \
{								            \
	TCHAR msg[256];				            \
	StringCchPrintf(msg, 256, __VA_ARGS__);	\
	OutputDebugString(msg);		            \
}
#else
#define DEBUG_PRINT(exp)		((void)0)
#endif

#endif  

