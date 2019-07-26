









#include "webrtc/modules/desktop_capture/screen_capturer.h"

#include "webrtc/modules/desktop_capture/desktop_capture_options.h"

namespace webrtc {

ScreenCapturer* ScreenCapturer::Create() {
  return Create(DesktopCaptureOptions::CreateDefault());
}

#if defined(WEBRTC_LINUX)
ScreenCapturer* ScreenCapturer::CreateWithXDamage(
    bool use_update_notifications) {
  DesktopCaptureOptions options;
  options.set_use_update_notifications(use_update_notifications);
  return Create(options);
}
#elif defined(WEBRTC_WIN)
ScreenCapturer* ScreenCapturer::CreateWithDisableAero(bool disable_effects) {
  DesktopCaptureOptions options;
  options.set_disable_effects(disable_effects);
  return Create(options);
}
#endif

}  
