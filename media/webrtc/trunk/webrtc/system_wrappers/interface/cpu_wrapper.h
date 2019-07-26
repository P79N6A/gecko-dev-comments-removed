









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CPU_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CPU_WRAPPER_H_

#include "typedefs.h"

namespace webrtc {

class CpuWrapper {
 public:
  static CpuWrapper* CreateCpu();
  virtual ~CpuWrapper() {}

  
  
  virtual WebRtc_Word32 CpuUsage() = 0;
  virtual WebRtc_Word32 CpuUsage(WebRtc_Word8* process_name,
                                 WebRtc_UWord32 length) = 0;
  virtual WebRtc_Word32 CpuUsage(WebRtc_UWord32 process_id) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 CpuUsageMultiCore(WebRtc_UWord32& num_cores,
                                          WebRtc_UWord32*& cpu_usage) = 0;

  virtual void Reset() = 0;
  virtual void Stop() = 0;

 protected:
  CpuWrapper() {}
};

} 

#endif 
