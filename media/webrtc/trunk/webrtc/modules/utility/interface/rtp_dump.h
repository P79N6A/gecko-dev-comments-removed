














#ifndef WEBRTC_MODULES_UTILITY_INTERFACE_RTP_DUMP_H_
#define WEBRTC_MODULES_UTILITY_INTERFACE_RTP_DUMP_H_

#include "webrtc/system_wrappers/interface/file_wrapper.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class RtpDump
{
public:
    
    static RtpDump* CreateRtpDump();

    
    static void DestroyRtpDump(RtpDump* object);

    
    
    virtual int32_t Start(const char* fileNameUTF8) = 0;

    
    virtual int32_t Stop() = 0;

    
    virtual bool IsActive() const = 0;

    
    
    
    virtual int32_t DumpPacket(const uint8_t* packet,
                               uint16_t packetLength) = 0;

protected:
    virtual ~RtpDump();
};
}  
#endif 
