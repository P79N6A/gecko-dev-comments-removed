



#include "base/gfx/point.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

namespace gfx {

Point::Point() : x_(0), y_(0) {
}

Point::Point(int x, int y) : x_(x), y_(y) {
}

#if defined(OS_WIN)
Point::Point(const POINT& point) : x_(point.x), y_(point.y) {
}

Point& Point::operator=(const POINT& point) {
  x_ = point.x;
  y_ = point.y;
  return *this;
}

POINT Point::ToPOINT() const {
  POINT p;
  p.x = x_;
  p.y = y_;
  return p;
}
#elif defined(OS_MACOSX)
Point::Point(const CGPoint& point) : x_(point.x), y_(point.y) {
}

CGPoint Point::ToCGPoint() const {
  return CGPointMake(x_, y_);
}
#endif

}  
