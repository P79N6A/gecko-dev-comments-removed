









#include <windows.h>

#include "webrtc/modules/desktop_capture/desktop_geometry.h"

namespace webrtc {





bool GetCroppedWindowRect(HWND window,
                          DesktopRect* cropped_rect,
                          DesktopRect* original_rect);

}  
