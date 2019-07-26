









#include "webrtc/system_wrappers/interface/event_wrapper.h"

#if defined(_WIN32)
#include <windows.h>
#include "webrtc/system_wrappers/source/event_win.h"
#elif defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
#include <ApplicationServices/ApplicationServices.h>
#include <pthread.h>
#include "webrtc/system_wrappers/source/event_posix.h"
#else
#include <pthread.h>
#include "webrtc/system_wrappers/source/event_posix.h"
#endif

namespace webrtc {
EventWrapper* EventWrapper::Create() {
#if defined(_WIN32)
  return new EventWindows();
#else
  return EventPosix::Create();
#endif
}
}  
