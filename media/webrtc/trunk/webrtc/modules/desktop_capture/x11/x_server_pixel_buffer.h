











#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_X11_X_SERVER_PIXEL_BUFFER_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_X11_X_SERVER_PIXEL_BUFFER_H_

#include "webrtc/modules/desktop_capture/desktop_geometry.h"

#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

namespace webrtc {

class DesktopFrame;



class XServerPixelBuffer {
 public:
  XServerPixelBuffer();
  ~XServerPixelBuffer();

  void Release();

  
  
  bool Init(Display* display, Window window);

  bool is_initialized() { return window_ != 0; }

  
  const DesktopSize& window_size() { return window_size_; }

  
  
  
  
  
  void Synchronize();

  
  
  
  
  void CaptureRect(const DesktopRect& rect, DesktopFrame* frame);

 private:
  void InitShm(const XWindowAttributes& attributes);
  bool InitPixmaps(int depth);

  
  
  void FastBlit(uint8_t* image,
                const DesktopRect& rect,
                DesktopFrame* frame);
  void SlowBlit(uint8_t* image,
                const DesktopRect& rect,
                DesktopFrame* frame);

  Display* display_;
  Window window_;
  DesktopSize window_size_;
  XImage* x_image_;
  XShmSegmentInfo* shm_segment_info_;
  Pixmap shm_pixmap_;
  GC shm_gc_;

  DISALLOW_COPY_AND_ASSIGN(XServerPixelBuffer);
};

}  

#endif  
