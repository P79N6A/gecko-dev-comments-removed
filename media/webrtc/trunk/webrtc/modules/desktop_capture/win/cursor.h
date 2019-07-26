









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_CURSOR_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_CURSOR_H_

#include <windows.h>

#include "webrtc/modules/desktop_capture/mouse_cursor_shape.h"

namespace webrtc {


MouseCursorShape* CreateMouseCursorShapeFromCursor(
    HDC dc, HCURSOR cursor);

}  

#endif  
