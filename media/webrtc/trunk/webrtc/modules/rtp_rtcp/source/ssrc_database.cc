









#include "webrtc/modules/rtp_rtcp/source/ssrc_database.h"

#include <assert.h>
#include <stdlib.h>

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"

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

uint32_t
SSRCDatabase::CreateSSRC()
{
    CriticalSectionScoped lock(_critSect);

    uint32_t ssrc = GenerateRandom();

    while(_ssrcMap.find(ssrc) != _ssrcMap.end())
    {
        ssrc = GenerateRandom();
    }
    _ssrcMap[ssrc] = 0;

    return ssrc;
}

int32_t
SSRCDatabase::RegisterSSRC(const uint32_t ssrc)
{
    CriticalSectionScoped lock(_critSect);
    _ssrcMap[ssrc] = 0;
    return 0;
}

int32_t
SSRCDatabase::ReturnSSRC(const uint32_t ssrc)
{
    CriticalSectionScoped lock(_critSect);
    _ssrcMap.erase(ssrc);
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

    _critSect = CriticalSectionWrapper::CreateCriticalSection();
}

SSRCDatabase::~SSRCDatabase()
{
    _ssrcMap.clear();
    delete _critSect;
}

uint32_t SSRCDatabase::GenerateRandom()
{
    uint32_t ssrc = 0;
    do
    {
        ssrc = rand();
        ssrc = ssrc <<16;
        ssrc += rand();

    } while (ssrc == 0 || ssrc == 0xffffffff);

    return ssrc;
}
}  
