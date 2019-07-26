









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_WINDOW_CAPTURER_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_WINDOW_CAPTURER_H_

#include <vector>
#include <string>

#include "webrtc/modules/desktop_capture/desktop_capture_types.h"
#include "webrtc/modules/desktop_capture/desktop_capturer.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class DesktopCaptureOptions;

class WindowCapturer : public DesktopCapturer {
 public:
  typedef webrtc::WindowId WindowId;

  struct Window {
    WindowId id;

    
    std::string title;
  };

  typedef std::vector<Window> WindowList;

  static WindowCapturer* Create(const DesktopCaptureOptions& options);

  
  static WindowCapturer* Create();

  virtual ~WindowCapturer() {}

  
  virtual bool GetWindowList(WindowList* windows) = 0;

  
  
  virtual bool SelectWindow(WindowId id) = 0;
};

}  

#endif  

