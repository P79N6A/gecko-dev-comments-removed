









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CPU_INFO_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CPU_INFO_H_

#include "typedefs.h"

namespace webrtc {

class CpuInfo {
 public:
  static WebRtc_UWord32 DetectNumberOfCores();

 private:
  CpuInfo() {}
  static WebRtc_UWord32 number_of_cores_;
};

} 

#endif 
