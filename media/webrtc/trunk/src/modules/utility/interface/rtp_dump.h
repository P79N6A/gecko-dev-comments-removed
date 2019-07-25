














#ifndef WEBRTC_MODULES_UTILITY_INTERFACE_RTP_DUMP_H_
#define WEBRTC_MODULES_UTILITY_INTERFACE_RTP_DUMP_H_

#include "typedefs.h"
#include "file_wrapper.h"

namespace webrtc {
class RtpDump
{
public:
    
    static RtpDump* CreateRtpDump();

    
    static void DestroyRtpDump(RtpDump* object);

    
    
    virtual WebRtc_Word32 Start(const char* fileNameUTF8) = 0;

    
    virtual WebRtc_Word32 Stop() = 0;

    
    virtual bool IsActive() const = 0;

    
    
    
    virtual WebRtc_Word32 DumpPacket(const WebRtc_UWord8* packet,
                                     WebRtc_UWord16 packetLength) = 0;

protected:
    virtual ~RtpDump();
};
} 
#endif 
