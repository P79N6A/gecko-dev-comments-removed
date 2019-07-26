









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CPU_INFO_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CPU_INFO_H_

#include "webrtc/typedefs.h"

namespace webrtc {

class CpuInfo {
 public:
  static uint32_t DetectNumberOfCores();

 private:
  CpuInfo() {}
  static uint32_t number_of_cores_;
};

}  

#endif 
