














#ifndef WEBRTC_VOICE_ENGINE_VOICE_ENGINE_DEFINES_H
#define WEBRTC_VOICE_ENGINE_VOICE_ENGINE_DEFINES_H

#include "webrtc/common_types.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/system_wrappers/interface/logging.h"





namespace webrtc {


enum { kMinVolumeLevel = 0 };
enum { kMaxVolumeLevel = 255 };

const float kMinOutputVolumeScaling = 0.0f;

const float kMaxOutputVolumeScaling = 10.0f;

const float kMinOutputVolumePanning = 0.0f;

const float kMaxOutputVolumePanning = 1.0f;


enum { kMinDtmfEventCode = 0 };                 
enum { kMaxDtmfEventCode = 15 };                
enum { kMinTelephoneEventCode = 0 };            
enum { kMaxTelephoneEventCode = 255 };          
enum { kMinTelephoneEventDuration = 100 };
enum { kMaxTelephoneEventDuration = 60000 };    
enum { kMinTelephoneEventAttenuation = 0 };     
enum { kMaxTelephoneEventAttenuation = 36 };    
enum { kMinTelephoneEventSeparationMs = 100 };  
                                                
enum { kVoiceEngineMaxIpPacketSizeBytes = 1500 };       

enum { kVoiceEngineMaxModuleVersionSize = 960 };


enum { kVoiceEngineVersionMaxMessageSize = 1024 };


const NoiseSuppression::Level kDefaultNsMode = NoiseSuppression::kModerate;
const GainControl::Mode kDefaultAgcMode =
#if defined(WEBRTC_ANDROID) || defined(WEBRTC_IOS)
  GainControl::kAdaptiveDigital;
#else
  GainControl::kAdaptiveAnalog;
#endif
const bool kDefaultAgcState =
#if defined(WEBRTC_ANDROID) || defined(WEBRTC_IOS)
  false;
#else
  true;
#endif
const GainControl::Mode kDefaultRxAgcMode = GainControl::kAdaptiveDigital;



enum { kVoiceEngineMinIsacInitTargetRateBpsWb = 10000 };

enum { kVoiceEngineMaxIsacInitTargetRateBpsWb = 32000 };

enum { kVoiceEngineMinIsacInitTargetRateBpsSwb = 10000 };

enum { kVoiceEngineMaxIsacInitTargetRateBpsSwb = 56000 };

enum { kVoiceEngineMinIsacMaxRateBpsWb = 32000 };

enum { kVoiceEngineMaxIsacMaxRateBpsWb = 53400 };

enum { kVoiceEngineMinIsacMaxRateBpsSwb = 32000 };

enum { kVoiceEngineMaxIsacMaxRateBpsSwb = 107000 };

enum { kVoiceEngineMinIsacMaxPayloadSizeBytesWb = 120 };

enum { kVoiceEngineMaxIsacMaxPayloadSizeBytesWb = 400 };

enum { kVoiceEngineMinIsacMaxPayloadSizeBytesSwb = 120 };

enum { kVoiceEngineMaxIsacMaxPayloadSizeBytesSwb = 600 };



enum { kVoiceEngineMinMinPlayoutDelayMs = 0 };

enum { kVoiceEngineMaxMinPlayoutDelayMs = 10000 };



enum { kVoiceEngineMinPacketTimeoutSec = 1 };

enum { kVoiceEngineMaxPacketTimeoutSec = 150 };

enum { kVoiceEngineMinSampleTimeSec = 1 };

enum { kVoiceEngineMaxSampleTimeSec = 150 };



enum { kVoiceEngineMinRtpExtensionId = 1 };

enum { kVoiceEngineMaxRtpExtensionId = 14 };

}  





#if defined(_DEBUG)
#define BUILDMODE "d"
#elif defined(DEBUG)
#define BUILDMODE "d"
#elif defined(NDEBUG)
#define BUILDMODE "r"
#else
#define BUILDMODE "?"
#endif

#define BUILDTIME __TIME__
#define BUILDDATE __DATE__


#define BUILDINFO BUILDDATE " " BUILDTIME " " BUILDMODE





#define NOT_SUPPORTED(stat)                  \
  LOG_F(LS_ERROR) << "not supported";        \
  stat.SetLastError(VE_FUNC_NOT_SUPPORTED);  \
  return -1;

