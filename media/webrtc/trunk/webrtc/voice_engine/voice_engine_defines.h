














#ifndef WEBRTC_VOICE_ENGINE_VOICE_ENGINE_DEFINES_H
#define WEBRTC_VOICE_ENGINE_VOICE_ENGINE_DEFINES_H

#include "common_types.h"
#include "engine_configurations.h"





namespace webrtc
{


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



enum { kVoiceEngineMaxSrtpKeyLength = 30 };

enum { kVoiceEngineMinSrtpEncryptLength = 16 };

enum { kVoiceEngineMaxSrtpEncryptLength = 256 };


enum { kVoiceEngineMaxSrtpAuthSha1Length = 20 };


enum { kVoiceEngineMaxSrtpTagAuthNullLength = 12 };


enum { kVoiceEngineMaxSrtpKeyAuthNullLength = 256 };


enum { kVoiceEngineAudioProcessingDeviceSampleRateHz = 48000 };



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

enum { kVoiceEngineMaxMinPlayoutDelayMs = 1000 };



enum { kVoiceEngineMinPacketTimeoutSec = 1 };

enum { kVoiceEngineMaxPacketTimeoutSec = 150 };

enum { kVoiceEngineMinSampleTimeSec = 1 };

enum { kVoiceEngineMaxSampleTimeSec = 150 };



enum { kVoiceEngineMinRtpExtensionId = 1 };

enum { kVoiceEngineMaxRtpExtensionId = 14 };

} 



#define WEBRTC_AUDIO_PROCESSING_OFF false

#define WEBRTC_VOICE_ENGINE_HP_DEFAULT_STATE true
    
#define WEBRTC_VOICE_ENGINE_NS_DEFAULT_STATE  WEBRTC_AUDIO_PROCESSING_OFF
    
#define WEBRTC_VOICE_ENGINE_AGC_DEFAULT_STATE true
    
#define WEBRTC_VOICE_ENGINE_EC_DEFAULT_STATE  WEBRTC_AUDIO_PROCESSING_OFF
    
#define WEBRTC_VOICE_ENGINE_VAD_DEFAULT_STATE WEBRTC_AUDIO_PROCESSING_OFF
    
#define WEBRTC_VOICE_ENGINE_RX_AGC_DEFAULT_STATE WEBRTC_AUDIO_PROCESSING_OFF
    
#define WEBRTC_VOICE_ENGINE_RX_NS_DEFAULT_STATE WEBRTC_AUDIO_PROCESSING_OFF
    
#define WEBRTC_VOICE_ENGINE_RX_HP_DEFAULT_STATE WEBRTC_AUDIO_PROCESSING_OFF
    

#define WEBRTC_VOICE_ENGINE_NS_DEFAULT_MODE NoiseSuppression::kModerate
    
#define WEBRTC_VOICE_ENGINE_AGC_DEFAULT_MODE GainControl::kAdaptiveAnalog
    
#define WEBRTC_VOICE_ENGINE_RX_AGC_DEFAULT_MODE GainControl::kAdaptiveDigital
    
#define WEBRTC_VOICE_ENGINE_RX_NS_DEFAULT_MODE NoiseSuppression::kModerate
    



#define STR_CASE_CMP(x,y) ::_stricmp(x,y)

#define STR_NCASE_CMP(x,y,n) ::_strnicmp(x,y,n)





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





#define WEBRTC_VOICE_ENGINE_DEFAULT_TRACE_FILTER \
    kTraceStateInfo | kTraceWarning | kTraceError | kTraceCritical | \
    kTraceApiCall





namespace webrtc
{

inline int VoEId(const int veId, const int chId)
{
    if (chId == -1)
    {
        const int dummyChannel(99);
        return (int) ((veId << 16) + dummyChannel);
    }
    return (int) ((veId << 16) + chId);
}

inline int VoEModuleId(const int veId, const int chId)
{
    return (int) ((veId << 16) + chId);
}


inline int VoEChannelId(const int moduleId)
{
    return (int) (moduleId & 0xffff);
}

} 







#if defined(_WIN32)

  #pragma comment( lib, "winmm.lib" )

  #ifndef WEBRTC_EXTERNAL_TRANSPORT
    #pragma comment( lib, "ws2_32.lib" )
  #endif





namespace webrtc
{

enum { kVoiceEngineMaxNumOfChannels = 32 };

enum { kVoiceEngineMaxNumOfActiveChannels = 16 };
} 





  #include <windows.h>

  
  #define STR_CASE_CMP(x,y) ::_stricmp(x,y)
  
  #define STR_NCASE_CMP(x,y,n) ::_strnicmp(x,y,n)


  #define WEBRTC_VOICE_ENGINE_DEFAULT_DEVICE \
    AudioDeviceModule::kDefaultCommunicationDevice

