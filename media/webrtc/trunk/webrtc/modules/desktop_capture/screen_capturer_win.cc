









#include "webrtc/modules/desktop_capture/screen_capturer.h"

#include "webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "webrtc/modules/desktop_capture/win/screen_capturer_win_gdi.h"
#include "webrtc/modules/desktop_capture/win/screen_capturer_win_magnifier.h"

namespace webrtc {


ScreenCapturer* ScreenCapturer::Create(const DesktopCaptureOptions& options) {
  scoped_ptr<ScreenCapturer> gdi_capturer(new ScreenCapturerWinGdi(options));

  if (options.allow_use_magnification_api())
    return new ScreenCapturerWinMagnifier(gdi_capturer.Pass());

  return gdi_capturer.release();
}

}  
