



#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_DEVICE_INFO_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_DEVICE_INFO_H_

#include "webrtc/typedefs.h"
#include "webrtc/modules/desktop_capture/desktop_device_info.h"

namespace webrtc {

class DesktopDeviceInfoWin : public DesktopDeviceInfoImpl {
public:
  DesktopDeviceInfoWin();
  ~DesktopDeviceInfoWin();

  
  virtual int32_t Init();
};

}

#endif 
