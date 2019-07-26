









#include "webrtc/modules/desktop_capture/mac/desktop_configuration_monitor.h"

#include "webrtc/modules/desktop_capture/mac/desktop_configuration.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/logging.h"

namespace webrtc {


static const int64_t kDisplayConfigurationEventTimeoutMs = 10 * 1000;

DesktopConfigurationMonitor::DesktopConfigurationMonitor()
    : ref_count_(0),
      display_configuration_capture_event_(EventWrapper::Create()) {
  CGError err = CGDisplayRegisterReconfigurationCallback(
      DesktopConfigurationMonitor::DisplaysReconfiguredCallback, this);
  if (err != kCGErrorSuccess) {
    LOG(LS_ERROR) << "CGDisplayRegisterReconfigurationCallback " << err;
    abort();
  }
  display_configuration_capture_event_->Set();

  desktop_configuration_ = MacDesktopConfiguration::GetCurrent(
      MacDesktopConfiguration::TopLeftOrigin);
}

DesktopConfigurationMonitor::~DesktopConfigurationMonitor() {
  CGError err = CGDisplayRemoveReconfigurationCallback(
      DesktopConfigurationMonitor::DisplaysReconfiguredCallback, this);
  if (err != kCGErrorSuccess)
    LOG(LS_ERROR) << "CGDisplayRemoveReconfigurationCallback " << err;
}

void DesktopConfigurationMonitor::Lock() {
  if (!display_configuration_capture_event_->Wait(
              kDisplayConfigurationEventTimeoutMs)) {
    LOG_F(LS_ERROR) << "Event wait timed out.";
    abort();
  }
}

void DesktopConfigurationMonitor::Unlock() {
  display_configuration_capture_event_->Set();
}


void DesktopConfigurationMonitor::DisplaysReconfiguredCallback(
    CGDirectDisplayID display,
    CGDisplayChangeSummaryFlags flags,
    void *user_parameter) {
  DesktopConfigurationMonitor* monitor =
      reinterpret_cast<DesktopConfigurationMonitor*>(user_parameter);
  monitor->DisplaysReconfigured(display, flags);
}

void DesktopConfigurationMonitor::DisplaysReconfigured(
    CGDirectDisplayID display,
    CGDisplayChangeSummaryFlags flags) {
  if (flags & kCGDisplayBeginConfigurationFlag) {
    if (reconfiguring_displays_.empty()) {
      
      
      
      if (!display_configuration_capture_event_->Wait(
              kDisplayConfigurationEventTimeoutMs)) {
        LOG_F(LS_ERROR) << "Event wait timed out.";
        abort();
      }
    }
    reconfiguring_displays_.insert(display);
  } else {
    reconfiguring_displays_.erase(display);
    if (reconfiguring_displays_.empty()) {
      desktop_configuration_ = MacDesktopConfiguration::GetCurrent(
          MacDesktopConfiguration::TopLeftOrigin);
      display_configuration_capture_event_->Set();
    }
  }
}

}  
