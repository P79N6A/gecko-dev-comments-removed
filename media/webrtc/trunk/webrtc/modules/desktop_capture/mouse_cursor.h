









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_MOUSE_CURSOR_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_MOUSE_CURSOR_H_

#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/desktop_capture/desktop_geometry.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class DesktopFrame;

class MouseCursor {
 public:
  MouseCursor();

  
  MouseCursor(DesktopFrame* image, const DesktopVector& hotspot);

  ~MouseCursor();

  static MouseCursor* CopyOf(const MouseCursor& cursor);

  void set_image(DesktopFrame* image) { image_.reset(image); }
  const DesktopFrame* image() const { return image_.get(); }

  void set_hotspot(const DesktopVector& hotspot ) { hotspot_ = hotspot; }
  const DesktopVector& hotspot() const { return hotspot_; }

 private:
  scoped_ptr<DesktopFrame> image_;
  DesktopVector hotspot_;

  DISALLOW_COPY_AND_ASSIGN(MouseCursor);
};

}  

#endif  
