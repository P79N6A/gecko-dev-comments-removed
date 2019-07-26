









#include "webrtc/modules/utility/source/rtp_dump_impl.h"

#include <assert.h>
#include <stdio.h>

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

#if defined(_WIN32)
#include <Windows.h>
#include <mmsystem.h>
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC) || defined(WEBRTC_BSD)
#include <string.h>
#include <sys/time.h>
#include <time.h>
#endif

#if (defined(_DEBUG) && defined(_WIN32))
#define DEBUG_PRINT(expr)   OutputDebugString(##expr)
#define DEBUG_PRINTP(expr, p)   \
{                               \
    char msg[128];              \
    sprintf(msg, ##expr, p);    \
    OutputDebugString(msg);     \
}
#else
#define DEBUG_PRINT(expr)    ((void)0)
#define DEBUG_PRINTP(expr,p) ((void)0)
#endif  

namespace webrtc {
const char RTPFILE_VERSION[] = "1.0";
const uint32_t MAX_UWORD32 = 0xffffffff;




typedef struct
{
    
    
    uint16_t length;
    
    uint16_t plen;
    
    uint32_t offset;
} rtpDumpPktHdr_t;

RtpDump* RtpDump::CreateRtpDump()
{
    return new RtpDumpImpl();
}

void RtpDump::DestroyRtpDump(RtpDump* object)
{
    delete object;
}

RtpDumpImpl::RtpDumpImpl()
    : _critSect(CriticalSectionWrapper::CreateCriticalSection()),
      _file(*FileWrapper::Create()),
      _startTime(0)
{
    WEBRTC_TRACE(kTraceMemory, kTraceUtility, -1, "%s created", __FUNCTION__);
}

RtpDump::~RtpDump()
{
}

RtpDumpImpl::~RtpDumpImpl()
{
    _file.Flush();
    _file.CloseFile();
    delete &_file;
    delete _critSect;
    WEBRTC_TRACE(kTraceMemory, kTraceUtility, -1, "%s deleted", __FUNCTION__);
}

int32_t RtpDumpImpl::Start(const char* fileNameUTF8)
{

    if (fileNameUTF8 == NULL)
    {
        return -1;
    }

    CriticalSectionScoped lock(_critSect);
    _file.Flush();
    _file.CloseFile();
    if (_file.OpenFile(fileNameUTF8, false, false, false) == -1)
    {
        WEBRTC_TRACE(kTraceError, kTraceUtility, -1,
                     "failed to open the specified file");
        return -1;
    }

    
    _startTime = GetTimeInMS();

    
    char magic[16];
    sprintf(magic, "#!rtpplay%s \n", RTPFILE_VERSION);
    if (_file.WriteText(magic) == -1)
    {
        WEBRTC_TRACE(kTraceError, kTraceUtility, -1,
                     "error writing to file");
        return -1;
    }

    
    
    
    
    
    
    
    char dummyHdr[16];
    memset(dummyHdr, 0, 16);
    if (!_file.Write(dummyHdr, sizeof(dummyHdr)))
    {
        WEBRTC_TRACE(kTraceError, kTraceUtility, -1,
                     "error writing to file");
        return -1;
    }
    return 0;
}

int32_t RtpDumpImpl::Stop()
{
    CriticalSectionScoped lock(_critSect);
    _file.Flush();
    _file.CloseFile();
    return 0;
}

bool RtpDumpImpl::IsActive() const
{
    CriticalSectionScoped lock(_critSect);
    return _file.Open();
}

int32_t RtpDumpImpl::DumpPacket(const uint8_t* packet, uint16_t packetLength)
{
    CriticalSectionScoped lock(_critSect);
    if (!IsActive())
    {
        return 0;
    }

    if (packet == NULL)
    {
        return -1;
    }

    if (packetLength < 1)
    {
        return -1;
    }

    
    
    bool isRTCP = RTCP(packet);

    rtpDumpPktHdr_t hdr;
    uint32_t offset;

    
    offset = GetTimeInMS();
    if (offset < _startTime)
    {
        
        offset += MAX_UWORD32 - _startTime + 1;
    } else {
        offset -= _startTime;
    }
    hdr.offset = RtpDumpHtonl(offset);

    hdr.length = RtpDumpHtons((uint16_t)(packetLength + sizeof(hdr)));
    if (isRTCP)
    {
        hdr.plen = 0;
    }
    else
    {
        hdr.plen = RtpDumpHtons((uint16_t)packetLength);
    }

    if (!_file.Write(&hdr, sizeof(hdr)))
    {
        WEBRTC_TRACE(kTraceError, kTraceUtility, -1,
                     "error writing to file");
        return -1;
    }
    if (!_file.Write(packet, packetLength))
    {
        WEBRTC_TRACE(kTraceError, kTraceUtility, -1,
                     "error writing to file");
        return -1;
    }

    return 0;
}

bool RtpDumpImpl::RTCP(const uint8_t* packet) const
{
    const uint8_t payloadType = packet[1];
    bool is_rtcp = false;

    switch(payloadType)
    {
    case 192:
        is_rtcp = true;
        break;
    case 193: case 195:
        break;
    case 200: case 201: case 202: case 203:
    case 204: case 205: case 206: case 207:
        is_rtcp = true;
        break;
    }
    return is_rtcp;
}


inline uint32_t RtpDumpImpl::GetTimeInMS() const
{
#if defined(_WIN32)
    return timeGetTime();
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_BSD) || defined(WEBRTC_MAC)
    struct timeval tv;
    struct timezone tz;
    unsigned long val;

    gettimeofday(&tv, &tz);
    val = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return val;
#endif
}

inline uint32_t RtpDumpImpl::RtpDumpHtonl(uint32_t x) const
{
#if defined(WEBRTC_ARCH_BIG_ENDIAN)
    return x;
#elif defined(WEBRTC_ARCH_LITTLE_ENDIAN)
    return (x >> 24) + ((((x >> 16) & 0xFF) << 8) + ((((x >> 8) & 0xFF) << 16) +
                                                     ((x & 0xFF) << 24)));
#endif
}

inline uint16_t RtpDumpImpl::RtpDumpHtons(uint16_t x) const
{
#if defined(WEBRTC_ARCH_BIG_ENDIAN)
    return x;
#elif defined(WEBRTC_ARCH_LITTLE_ENDIAN)
    return (x >> 8) + ((x & 0xFF) << 8);
#endif
}
}  
