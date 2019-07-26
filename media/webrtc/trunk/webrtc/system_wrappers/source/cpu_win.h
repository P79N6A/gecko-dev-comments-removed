













#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_CPU_WINDOWS_NO_CPOL_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_CPU_WINDOWS_NO_CPOL_H_

#include "cpu_wrapper.h"

#include <Wbemidl.h>

namespace webrtc {
class ConditionVariableWrapper;
class CriticalSectionWrapper;
class EventWrapper;
class ThreadWrapper;

class CpuWindows : public CpuWrapper
{
public:
    virtual WebRtc_Word32 CpuUsage();
    virtual WebRtc_Word32 CpuUsage(WebRtc_Word8* ,
                                   WebRtc_UWord32 ) {return -1;}
    virtual WebRtc_Word32 CpuUsage(WebRtc_UWord32  ) {return -1;}

    virtual WebRtc_Word32 CpuUsageMultiCore(WebRtc_UWord32& num_cores,
                                            WebRtc_UWord32*& cpu_usage);

    virtual void Reset() {}
    virtual void Stop() {}

    CpuWindows();
    virtual ~CpuWindows();
private:
    bool AllocateComplexDataTypes();
    void DeAllocateComplexDataTypes();

    void StartPollingCpu();
    bool StopPollingCpu();

    static bool Process(void* thread_object);
    bool ProcessImpl();

    bool CreateWmiConnection();
    bool CreatePerfOsRefresher();
    bool CreatePerfOsCpuHandles();
    bool Initialize();
    bool Terminate();

    bool UpdateCpuUsage();

    ThreadWrapper* cpu_polling_thread;

    bool initialize_;
    bool has_initialized_;
    CriticalSectionWrapper* init_crit_;
    ConditionVariableWrapper* init_cond_;

    bool terminate_;
    bool has_terminated_;
    CriticalSectionWrapper* terminate_crit_;
    ConditionVariableWrapper* terminate_cond_;

    
    EventWrapper* sleep_event;

    
    WebRtc_UWord32* cpu_usage_;

    
    
    
    IWbemObjectAccess** wbem_enum_access_;
    DWORD number_of_objects_;

    
    long cpu_usage_handle_;
    unsigned __int64* previous_processor_timestamp_;

    
    long timestamp_sys_100_ns_handle_;
    unsigned __int64* previous_100ns_timestamp_;

    IWbemServices* wbem_service_;
    IWbemServices* wbem_service_proxy_;

    IWbemRefresher* wbem_refresher_;

    IWbemHiPerfEnum* wbem_enum_;

};
} 
#endif 
