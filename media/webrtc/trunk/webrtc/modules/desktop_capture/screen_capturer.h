









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_SCREEN_CAPTURER_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_SCREEN_CAPTURER_H_

#include "webrtc/modules/desktop_capture/desktop_capturer.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

struct MouseCursorShape;



















class ScreenCapturer : public DesktopCapturer {
 public:
  
  
  
  
  
  class MouseShapeObserver {
   public:
    
    
    virtual void OnCursorShapeChanged(MouseCursorShape* cursor_shape) = 0;

   protected:
    virtual ~MouseShapeObserver() {}
  };

  virtual ~ScreenCapturer() {}

  
  static ScreenCapturer* Create();

#if defined(WEBRTC_LINUX)
  
  
  static ScreenCapturer* CreateWithXDamage(bool use_x_damage);
#elif defined(WEBRTC_WIN)
  
  
  static ScreenCapturer* CreateWithDisableAero(bool disable_aero);
#endif  

  
  
  virtual void SetMouseShapeObserver(
      MouseShapeObserver* mouse_shape_observer) = 0;
};

}  

#endif  
