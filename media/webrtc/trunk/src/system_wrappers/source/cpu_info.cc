









#include "cpu_info.h"

#if defined(_WIN32)
#include <Windows.h>
#elif defined(WEBRTC_MAC)
#include <sys/types.h>
#include <sys/sysctl.h>
#elif defined(WEBRTC_MAC_INTEL)

#elif defined(WEBRTC_ANDROID)

#else 
#include <sys/sysinfo.h>
#endif

#include "trace.h"

namespace webrtc {

WebRtc_UWord32 CpuInfo::_numberOfCores = 0;

WebRtc_UWord32 CpuInfo::DetectNumberOfCores()
{
    if (!_numberOfCores)
    {
#if defined(_WIN32)
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        _numberOfCores = static_cast<WebRtc_UWord32>(si.dwNumberOfProcessors);
        WEBRTC_TRACE(kTraceStateInfo, kTraceUtility, -1,
                     "Available number of cores:%d", _numberOfCores);

#elif defined(WEBRTC_LINUX) && !defined(WEBRTC_ANDROID)
        _numberOfCores = get_nprocs();
        WEBRTC_TRACE(kTraceStateInfo, kTraceUtility, -1,
                     "Available number of cores:%d", _numberOfCores);

#elif (defined(WEBRTC_MAC) || defined(WEBRTC_MAC_INTEL))
        int name[] = {CTL_HW, HW_AVAILCPU};
        int ncpu;
        size_t size = sizeof(ncpu);
        if(0 == sysctl(name, 2, &ncpu, &size, NULL, 0))
        {
            _numberOfCores = static_cast<WebRtc_UWord32>(ncpu);
            WEBRTC_TRACE(kTraceStateInfo, kTraceUtility, -1,
                         "Available number of cores:%d", _numberOfCores);
    } else
    {
            WEBRTC_TRACE(kTraceError, kTraceUtility, -1,
                         "Failed to get number of cores");
            _numberOfCores = 1;
    }
#else
        WEBRTC_TRACE(kTraceWarning, kTraceUtility, -1,
                     "No function to get number of cores");
        _numberOfCores = 1;
#endif
    }
    return _numberOfCores;
}

} 
