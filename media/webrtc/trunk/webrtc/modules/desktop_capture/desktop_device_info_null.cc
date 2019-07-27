



#include "webrtc/modules/desktop_capture/desktop_device_info.h"

namespace webrtc {

class DesktopDeviceInfoNull : public DesktopDeviceInfoImpl {
public:
  DesktopDeviceInfoNull();
  ~DesktopDeviceInfoNull();

protected:
  virtual void InitializeScreenList();
  virtual void InitializeApplicationList();
};

DesktopDeviceInfo * DesktopDeviceInfoImpl::Create() {
  DesktopDeviceInfoNull * pDesktopDeviceInfo = new DesktopDeviceInfoNull();
  if (pDesktopDeviceInfo && pDesktopDeviceInfo->Init() != 0) {
    delete pDesktopDeviceInfo;
    pDesktopDeviceInfo = NULL;
  }
  return pDesktopDeviceInfo;
}

DesktopDeviceInfoNull::DesktopDeviceInfoNull() {
}

DesktopDeviceInfoNull::~DesktopDeviceInfoNull() {
}

void
DesktopDeviceInfoNull::InitializeScreenList() {
}

void
DesktopDeviceInfoNull::InitializeApplicationList() {
}

} 
