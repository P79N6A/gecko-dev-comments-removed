









#ifndef WEBRTC_MODULES_UTILITY_SOURCE_RTP_DUMP_IMPL_H_
#define WEBRTC_MODULES_UTILITY_SOURCE_RTP_DUMP_IMPL_H_

#include "rtp_dump.h"

namespace webrtc {
class CriticalSectionWrapper;
class FileWrapper;
class RtpDumpImpl : public RtpDump
{
public:
    RtpDumpImpl();
    virtual ~RtpDumpImpl();

    virtual WebRtc_Word32 Start(const char* fileNameUTF8);
    virtual WebRtc_Word32 Stop();
    virtual bool IsActive() const;
    virtual WebRtc_Word32 DumpPacket(const WebRtc_UWord8* packet,
                                     WebRtc_UWord16 packetLength);
private:
    
    inline WebRtc_UWord32 GetTimeInMS() const;
    
    inline WebRtc_UWord32 RtpDumpHtonl(WebRtc_UWord32 x) const;
    
    inline WebRtc_UWord16 RtpDumpHtons(WebRtc_UWord16 x) const;

    
    
    
    bool RTCP(const WebRtc_UWord8* packet) const;

private:
    CriticalSectionWrapper* _critSect;
    FileWrapper& _file;
    WebRtc_UWord32 _startTime;
};
} 
#endif 
