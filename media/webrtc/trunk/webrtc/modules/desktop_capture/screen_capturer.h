









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_SCREEN_CAPTURER_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_SCREEN_CAPTURER_H_

#include <vector>

#include "webrtc/modules/desktop_capture/desktop_capture_types.h"
#include "webrtc/modules/desktop_capture/desktop_capturer.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class DesktopCaptureOptions;
struct MouseCursorShape;



















class ScreenCapturer : public DesktopCapturer {
 public:
  
  
  struct Screen {
    ScreenId id;
  };
  typedef std::vector<Screen> ScreenList;

  
  
  
  
  
  class MouseShapeObserver {
   public:
    
    
    virtual void OnCursorShapeChanged(MouseCursorShape* cursor_shape) = 0;

   protected:
    virtual ~MouseShapeObserver() {}
  };

  virtual ~ScreenCapturer() {}

  
  
  
  
  static ScreenCapturer* Create(const DesktopCaptureOptions& options);
  static ScreenCapturer* Create();

#if defined(WEBRTC_LINUX)
  
  
  static ScreenCapturer* CreateWithXDamage(bool use_x_damage);
#elif defined(WEBRTC_WIN)
  
  
  static ScreenCapturer* CreateWithDisableAero(bool disable_aero);
#endif  

  
  
  virtual void SetMouseShapeObserver(
      MouseShapeObserver* mouse_shape_observer) = 0;

  
  
  virtual bool GetScreenList(ScreenList* screens) = 0;

  
  
  
  virtual bool SelectScreen(ScreenId id) = 0;
};

}  

#endif  
