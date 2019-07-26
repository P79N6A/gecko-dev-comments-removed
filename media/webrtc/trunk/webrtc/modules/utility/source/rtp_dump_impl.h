









#ifndef WEBRTC_MODULES_UTILITY_SOURCE_RTP_DUMP_IMPL_H_
#define WEBRTC_MODULES_UTILITY_SOURCE_RTP_DUMP_IMPL_H_

#include "webrtc/modules/utility/interface/rtp_dump.h"

namespace webrtc {
class CriticalSectionWrapper;
class FileWrapper;
class RtpDumpImpl : public RtpDump
{
public:
    RtpDumpImpl();
    virtual ~RtpDumpImpl();

    virtual int32_t Start(const char* fileNameUTF8);
    virtual int32_t Stop();
    virtual bool IsActive() const;
    virtual int32_t DumpPacket(const uint8_t* packet, uint16_t packetLength);
private:
    
    inline uint32_t GetTimeInMS() const;
    
    inline uint32_t RtpDumpHtonl(uint32_t x) const;
    
    inline uint16_t RtpDumpHtons(uint16_t x) const;

    
    
    
    bool RTCP(const uint8_t* packet) const;

private:
    CriticalSectionWrapper* _critSect;
    FileWrapper& _file;
    uint32_t _startTime;
};
}  
#endif 
