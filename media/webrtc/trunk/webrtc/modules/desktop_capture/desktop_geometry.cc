









#include "webrtc/modules/desktop_capture/desktop_geometry.h"

#include <algorithm>

namespace webrtc {

bool DesktopRect::Contains(const DesktopVector& point) const {
  return point.x() >= left() && point.x() < right() &&
         point.y() >= top() && point.y() < bottom();
}

bool DesktopRect::ContainsRect(const DesktopRect& rect) const {
  return rect.left() >= left() && rect.right() <= right() &&
         rect.top() >= top() && rect.bottom() <= bottom();
}

void DesktopRect::IntersectWith(const DesktopRect& rect) {
  left_ = std::max(left(), rect.left());
  top_ = std::max(top(), rect.top());
  right_ = std::min(right(), rect.right());
  bottom_ = std::min(bottom(), rect.bottom());
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

