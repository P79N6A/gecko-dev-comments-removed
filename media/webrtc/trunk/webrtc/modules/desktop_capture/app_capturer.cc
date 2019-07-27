









#include "webrtc/modules/desktop_capture/app_capturer.h"
#include "webrtc/modules/desktop_capture/desktop_capture_options.h"

namespace webrtc {


AppCapturer* AppCapturer::Create() {
  return Create(DesktopCaptureOptions::CreateDefault());
}

}  
