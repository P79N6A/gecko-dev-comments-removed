









#include "webrtc/modules/desktop_capture/desktop_capture_options.h"

namespace webrtc {

DesktopCaptureOptions::DesktopCaptureOptions()
    : use_update_notifications_(true),
      disable_effects_(true) {
#if defined(USE_X11)
  
  use_update_notifications_ = false;
#endif
}

DesktopCaptureOptions::~DesktopCaptureOptions() {}


DesktopCaptureOptions DesktopCaptureOptions::CreateDefault() {
  DesktopCaptureOptions result;
#if defined(USE_X11)
  result.set_x_display(SharedXDisplay::CreateDefault());
#endif
#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
  result.set_configuration_monitor(new DesktopConfigurationMonitor());
#endif
  return result;
}

}  
