



#include "webrtc/modules/desktop_capture/x11/desktop_device_info_x11.h"
#include "webrtc/modules/desktop_capture/window_capturer.h"

namespace webrtc{

DesktopDeviceInfo * DesktopDeviceInfoImpl::Create() {
  DesktopDeviceInfoX11 * pDesktopDeviceInfo = new DesktopDeviceInfoX11();
  if (pDesktopDeviceInfo && pDesktopDeviceInfo->Init() != 0){
    delete pDesktopDeviceInfo;
    pDesktopDeviceInfo = NULL;
  }
  return pDesktopDeviceInfo;
}

DesktopDeviceInfoX11::DesktopDeviceInfoX11() {
}

DesktopDeviceInfoX11::~DesktopDeviceInfoX11() {
}

int32_t DesktopDeviceInfoX11::Init() {
#if !defined(MULTI_MONITOR_SCREENSHARE)
  DesktopDisplayDevice *pDesktopDeviceInfo = new DesktopDisplayDevice;
  if(pDesktopDeviceInfo){
    pDesktopDeviceInfo->setScreenId(0);
    pDesktopDeviceInfo->setDeviceName("Primary Monitor");
    pDesktopDeviceInfo->setUniqueIdName("\\screen\\monitor#1");

    desktop_display_list_[pDesktopDeviceInfo->getScreenId()] = pDesktopDeviceInfo;
  }
#endif
  return 0;
}

} 
