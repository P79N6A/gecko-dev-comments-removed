









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_MOUSE_CURSOR_MONITOR_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_MOUSE_CURSOR_MONITOR_H_

#include "webrtc/modules/desktop_capture/desktop_capture_types.h"
#include "webrtc/modules/desktop_capture/desktop_geometry.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class DesktopCaptureOptions;
class DesktopFrame;
class MouseCursor;


class MouseCursorMonitor {
 public:
  enum CursorState {
    
    INSIDE,

    
    OUTSIDE,
  };

  enum Mode {
    
    SHAPE_ONLY,

    
    SHAPE_AND_POSITION,
  };

  
  class Callback {
   public:
    
    
    virtual void OnMouseCursor(MouseCursor* cursor) = 0;

    
    
    virtual void OnMouseCursorPosition(CursorState state,
                                       const DesktopVector& position) = 0;

   protected:
    virtual ~Callback() {}
  };

  virtual ~MouseCursorMonitor() {}

  
  
  static MouseCursorMonitor* CreateForWindow(
      const DesktopCaptureOptions& options,
      WindowId window);

  
  
  
  
  static MouseCursorMonitor* CreateForScreen(
      const DesktopCaptureOptions& options,
      ScreenId screen);

  
  
  virtual void Init(Callback* callback, Mode mode) = 0;

  
  
  
  
  
  virtual void Capture() = 0;
};

}  

#endif

