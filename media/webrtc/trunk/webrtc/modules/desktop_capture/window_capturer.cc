  









#include "webrtc/modules/desktop_capture/window_capturer.h"

#include "webrtc/modules/desktop_capture/desktop_capture_options.h"

namespace webrtc {


WindowCapturer* WindowCapturer::Create() {
  return Create(DesktopCaptureOptions::CreateDefault());
}

}  
