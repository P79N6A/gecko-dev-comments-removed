









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_CPU_LINUX_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_CPU_LINUX_H_

#include "cpu_wrapper.h"

namespace webrtc {
class CpuLinux : public CpuWrapper
{
public:
    CpuLinux();
    virtual ~CpuLinux();

    virtual WebRtc_Word32 CpuUsage();
    virtual WebRtc_Word32 CpuUsage(WebRtc_Word8* ,
                                   WebRtc_UWord32 ) {return 0;}
    virtual WebRtc_Word32 CpuUsage(WebRtc_UWord32 ) {return 0;}

    virtual WebRtc_Word32 CpuUsageMultiCore(WebRtc_UWord32& numCores,
                                            WebRtc_UWord32*& array);

    virtual void Reset() {return;}
    virtual void Stop() {return;}
private:
    int GetData(long long& busy, long long& idle, long long*& busyArray,
                long long*& idleArray);
    int GetNumCores();

    long long m_oldBusyTime;
    long long m_oldIdleTime;

    long long* m_oldBusyTimeMulti;
    long long* m_oldIdleTimeMulti;

    long long* m_idleArray;
    long long* m_busyArray;
    WebRtc_UWord32* m_resultArray;
    WebRtc_UWord32  m_numCores;
};
} 

#endif 
