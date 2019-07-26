









#include "trace_posix.h"

#include <cassert>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#ifdef WEBRTC_ANDROID
    #include <pthread.h>
#else
    #include <iostream>
#endif

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

namespace webrtc {
TracePosix::TracePosix()
{
    struct timeval systemTimeHighRes;
    gettimeofday(&systemTimeHighRes, 0);
    _prevAPITickCount = _prevTickCount = systemTimeHighRes.tv_sec;
}

TracePosix::~TracePosix()
{
    StopThread();
}

WebRtc_Word32 TracePosix::AddTime(char* traceMessage,
                                  const TraceLevel level) const
{
    struct timeval systemTimeHighRes;
    if (gettimeofday(&systemTimeHighRes, 0) == -1)
    {
        return -1;
    }
    struct tm buffer;
    const struct tm* systemTime =
        localtime_r(&systemTimeHighRes.tv_sec, &buffer);

    const WebRtc_UWord32 ms_time = systemTimeHighRes.tv_usec / 1000;
    WebRtc_UWord32 prevTickCount = 0;
    if (level == kTraceApiCall)
    {
        prevTickCount = _prevTickCount;
        _prevTickCount = ms_time;
    } else {
        prevTickCount = _prevAPITickCount;
        _prevAPITickCount = ms_time;
    }
    WebRtc_UWord32 dwDeltaTime = ms_time - prevTickCount;
    if (prevTickCount == 0)
    {
        dwDeltaTime = 0;
    }
    if (dwDeltaTime > 0x0fffffff)
    {
        
        dwDeltaTime = 0;
    }
    if(dwDeltaTime > 99999)
    {
        dwDeltaTime = 99999;
    }

    sprintf(traceMessage, "(%2u:%2u:%2u:%3u |%5lu) ", systemTime->tm_hour,
            systemTime->tm_min, systemTime->tm_sec, ms_time,
            static_cast<unsigned long>(dwDeltaTime));
    
    return 22;
}

WebRtc_Word32 TracePosix::AddBuildInfo(char* traceMessage) const
{
    sprintf(traceMessage, "Build info: %s", BUILDINFO);
    
    return strlen(traceMessage) + 1;
}

WebRtc_Word32 TracePosix::AddDateTimeInfo(char* traceMessage) const
{
    time_t t;
    time(&t);
    char buffer[26];  
    sprintf(traceMessage, "Local Date: %s", ctime_r(&t, buffer));
    WebRtc_Word32 len = static_cast<WebRtc_Word32>(strlen(traceMessage));

    if ('\n' == traceMessage[len - 1])
    {
        traceMessage[len - 1] = '\0';
        --len;
    }

    
    return len + 1;
}
} 
