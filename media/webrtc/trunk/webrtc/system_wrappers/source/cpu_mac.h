









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_CPU_MAC_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_CPU_MAC_H_

#include "system_wrappers/interface/cpu_wrapper.h"

namespace webrtc {

class CpuWrapperMac : public CpuWrapper {
 public:
  CpuWrapperMac();
  virtual ~CpuWrapperMac();

  virtual WebRtc_Word32 CpuUsage();
  virtual WebRtc_Word32 CpuUsage(WebRtc_Word8* process_name,
                                 WebRtc_UWord32 length) {
    return -1;
  }
  virtual WebRtc_Word32 CpuUsage(WebRtc_UWord32 process_id) {
    return -1;
  }

  
  
  
  virtual WebRtc_Word32 CpuUsageMultiCore(WebRtc_UWord32& num_cores,
                                          WebRtc_UWord32*& array);

  virtual void Reset() {}
  virtual void Stop() {}

 private:
  WebRtc_Word32 Update(WebRtc_Word64 time_diffMS);

  WebRtc_UWord32  cpu_count_;
  WebRtc_UWord32* cpu_usage_;
  WebRtc_Word32   total_cpu_usage_;
  WebRtc_Word64*  last_tick_count_;
  WebRtc_Word64   last_time_;
};

} 

#endif  
