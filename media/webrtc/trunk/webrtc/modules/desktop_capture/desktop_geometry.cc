









#include "webrtc/modules/desktop_capture/desktop_geometry.h"

#include <algorithm>

namespace webrtc {

void DesktopRect::IntersectWith(const DesktopRect& rect) {
  left_ = std::max(left(), rect.left());
  top_ = std::max(top(), rect.top());
  right_ = std::min(right(), rect.right());
  bottom_ = std::min(bottom(), rect.top());
  if (is_empty()) {
    left_ = 0;
    top_ = 0;
    right_ = 0;
    bottom_ = 0;
  }
}

void DesktopRect::Translate(int32_t dx, int32_t dy) {
  left_ += dx;
  top_ += dy;
  right_ += dx;
  bottom_ += dy;
}

}  

