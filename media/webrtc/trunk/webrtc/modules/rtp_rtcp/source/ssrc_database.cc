









#include "ssrc_database.h"

#include "critical_section_wrapper.h"
#include "trace.h"

#include <stdlib.h>
#include <cassert>

#ifdef _WIN32
    #include <windows.h>
    #include <MMSystem.h> 


    #pragma warning(disable:4311)
    #pragma warning(disable:4312)
#else
    #include <stdio.h>
    #include <string.h>
    #include <time.h>
    #include <sys/time.h>
#endif

namespace webrtc {
SSRCDatabase*
SSRCDatabase::StaticInstance(CountOperation count_operation)
{
  SSRCDatabase* impl =
      GetStaticInstance<SSRCDatabase>(count_operation);
  return impl;
}

SSRCDatabase*
SSRCDatabase::GetSSRCDatabase()
{
    return StaticInstance(kAddRef);
}

void
SSRCDatabase::ReturnSSRCDatabase()
{
    StaticInstance(kRelease);
}

WebRtc_UWord32
SSRCDatabase::CreateSSRC()
{
    CriticalSectionScoped lock(_critSect);

    WebRtc_UWord32 ssrc = GenerateRandom();

#ifndef WEBRTC_NO_STL

    while(_ssrcMap.find(ssrc) != _ssrcMap.end())
    {
        ssrc = GenerateRandom();
    }
    _ssrcMap[ssrc] = 0;

#else
    if(_sizeOfSSRC <= _numberOfSSRC)
    {
        
        const int newSize = _sizeOfSSRC + 10;
        WebRtc_UWord32* tempSSRCVector = new WebRtc_UWord32[newSize];
        memcpy(tempSSRCVector, _ssrcVector, _sizeOfSSRC*sizeof(WebRtc_UWord32));
        delete [] _ssrcVector;

        _ssrcVector = tempSSRCVector;
        _sizeOfSSRC = newSize;
    }

    
    if(_ssrcVector)
    {
        for (int i=0; i<_numberOfSSRC; i++)
        {
            if (_ssrcVector[i] == ssrc)
            {
                
                i = 0; 
                ssrc = GenerateRandom();
            }

        }
        
        _ssrcVector[_numberOfSSRC] = ssrc;
        _numberOfSSRC++;
    }
#endif
    return ssrc;
}

WebRtc_Word32
SSRCDatabase::RegisterSSRC(const WebRtc_UWord32 ssrc)
{
    CriticalSectionScoped lock(_critSect);

#ifndef WEBRTC_NO_STL

    _ssrcMap[ssrc] = 0;

#else
    if(_sizeOfSSRC <= _numberOfSSRC)
    {
        
        const int newSize = _sizeOfSSRC + 10;
        WebRtc_UWord32* tempSSRCVector = new WebRtc_UWord32[newSize];
        memcpy(tempSSRCVector, _ssrcVector, _sizeOfSSRC*sizeof(WebRtc_UWord32));
        delete [] _ssrcVector;

        _ssrcVector = tempSSRCVector;
        _sizeOfSSRC = newSize;
    }
    
    if(_ssrcVector)
    {
        for (int i=0; i<_numberOfSSRC; i++)
        {
            if (_ssrcVector[i] == ssrc)
            {
                
                return -1;
            }
        }
        
        _ssrcVector[_numberOfSSRC] = ssrc;
        _numberOfSSRC++;
    }
#endif
    return 0;
}

WebRtc_Word32
SSRCDatabase::ReturnSSRC(const WebRtc_UWord32 ssrc)
{
    CriticalSectionScoped lock(_critSect);

#ifndef WEBRTC_NO_STL
    _ssrcMap.erase(ssrc);

#else
    if(_ssrcVector)
    {
        for (int i=0; i<_numberOfSSRC; i++)
        {
            if (_ssrcVector[i] == ssrc)
            {
                
                
                _ssrcVector[i] = _ssrcVector[_numberOfSSRC-1];
                _numberOfSSRC--;
                break;
            }
        }
    }
#endif
    return 0;
}

SSRCDatabase::SSRCDatabase()
{
    
#ifdef _WIN32
    srand(timeGetTime());
#else
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    srand(tv.tv_usec);
#endif

#ifdef WEBRTC_NO_STL
    _sizeOfSSRC = 10;
    _numberOfSSRC = 0;
    _ssrcVector = new WebRtc_UWord32[10];
#endif
    _critSect = CriticalSectionWrapper::CreateCriticalSection();

    WEBRTC_TRACE(kTraceMemory, kTraceRtpRtcp, -1, "%s created", __FUNCTION__);
}

SSRCDatabase::~SSRCDatabase()
{
#ifdef WEBRTC_NO_STL
    delete [] _ssrcVector;
#else
    _ssrcMap.clear();
#endif
    delete _critSect;

    WEBRTC_TRACE(kTraceMemory, kTraceRtpRtcp, -1, "%s deleted", __FUNCTION__);
}

WebRtc_UWord32 SSRCDatabase::GenerateRandom()
{
    WebRtc_UWord32 ssrc = 0;
    do
    {
        ssrc = rand();
        ssrc = ssrc <<16;
        ssrc += rand();

    } while (ssrc == 0 || ssrc == 0xffffffff);

    return ssrc;
}
} 
