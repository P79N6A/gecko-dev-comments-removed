









#include "webrtc/modules/desktop_capture/mouse_cursor.h"

#include "webrtc/modules/desktop_capture/desktop_frame.h"

namespace webrtc {

MouseCursor::MouseCursor() {}

MouseCursor::MouseCursor(DesktopFrame* image, const DesktopVector& hotspot)
    : image_(image),
      hotspot_(hotspot) {
  assert(0 <= hotspot_.x() && hotspot_.x() <= image_->size().width());
  assert(0 <= hotspot_.y() && hotspot_.y() <= image_->size().height());
}

MouseCursor::~MouseCursor() {}


MouseCursor* MouseCursor::CopyOf(const MouseCursor& cursor) {
  return cursor.image()
             ? new MouseCursor(BasicDesktopFrame::CopyOf(*cursor.image()),
                               cursor.hotspot())
             : new MouseCursor();
}

}  
