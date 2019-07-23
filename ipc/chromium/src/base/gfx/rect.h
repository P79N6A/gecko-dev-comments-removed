










#ifndef BASE_GFX_RECT_H__
#define BASE_GFX_RECT_H__

#include <iostream>

#include "base/gfx/point.h"
#include "base/gfx/size.h"

#if defined(OS_WIN)
typedef struct tagRECT RECT;
#elif defined(OS_LINUX)
typedef struct _GdkRectangle GdkRectangle;
#endif

namespace gfx {

class Rect {
 public:
  Rect();
  Rect(int width, int height);
  Rect(int x, int y, int width, int height);
#if defined(OS_WIN)
  explicit Rect(const RECT& r);
#elif defined(OS_MACOSX)
  explicit Rect(const CGRect& r);
#elif defined(OS_LINUX)
  explicit Rect(const GdkRectangle& r);
#endif
  Rect(const gfx::Point& origin, const gfx::Size& size);

  ~Rect() {}

#if defined(OS_WIN)
  Rect& operator=(const RECT& r);
#elif defined(OS_MACOSX)
  Rect& operator=(const CGRect& r);
#elif defined(OS_LINUX)
  Rect& operator=(const GdkRectangle& r);
#endif

  int x() const { return origin_.x(); }
  void set_x(int x) { origin_.set_x(x); }

  int y() const { return origin_.y(); }
  void set_y(int y) { origin_.set_y(y); }

  int width() const { return size_.width(); }
  void set_width(int width);

  int height() const { return size_.height(); }
  void set_height(int height);

  const gfx::Point& origin() const { return origin_; }
  void set_origin(const gfx::Point& origin) { origin_ = origin; }

  const gfx::Size& size() const { return size_; }

  int right() const { return x() + width(); }
  int bottom() const { return y() + height(); }

  void SetRect(int x, int y, int width, int height);

  
  void Inset(int horizontal, int vertical) {
    Inset(horizontal, vertical, horizontal, vertical);
  }

  
  void Inset(int left, int top, int right, int bottom);

  
  void Offset(int horizontal, int vertical);
  void Offset(const gfx::Point& point) {
    Offset(point.x(), point.y());
  }

  
  bool IsEmpty() const { return size_.IsEmpty(); }

  bool operator==(const Rect& other) const;

  bool operator!=(const Rect& other) const {
    return !(*this == other);
  }

#if defined(OS_WIN)
  
  RECT ToRECT() const;
#elif defined(OS_LINUX)
  GdkRectangle ToGdkRectangle() const;
#elif defined(OS_MACOSX)
  
  CGRect ToCGRect() const;
#endif

  
  
  
  bool Contains(int point_x, int point_y) const;

  
  bool Contains(const gfx::Point& point) const {
    return Contains(point.x(), point.y());
  }

  
  bool Contains(const Rect& rect) const;

  
  bool Intersects(const Rect& rect) const;

  
  Rect Intersect(const Rect& rect) const;

  
  
  Rect Union(const Rect& rect) const;

  
  
  
  
  Rect Subtract(const Rect& rect) const;

  
  bool Equals(const Rect& rect) const {
    return *this == rect;
  }

  
  
  
  
  
  Rect AdjustToFit(const Rect& rect) const;

  
  Point CenterPoint() const;

 private:
  gfx::Point origin_;
  gfx::Size size_;
};

}  

inline std::ostream& operator<<(std::ostream& out, const gfx::Rect& r) {
  return out << r.origin() << " " << r.size();
}

#endif  
