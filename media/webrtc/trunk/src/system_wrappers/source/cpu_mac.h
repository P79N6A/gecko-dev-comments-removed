









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_CPU_MAC_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_CPU_MAC_H_

#include "cpu_wrapper.h"

namespace webrtc {
class CpuWrapperMac : public CpuWrapper
{
public:
    CpuWrapperMac();
    virtual ~CpuWrapperMac();

    virtual WebRtc_Word32 CpuUsage();
    virtual WebRtc_Word32 CpuUsage(WebRtc_Word8* ,
                                   WebRtc_UWord32 ) {return -1;}
    virtual WebRtc_Word32 CpuUsage(WebRtc_UWord32  ) {return -1;}

    
    
    
    virtual WebRtc_Word32 CpuUsageMultiCore(WebRtc_UWord32& numCores,
                                            WebRtc_UWord32*& array);

    virtual void Reset() {}
    virtual void Stop() {}

private:
    WebRtc_Word32 Update(WebRtc_Word64 timeDiffMS);
    
    WebRtc_UWord32  _cpuCount;
    WebRtc_UWord32* _cpuUsage;
    WebRtc_Word32   _totalCpuUsage;
    WebRtc_Word64*  _lastTickCount;
    WebRtc_Word64   _lastTime;
};
} 

#endif 
