









#include "webrtc/modules/desktop_capture/mouse_cursor_monitor.h"

#include <stddef.h>

namespace webrtc {

MouseCursorMonitor* MouseCursorMonitor::CreateForWindow(
    const DesktopCaptureOptions& options,
    WindowId window) {
  return NULL;
}

MouseCursorMonitor* MouseCursorMonitor::CreateForScreen(
    const DesktopCaptureOptions& options,
    ScreenId screen) {
  return NULL;
}

}  
