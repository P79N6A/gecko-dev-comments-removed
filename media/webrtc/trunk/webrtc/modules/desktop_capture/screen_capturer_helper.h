









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_SCREEN_CAPTURER_HELPER_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_SCREEN_CAPTURER_HELPER_H_

#include "webrtc/modules/desktop_capture/desktop_geometry.h"
#include "webrtc/modules/desktop_capture/desktop_region.h"
#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {





class ScreenCapturerHelper {
 public:
  ScreenCapturerHelper();
  ~ScreenCapturerHelper();

  
  void ClearInvalidRegion();

  
  void InvalidateRegion(const DesktopRegion& invalid_region);

  
  void InvalidateScreen(const DesktopSize& size);

  
  
  void TakeInvalidRegion(DesktopRegion* invalid_region);

  
  const DesktopSize& size_most_recent() const;
  void set_size_most_recent(const DesktopSize& size);

  
  
  
  
  
  
  
  
  
  
  
  void SetLogGridSize(int log_grid_size);

  
  
  static void ExpandToGrid(const DesktopRegion& region,
                           int log_grid_size,
                           DesktopRegion* result);

 private:
  
  
  
  DesktopRegion invalid_region_;

  
  scoped_ptr<RWLockWrapper> invalid_region_lock_;

  
  DesktopSize size_most_recent_;

  
  
  
  int log_grid_size_;

  DISALLOW_COPY_AND_ASSIGN(ScreenCapturerHelper);
};

}  

#endif  
