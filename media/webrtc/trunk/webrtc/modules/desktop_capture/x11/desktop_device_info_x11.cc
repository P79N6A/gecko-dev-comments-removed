



#include "webrtc/modules/desktop_capture/x11/desktop_device_info_x11.h"
#include "webrtc/modules/desktop_capture/window_capturer.h"
#include "webrtc/modules/desktop_capture/x11/x_error_trap.h"
#include "webrtc/modules/desktop_capture/x11/x_server_pixel_buffer.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"
#include "webrtc/modules/desktop_capture/x11/shared_x_util.h"
#include <unistd.h>
#include <stdio.h>

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

#if !defined(MULTI_MONITOR_SCREENSHARE)
void DesktopDeviceInfoX11::MultiMonitorScreenshare()
{
  DesktopDisplayDevice *pDesktopDeviceInfo = new DesktopDisplayDevice;
  if (pDesktopDeviceInfo) {
    pDesktopDeviceInfo->setScreenId(0);
    pDesktopDeviceInfo->setDeviceName("Primary Monitor");
    pDesktopDeviceInfo->setUniqueIdName("\\screen\\monitor#1");

    desktop_display_list_[pDesktopDeviceInfo->getScreenId()] = pDesktopDeviceInfo;
  }
}
#endif

void DesktopDeviceInfoX11::InitializeScreenList() {
#if !defined(MULTI_MONITOR_SCREENSHARE)
  MultiMonitorScreenshare();
#endif
}
void DesktopDeviceInfoX11::InitializeApplicationList() {
  
  scoped_refptr<SharedXDisplay> SharedDisplay = SharedXDisplay::CreateDefault();
  XErrorTrap error_trap(SharedDisplay->display());

  WindowUtilX11 window_util_x11(SharedDisplay);
  int num_screens = XScreenCount(SharedDisplay->display());
  for (int screen = 0; screen < num_screens; ++screen) {
    ::Window root_window = XRootWindow(SharedDisplay->display(), screen);
    ::Window parent;
    ::Window *children;
    unsigned int num_children;
    int status = XQueryTree(SharedDisplay->display(), root_window, &root_window, &parent,
        &children, &num_children);
    if (status == 0) {
      LOG(LS_ERROR) << "Failed to query for child windows for screen " << screen;
      continue;
    }

    for (unsigned int i = 0; i < num_children; ++i) {
      ::Window app_window = window_util_x11.GetApplicationWindow(children[num_children - 1 - i]);

      if (!app_window
          || window_util_x11.IsDesktopElement(app_window)
          || window_util_x11.GetWindowStatus(app_window) == WithdrawnState) {
        continue;
      }

      unsigned int processId = window_util_x11.GetWindowProcessID(app_window);
      
      if (processId == 0) {
        continue;
      }
      
      if (processId == getpid()) {
        continue;
      }

      
      DesktopApplication *pDesktopApplication = new DesktopApplication;
      if (!pDesktopApplication) {
        continue;
      }

      
      pDesktopApplication->setProcessId(processId);

      
      pDesktopApplication->setProcessPathName("");

      
      std::string strAppName;
      window_util_x11.GetWindowTitle(app_window, &strAppName);
      pDesktopApplication->setProcessAppName(strAppName.c_str());

      
      char idStr[64];
      snprintf(idStr, sizeof(idStr), "%ld", pDesktopApplication->getProcessId());
      pDesktopApplication->setUniqueIdName(idStr);
      desktop_application_list_[processId] = pDesktopApplication;
    }

    if (children) {
      XFree(children);
    }
  }
}

} 
