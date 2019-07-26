









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_REGION_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_REGION_H_

#include <vector>

#include "webrtc/modules/desktop_capture/desktop_geometry.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {





class DesktopRegion {
 public:
  
  
  class Iterator {
   public:
    explicit Iterator(const DesktopRegion& target);

    bool IsAtEnd() const;
    void Advance();

    const DesktopRect& rect() const { return *it_; }

   private:
    const DesktopRegion& region_;
    std::vector<DesktopRect>::const_iterator it_;
  };

  DesktopRegion();
  DesktopRegion(const DesktopRegion& other);
  ~DesktopRegion();

  bool is_empty() const { return rects_.empty(); }

  void Clear();
  void SetRect(const DesktopRect& rect);
  void AddRect(const DesktopRect& rect);
  void AddRegion(const DesktopRegion& region);

  
  void IntersectWith(const DesktopRect& rect);

  
  void Translate(int32_t dx, int32_t dy);

  void Swap(DesktopRegion* region);

 private:
  typedef std::vector<DesktopRect> RectsList;
  RectsList rects_;
};

}  

#endif  