#if (defined(_DEBUG) && defined(_WIN32) && (_MSC_VER >= 1400))
  #include <windows.h>
  #include <stdio.h>
  #define DEBUG_PRINT(...)      \
  {                             \
    char msg[256];              \
    sprintf(msg, __VA_ARGS__);  \
    OutputDebugStringA(msg);    \
  }
#else
  
  #define DEBUG_PRINT(exp)      ((void)0)
#endif  

#define CHECK_CHANNEL(channel)  if (CheckChannel(channel) == -1) return -1;





namespace webrtc
{

inline int VoEId(int veId, int chId)
{
    if (chId == -1)
    {
        const int dummyChannel(99);
        return (int) ((veId << 16) + dummyChannel);
    }
    return (int) ((veId << 16) + chId);
}

inline int VoEModuleId(int veId, int chId)
{
    return (int) ((veId << 16) + chId);
}


inline int VoEChannelId(int moduleId)
{
    return (int) (moduleId & 0xffff);
}

}  







#if defined(_WIN32)

  #include <windows.h>

  #pragma comment( lib, "winmm.lib" )

  #ifndef WEBRTC_EXTERNAL_TRANSPORT
    #pragma comment( lib, "ws2_32.lib" )
  #endif






  #define WEBRTC_VOICE_ENGINE_DEFAULT_DEVICE \
    AudioDeviceModule::kDefaultCommunicationDevice

#endif  



#ifdef WEBRTC_LINUX

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#ifndef QNX
  #include <linux/net.h>
#ifndef ANDROID
  #include <sys/soundcard.h>
#endif 
#endif 
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define DWORD unsigned long int
#define WINAPI
#define LPVOID void *
#define FALSE 0
#define TRUE 1
#define UINT unsigned int
#define UCHAR unsigned char
#define TCHAR char
#ifdef QNX
#define _stricmp stricmp
#else
#define _stricmp strcasecmp
#endif
#define GetLastError() errno
#define WSAGetLastError() errno
#define LPCTSTR const char*
#define LPCSTR const char*
#define wsprintf sprintf
#define TEXT(a) a
#define _ftprintf fprintf
#define _tcslen strlen
#define FAR
#define __cdecl
#define LPSOCKADDR struct sockaddr *


#define WEBRTC_VOICE_ENGINE_DEFAULT_DEVICE 0

#ifdef ANDROID





  
  #undef WEBRTC_CODEC_ISAC
  #undef WEBRTC_VOE_EXTERNAL_REC_AND_PLAYOUT

  #define ANDROID_NOT_SUPPORTED(stat) NOT_SUPPORTED(stat)

#else 





  #define ANDROID_NOT_SUPPORTED(stat)

#endif 

#else
#define ANDROID_NOT_SUPPORTED(stat)
#endif  




#ifdef WEBRTC_MAC

#include <AudioUnit/AudioUnit.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#if !defined(WEBRTC_IOS)
  #include <CoreServices/CoreServices.h>
  #include <CoreAudio/CoreAudio.h>
  #include <AudioToolbox/DefaultAudioOutput.h>
  #include <AudioToolbox/AudioConverter.h>
  #include <CoreAudio/HostTime.h>
#endif

#define DWORD unsigned long int
#define WINAPI
#define LPVOID void *
#define FALSE 0
#define TRUE 1
#define SOCKADDR_IN struct sockaddr_in
#define UINT unsigned int
#define UCHAR unsigned char
#define TCHAR char
#define _stricmp strcasecmp
#define GetLastError() errno
#define WSAGetLastError() errno
#define LPCTSTR const char*
#define wsprintf sprintf
#define TEXT(a) a
#define _ftprintf fprintf
#define _tcslen strlen
#define FAR
#define __cdecl
#define LPSOCKADDR struct sockaddr *
#define LPCSTR const char*
#define ULONG unsigned long


#define WEBRTC_VOICE_ENGINE_DEFAULT_DEVICE 0


#if defined(WEBRTC_IOS)





  
  #undef WEBRTC_CODEC_ISAC
  #undef WEBRTC_VOE_EXTERNAL_REC_AND_PLAYOUT

  #define IPHONE_NOT_SUPPORTED(stat) NOT_SUPPORTED(stat)

#else 









  #define IPHONE_NOT_SUPPORTED(stat)
#endif

#else
#define IPHONE_NOT_SUPPORTED(stat)
#endif  

#endif 
