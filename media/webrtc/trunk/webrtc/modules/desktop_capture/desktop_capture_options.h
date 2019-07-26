








#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_CAPTURE_OPTIONS_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_CAPTURE_OPTIONS_H_

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"

#if defined(USE_X11)
#include "webrtc/modules/desktop_capture/x11/shared_x_display.h"
#endif

#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
#include "webrtc/modules/desktop_capture/mac/desktop_configuration_monitor.h"
#endif

namespace webrtc {



class DesktopCaptureOptions {
 public:
  
  DesktopCaptureOptions();
  ~DesktopCaptureOptions();

  
  
  
  static DesktopCaptureOptions CreateDefault();

#if defined(USE_X11)
  SharedXDisplay* x_display() const { return x_display_; }
  void set_x_display(scoped_refptr<SharedXDisplay> x_display) {
    x_display_ = x_display;
  }
#endif

#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
  DesktopConfigurationMonitor* configuration_monitor() const {
    return configuration_monitor_;
  }
  void set_configuration_monitor(scoped_refptr<DesktopConfigurationMonitor> m) {
    configuration_monitor_ = m;
  }
#endif

  
  
  bool use_update_notifications() const { return use_update_notifications_; }
  void set_use_update_notifications(bool use_update_notifications) {
    use_update_notifications_ = use_update_notifications;
  }

  
  
  bool disable_effects() const { return disable_effects_; }
  void set_disable_effects(bool disable_effects) {
    disable_effects_ = disable_effects;
  }

 private:
#if defined(USE_X11)
  scoped_refptr<SharedXDisplay> x_display_;
#endif

#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
  scoped_refptr<DesktopConfigurationMonitor> configuration_monitor_;
#endif
  bool use_update_notifications_;
  bool disable_effects_;
};

}  

#endif  
