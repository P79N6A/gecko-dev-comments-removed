









#include "system_wrappers/interface/cpu_wrapper.h"

#if defined(_WIN32)
#include "cpu_win.h"
#elif defined(WEBRTC_MAC)
#include "cpu_mac.h"
#elif defined(WEBRTC_ANDROID)

#else 
#include "cpu_linux.h"
#endif

namespace webrtc {
CpuWrapper* CpuWrapper::CreateCpu() {
#if defined(_WIN32)
  return new CpuWindows();
#elif defined(WEBRTC_MAC)
  return new CpuWrapperMac();
#elif defined(WEBRTC_ANDROID)
  return 0;
#else
  return new CpuLinux();
#endif
}

}  
