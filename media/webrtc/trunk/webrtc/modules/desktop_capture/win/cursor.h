









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_CURSOR_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_CURSOR_H_

#include <windows.h>

namespace webrtc {

class MouseCursor;


MouseCursor* CreateMouseCursorFromHCursor(HDC dc, HCURSOR cursor);

}  

#endif  
