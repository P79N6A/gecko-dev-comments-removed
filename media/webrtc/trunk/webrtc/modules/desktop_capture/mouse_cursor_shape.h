









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_MOUSE_CURSOR_SHAPE_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_MOUSE_CURSOR_SHAPE_H_

#include <string>

#include "webrtc/modules/desktop_capture/desktop_geometry.h"

namespace webrtc {




struct MouseCursorShape {
  
  DesktopSize size;

  
  DesktopVector hotspot;

  
  std::string data;
};

}  

#endif  