#endif  



#ifdef WEBRTC_LINUX

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef QNX
  #include <linux/net.h>
#ifndef ANDROID
  #include <sys/soundcard.h>
#endif 
#endif 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <time.h>
#include <sys/time.h>

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





namespace webrtc
{
  
  enum { kVoiceEngineMaxNumOfChannels = 32 };
  
  enum { kVoiceEngineMaxNumOfActiveChannels = 16 };
} 





  
  #undef WEBRTC_CODEC_ISAC
  
  
  
  
  #undef WEBRTC_CONFERENCING
  #undef WEBRTC_TYPING_DETECTION

  
  #undef  WEBRTC_VOICE_ENGINE_NS_DEFAULT_STATE
  #undef  WEBRTC_VOICE_ENGINE_AGC_DEFAULT_STATE
  #undef  WEBRTC_VOICE_ENGINE_EC_DEFAULT_STATE
  #define WEBRTC_VOICE_ENGINE_NS_DEFAULT_STATE  WEBRTC_AUDIO_PROCESSING_OFF
  #define WEBRTC_VOICE_ENGINE_AGC_DEFAULT_STATE WEBRTC_AUDIO_PROCESSING_OFF
  #define WEBRTC_VOICE_ENGINE_EC_DEFAULT_STATE  WEBRTC_AUDIO_PROCESSING_OFF

  
  #undef  WEBRTC_VOICE_ENGINE_NS_DEFAULT_MODE
  #undef  WEBRTC_VOICE_ENGINE_AGC_DEFAULT_MODE
  #define WEBRTC_VOICE_ENGINE_NS_DEFAULT_MODE  \
      NoiseSuppression::kModerate
  #define WEBRTC_VOICE_ENGINE_AGC_DEFAULT_MODE \
      GainControl::kAdaptiveDigital

  
  
  
  #define ANDROID_NOT_SUPPORTED(stat)

#else 




namespace webrtc
{
  
  enum { kVoiceEngineMaxNumOfChannels = 32 };
  
  enum { kVoiceEngineMaxNumOfActiveChannels = 16 };
} 





  #define ANDROID_NOT_SUPPORTED(stat)

#endif 

#else
#define ANDROID_NOT_SUPPORTED(stat)
#endif  




#ifdef WEBRTC_MAC

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/time.h>
#include <time.h>
#include <AudioUnit/AudioUnit.h>
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





namespace webrtc
{
  
  enum { kVoiceEngineMaxNumOfChannels = 2 };
  
  enum { kVoiceEngineMaxNumOfActiveChannels = 2 };
} 





  
  #undef WEBRTC_CODEC_ISAC
  #undef WEBRTC_VOE_EXTERNAL_REC_AND_PLAYOUT

  #undef  WEBRTC_VOICE_ENGINE_NS_DEFAULT_STATE
  #undef  WEBRTC_VOICE_ENGINE_AGC_DEFAULT_STATE
  #undef  WEBRTC_VOICE_ENGINE_EC_DEFAULT_STATE
  #define WEBRTC_VOICE_ENGINE_NS_DEFAULT_STATE  WEBRTC_AUDIO_PROCESSING_OFF
  #define WEBRTC_VOICE_ENGINE_AGC_DEFAULT_STATE WEBRTC_AUDIO_PROCESSING_OFF
  #define WEBRTC_VOICE_ENGINE_EC_DEFAULT_STATE  WEBRTC_AUDIO_PROCESSING_OFF

  #undef  WEBRTC_VOICE_ENGINE_NS_DEFAULT_MODE
  #undef  WEBRTC_VOICE_ENGINE_AGC_DEFAULT_MODE
  #define WEBRTC_VOICE_ENGINE_NS_DEFAULT_MODE \
      NoiseSuppression::kModerate
  #define WEBRTC_VOICE_ENGINE_AGC_DEFAULT_MODE \
      GainControl::kAdaptiveDigital

  #define IPHONE_NOT_SUPPORTED(stat) \
    stat.SetLastError(VE_FUNC_NOT_SUPPORTED, kTraceError, \
                      "API call not supported"); \
    return -1;

#else 





namespace webrtc
{
  
  enum { kVoiceEngineMaxNumOfChannels = 32 };
  
  enum { kVoiceEngineMaxNumOfActiveChannels = 16 };
} 





  #define IPHONE_NOT_SUPPORTED(stat)
#endif

#else
#define IPHONE_NOT_SUPPORTED(stat)
#endif  



#endif 
