









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_MAC_DESKTOP_CONFIGURATION_MONITOR_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_MAC_DESKTOP_CONFIGURATION_MONITOR_H_

#include <ApplicationServices/ApplicationServices.h>

#include <set>

#include "webrtc/modules/desktop_capture/mac/desktop_configuration.h"
#include "webrtc/system_wrappers/interface/atomic32.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class EventWrapper;



class DesktopConfigurationMonitor {
 public:
  DesktopConfigurationMonitor();
  
  void Lock();
  
  void Unlock();
  
  
  const MacDesktopConfiguration& desktop_configuration() {
    return desktop_configuration_;
  }

  void AddRef() { ++ref_count_; }
  void Release() {
    if (--ref_count_ == 0)
      delete this;
  }

 private:
  static void DisplaysReconfiguredCallback(CGDirectDisplayID display,
                                           CGDisplayChangeSummaryFlags flags,
                                           void *user_parameter);
  ~DesktopConfigurationMonitor();

  void DisplaysReconfigured(CGDirectDisplayID display,
                            CGDisplayChangeSummaryFlags flags);

  Atomic32 ref_count_;
  std::set<CGDirectDisplayID> reconfiguring_displays_;
  MacDesktopConfiguration desktop_configuration_;
  scoped_ptr<EventWrapper> display_configuration_capture_event_;

  DISALLOW_COPY_AND_ASSIGN(DesktopConfigurationMonitor);
};

}  

#endif  
