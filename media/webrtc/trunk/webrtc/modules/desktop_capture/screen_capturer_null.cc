









#include "webrtc/modules/desktop_capture/screen_capturer.h"

namespace webrtc {


ScreenCapturer* ScreenCapturer::Create() {
  return NULL;
}

#if defined(OS_LINUX)

ScreenCapturer* ScreenCapturer::CreateWithXDamage(bool use_x_damage) {
  return NULL;
}
#elif defined(OS_WIN)

ScreenCapturer* ScreenCapturer::CreateWithDisableAero(bool disable_aero) {
  return NULL;
}
#endif  

}  
