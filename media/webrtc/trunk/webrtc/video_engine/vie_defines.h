









#ifndef WEBRTC_VIDEO_ENGINE_VIE_DEFINES_H_
#define WEBRTC_VIDEO_ENGINE_VIE_DEFINES_H_

#include "engine_configurations.h"  


#ifdef WEBRTC_ANDROID
#include <arpa/inet.h>  
#include <linux/net.h>  
#include <netinet/in.h>  
#include <pthread.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <sys/time.h>  
#include <time.h>  
#endif

namespace webrtc {


enum { kViEMinKeyRequestIntervalMs = 300 };


enum { kViEMaxNumberOfChannels = 32 };
enum { kViEVersionMaxMessageSize = 1024 };
enum { kViEMaxModuleVersionSize = 960 };


enum { kViEMaxCaptureDevices = 10 };
enum { kViECaptureDefaultWidth = 352 };
enum { kViECaptureDefaultHeight = 288 };
enum { kViECaptureDefaultFramerate = 30 };
enum { kViECaptureMaxSnapshotWaitTimeMs = 500 };


enum { kViEMaxCodecWidth = 4048 };
enum { kViEMaxCodecHeight = 3040 };
enum { kViEMaxCodecFramerate = 60 };
enum { kViEMinCodecBitrate = 30 };


enum { kViEMaxSrtpKeyLength = 30 };
enum { kViEMinSrtpEncryptLength = 16 };
enum { kViEMaxSrtpEncryptLength = 256 };
enum { kViEMaxSrtpAuthSh1Length = 20 };
enum { kViEMaxSrtpTagAuthNullLength = 12 };
enum { kViEMaxSrtpKeyAuthNullLength = 256 };


enum { kViEMaxFilePlayers = 3 };


enum { kViEMaxMtu = 1500 };
enum { kViESocketThreads = 1 };
enum { kViENumReceiveSocketBuffers = 500 };



enum { kViEMaxRenderTimeoutTimeMs  = 10000 };

enum { kViEMinRenderTimeoutTimeMs = 33 };
enum { kViEDefaultRenderDelayMs = 10 };


enum { kNackHistorySize = 400 };


enum {
  kViEChannelIdBase = 0x0,
  kViEChannelIdMax = 0xFF,
  kViECaptureIdBase = 0x1001,
  kViECaptureIdMax = 0x10FF,
  kViEFileIdBase = 0x2000,
  kViEFileIdMax = 0x200F,
  kViEDummyChannelId = 0xFFFF
};





inline int ViEId(const int vieId, const int channelId = -1) {
  if (channelId == -1) {
    return static_cast<int>((vieId << 16) + kViEDummyChannelId);
  }
  return static_cast<int>((vieId << 16) + channelId);
}

inline int ViEModuleId(const int vieId, const int channelId = -1) {
  if (channelId == -1) {
    return static_cast<int>((vieId << 16) + kViEDummyChannelId);
  }
  return static_cast<int>((vieId << 16) + channelId);
}

inline int ChannelId(const int moduleId) {
  return static_cast<int>(moduleId & 0xffff);
}


#if defined(_WIN32)
  
  #if defined(_DEBUG)
  #define BUILDMODE TEXT("d")
  #elif defined(DEBUG)
  #define BUILDMODE TEXT("d")
  #elif defined(NDEBUG)
  #define BUILDMODE TEXT("r")
  #else
  #define BUILDMODE TEXT("?")
  #endif

  #define BUILDTIME TEXT(__TIME__)
  #define BUILDDATE TEXT(__DATE__)

  
  #define BUILDINFO BUILDDATE TEXT(" ") BUILDTIME TEXT(" ") BUILDMODE
  #define RENDER_MODULE_TYPE kRenderWindows

  
  
  #pragma warning(disable: 4351)
  
  #pragma warning(disable: 4355)
  
  #pragma warning(disable: 4731)

  
  #pragma comment(lib, "winmm.lib")

  #ifndef WEBRTC_EXTERNAL_TRANSPORT
  #pragma comment(lib, "ws2_32.lib")
  #pragma comment(lib, "Iphlpapi.lib")   // _GetAdaptersAddresses
  #endif
#endif


#ifdef WEBRTC_MAC
  #define SLEEP(x) usleep(x * 1000)

  
  #define TEXT(x) x
  #if defined(_DEBUG)
  #define BUILDMODE TEXT("d")
  #elif defined(DEBUG)
  #define BUILDMODE TEXT("d")
  #elif defined(NDEBUG)
  #define BUILDMODE TEXT("r")
  #else
  #define BUILDMODE TEXT("?")
  #endif

  #define BUILDTIME TEXT(__TIME__)
  #define BUILDDATE TEXT(__DATE__)

  
  #define BUILDINFO BUILDDATE TEXT(" ") BUILDTIME TEXT(" ") BUILDMODE
  #define RENDER_MODULE_TYPE kRenderWindows
#endif


#ifndef WEBRTC_ANDROID
#ifdef WEBRTC_LINUX
  
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
#endif  
#endif  


#ifdef WEBRTC_ANDROID
  #define FAR
  #define __cdecl

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

#endif  

}  

#endif  
