









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_SCREEN_CAPTURE_UTILS_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_SCREEN_CAPTURE_UTILS_H_

#include "webrtc/modules/desktop_capture/screen_capturer.h"

namespace webrtc {



bool GetScreenList(ScreenCapturer::ScreenList* screens);





bool IsScreenValid(ScreenId screen, std::wstring* device_key);




DesktopRect GetScreenRect(ScreenId screen, const std::wstring& device_key);

}  

#endif  
