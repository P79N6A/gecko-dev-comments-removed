









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_MAC_DESKTOP_CONFIGURATION_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_MAC_DESKTOP_CONFIGURATION_H_

#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <vector>

#include "webrtc/typedefs.h"
#include "webrtc/modules/desktop_capture/desktop_geometry.h"

namespace webrtc {


struct MacDisplayConfiguration {
  MacDisplayConfiguration();

  
  CGDirectDisplayID id;

  
  DesktopRect bounds;

  
  DesktopRect pixel_bounds;

  
  float dip_to_pixel_scale;
};

typedef std::vector<MacDisplayConfiguration> MacDisplayConfigurations;


struct MacDesktopConfiguration {
  
  enum Origin { BottomLeftOrigin, TopLeftOrigin };

  MacDesktopConfiguration();
  ~MacDesktopConfiguration();

  
  
  
  static MacDesktopConfiguration GetCurrent(Origin origin);

  
  bool Equals(const MacDesktopConfiguration& other);

  
  const MacDisplayConfiguration* FindDisplayConfigurationById(
      CGDirectDisplayID id);

  
  DesktopRect bounds;

  
  DesktopRect pixel_bounds;

  
  float dip_to_pixel_scale;

  
  MacDisplayConfigurations displays;
};

}  

#endif  
