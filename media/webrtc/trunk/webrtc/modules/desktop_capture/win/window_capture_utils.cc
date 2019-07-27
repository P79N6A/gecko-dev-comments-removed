









#include "webrtc/modules/desktop_capture/win/window_capture_utils.h"

namespace webrtc {

bool
GetCroppedWindowRect(HWND window,
                     DesktopRect* cropped_rect,
                     DesktopRect* original_rect) {
  RECT rect;
  if (!GetWindowRect(window, &rect)) {
    return false;
  }
  WINDOWPLACEMENT window_placement;
  window_placement.length = sizeof(window_placement);
  if (!GetWindowPlacement(window, &window_placement)) {
    return false;
  }

  *original_rect = DesktopRect::MakeLTRB(
      rect.left, rect.top, rect.right, rect.bottom);

  if (window_placement.showCmd & SW_SHOWMAXIMIZED) {
    DesktopSize border = DesktopSize(GetSystemMetrics(SM_CXSIZEFRAME),
                                     GetSystemMetrics(SM_CYSIZEFRAME));
    *cropped_rect = DesktopRect::MakeLTRB(
        rect.left + border.width(),
        rect.top,
        rect.right - border.width(),
        rect.bottom - border.height());
  } else {
    *cropped_rect = *original_rect;
  }
  return true;
}

}  
