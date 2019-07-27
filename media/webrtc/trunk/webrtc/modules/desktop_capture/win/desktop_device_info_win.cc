



#include "webrtc/modules/desktop_capture/win/desktop_device_info_win.h"

namespace webrtc{

DesktopDeviceInfo * DesktopDeviceInfoImpl::Create() {
  DesktopDeviceInfoWin * pDesktopDeviceInfo = new DesktopDeviceInfoWin();
  if(pDesktopDeviceInfo && pDesktopDeviceInfo->Init() != 0){
    delete pDesktopDeviceInfo;
    pDesktopDeviceInfo = NULL;
  }
  return pDesktopDeviceInfo;
}

DesktopDeviceInfoWin::DesktopDeviceInfoWin() {
}

DesktopDeviceInfoWin::~DesktopDeviceInfoWin() {
}

int32_t DesktopDeviceInfoWin::Init() {
#if !defined(MULTI_MONITOR_SCREENSHARE)
  DesktopDisplayDevice *pDesktopDeviceInfo = new DesktopDisplayDevice;
  if (pDesktopDeviceInfo) {
    pDesktopDeviceInfo->setScreenId(0);
    pDesktopDeviceInfo->setDeviceName("Primary Monitor");
    pDesktopDeviceInfo->setUniqueIdName("\\screen\\monitor#1");

    desktop_display_list_[pDesktopDeviceInfo->getScreenId()] = pDesktopDeviceInfo;
  }
#endif
  return 0;
}

} 
