









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_CAPTURER_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_CAPTURER_H_

#include <stddef.h>

#include "webrtc/modules/desktop_capture/desktop_capture_types.h"

namespace webrtc {

class DesktopFrame;
class DesktopRegion;
class SharedMemory;


class DesktopCapturer {
 public:
  
  class Callback {
   public:
    
    
    
    virtual SharedMemory* CreateSharedMemory(size_t size) = 0;

    
    
    
    virtual void OnCaptureCompleted(DesktopFrame* frame) = 0;

   protected:
    virtual ~Callback() {}
  };

  virtual ~DesktopCapturer() {}

  
  
  virtual void Start(Callback* callback) = 0;

  
  
  
  
  
  
  virtual void Capture(const DesktopRegion& region) = 0;

  
  
  
  virtual void SetExcludedWindow(WindowId window) {}
};

}  

#endif  

