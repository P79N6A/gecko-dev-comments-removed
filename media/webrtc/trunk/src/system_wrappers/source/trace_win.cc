









#include "trace_win.h"

#include <cassert>
#include <stdarg.h>

#include "Mmsystem.h"

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
TraceWindows::TraceWindows()
    : _prevAPITickCount(0),
      _prevTickCount(0)
{
}

TraceWindows::~TraceWindows()
{
    StopThread();
}

WebRtc_Word32 TraceWindows::AddTime(char* traceMessage,
                                    const TraceLevel level) const
{
    WebRtc_UWord32 dwCurrentTime = timeGetTime();
    SYSTEMTIME systemTime;
    GetSystemTime(&systemTime);

    if(level == kTraceApiCall)
    {
        WebRtc_UWord32 dwDeltaTime = dwCurrentTime- _prevTickCount;
        _prevTickCount = dwCurrentTime;

        if(_prevTickCount == 0)
        {
            dwDeltaTime = 0;
        }
        if(dwDeltaTime > 0x0fffffff)
        {
            
            dwDeltaTime = 0;
        }
        if(dwDeltaTime > 99999)
        {
            dwDeltaTime = 99999;
        }

        sprintf (traceMessage, "(%2u:%2u:%2u:%3u |%5lu) ", systemTime.wHour,
                 systemTime.wMinute, systemTime.wSecond,
                 systemTime.wMilliseconds, dwDeltaTime);
    } else {
        WebRtc_UWord32 dwDeltaTime = dwCurrentTime - _prevAPITickCount;
        _prevAPITickCount = dwCurrentTime;

        if(_prevAPITickCount == 0)
        {
            dwDeltaTime = 0;
        }
        if(dwDeltaTime > 0x0fffffff)
        {
            
            dwDeltaTime = 0;
        }
        if(dwDeltaTime > 99999)
        {
            dwDeltaTime = 99999;
        }
        sprintf (traceMessage, "(%2u:%2u:%2u:%3u |%5lu) ", systemTime.wHour,
                 systemTime.wMinute, systemTime.wSecond,
                 systemTime.wMilliseconds, dwDeltaTime);
    }
    
    return 22;
}

WebRtc_Word32 TraceWindows::AddBuildInfo(char* traceMessage) const
{
    
    sprintf(traceMessage, "Build info: %s", BUILDINFO);
    
    return static_cast<WebRtc_Word32>(strlen(traceMessage)+1);
}

WebRtc_Word32 TraceWindows::AddDateTimeInfo(char* traceMessage) const
{
    _prevAPITickCount = timeGetTime();
    _prevTickCount = _prevAPITickCount;

    SYSTEMTIME sysTime;
    GetLocalTime (&sysTime);

    TCHAR szDateStr[20];
    TCHAR szTimeStr[20];

    
    GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &sysTime, TEXT("MMM dd yyyy"),
                  szDateStr, 20);

    
    GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &sysTime, TEXT("HH':'mm':'ss"),
                  szTimeStr, 20);

    sprintf(traceMessage, "Local Date: %s Local Time: %s", szDateStr,
            szTimeStr);

    
    return static_cast<WebRtc_Word32>(strlen(traceMessage)+ 1);
}
} 
