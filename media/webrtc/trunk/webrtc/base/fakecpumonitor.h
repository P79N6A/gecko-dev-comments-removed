









#ifndef WEBRTC_BASE_FAKECPUMONITOR_H_
#define WEBRTC_BASE_FAKECPUMONITOR_H_

#include "webrtc/base/cpumonitor.h"

namespace rtc {

class FakeCpuMonitor : public rtc::CpuMonitor {
 public:
  explicit FakeCpuMonitor(Thread* thread)
      : CpuMonitor(thread) {
  }
  ~FakeCpuMonitor() {
  }

  virtual void OnMessage(rtc::Message* msg) {
  }
};

}  

#endif  
