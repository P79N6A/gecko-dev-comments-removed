









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_MAC_FULL_SCREEN_CHROME_WINDOW_DETECTOR_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_MAC_FULL_SCREEN_CHROME_WINDOW_DETECTOR_H_

#include <ApplicationServices/ApplicationServices.h>

#include "webrtc/modules/desktop_capture/window_capturer.h"
#include "webrtc/system_wrappers/interface/atomic32.h"
#include "webrtc/system_wrappers/interface/tick_util.h"

namespace webrtc {












class FullScreenChromeWindowDetector {
 public:
  FullScreenChromeWindowDetector();

  void AddRef() { ++ref_count_; }
  void Release() {
    if (--ref_count_ == 0)
      delete this;
  }

  
  
  CGWindowID FindFullScreenWindow(CGWindowID original_window);

  
  
  void UpdateWindowListIfNeeded(CGWindowID original_window);

 private:
  ~FullScreenChromeWindowDetector();

  Atomic32 ref_count_;

  
  
  
  
  WindowCapturer::WindowList current_window_list_;
  WindowCapturer::WindowList previous_window_list_;
  TickTime last_udpate_time_;

  DISALLOW_COPY_AND_ASSIGN(FullScreenChromeWindowDetector);
};

}  

#endif  
