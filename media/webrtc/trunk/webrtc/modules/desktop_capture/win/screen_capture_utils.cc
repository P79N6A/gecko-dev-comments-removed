









#include "webrtc/modules/desktop_capture/win/screen_capture_utils.h"

#include <assert.h>
#include <windows.h>

namespace webrtc {

bool GetScreenList(ScreenCapturer::ScreenList* screens) {
  assert(screens->size() == 0);

  BOOL enum_result = TRUE;
  for (int device_index = 0;; ++device_index) {
    DISPLAY_DEVICE device;
    device.cb = sizeof(device);
    enum_result = EnumDisplayDevices(NULL, device_index, &device, 0);

    
    if (!enum_result)
      break;

    
    if (!(device.StateFlags & DISPLAY_DEVICE_ACTIVE))
      continue;

    ScreenCapturer::Screen screen;
    screen.id = device_index;
    screens->push_back(screen);
  }
  return true;
}

bool IsScreenValid(ScreenId screen, std::wstring* device_key) {
  if (screen == kFullDesktopScreenId) {
    *device_key = L"";
    return true;
  }

  DISPLAY_DEVICE device;
  device.cb = sizeof(device);
  BOOL enum_result = EnumDisplayDevices(NULL, screen, &device, 0);
  if (enum_result)
    *device_key = device.DeviceKey;

  return !!enum_result;
}

DesktopRect GetScreenRect(ScreenId screen, const std::wstring& device_key) {
  if (screen == kFullDesktopScreenId) {
    return DesktopRect::MakeXYWH(GetSystemMetrics(SM_XVIRTUALSCREEN),
                                 GetSystemMetrics(SM_YVIRTUALSCREEN),
                                 GetSystemMetrics(SM_CXVIRTUALSCREEN),
                                 GetSystemMetrics(SM_CYVIRTUALSCREEN));
  }

  DISPLAY_DEVICE device;
  device.cb = sizeof(device);
  BOOL result = EnumDisplayDevices(NULL, screen, &device, 0);
  if (!result)
    return DesktopRect();

  
  
  
  
  if (device_key != device.DeviceKey)
    return DesktopRect();

  DEVMODE device_mode;
  device_mode.dmSize = sizeof(device_mode);
  device_mode.dmDriverExtra = 0;
  result = EnumDisplaySettingsEx(
      device.DeviceName, ENUM_CURRENT_SETTINGS, &device_mode, 0);
  if (!result)
    return DesktopRect();

  return DesktopRect::MakeXYWH(device_mode.dmPosition.x,
                               device_mode.dmPosition.y,
                               device_mode.dmPelsWidth,
                               device_mode.dmPelsHeight);
}

}  
