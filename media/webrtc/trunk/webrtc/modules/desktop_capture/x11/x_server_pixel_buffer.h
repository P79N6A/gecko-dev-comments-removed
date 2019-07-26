











#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_X11_X_SERVER_PIXEL_BUFFER_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_X11_X_SERVER_PIXEL_BUFFER_H_

#include "webrtc/modules/desktop_capture/desktop_geometry.h"

#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

namespace webrtc {



class XServerPixelBuffer {
 public:
  XServerPixelBuffer();
  ~XServerPixelBuffer();

  void Release();

  
  
  
  
  void Init(Display* display, const DesktopSize& screen_size);

  
  static DesktopSize GetRootWindowSize(Display* display);

  
  
  
  
  
  void Synchronize();

  
  
  
  
  
  
  
  uint8_t* CaptureRect(const DesktopRect& rect);

  
  
  int GetStride() const;
  int GetDepth() const;
  int GetBitsPerPixel() const;
  int GetRedMask() const;
  int GetBlueMask() const;
  int GetGreenMask() const;

 private:
  void InitShm(int screen);
  bool InitPixmaps(int depth);

  Display* display_;
  Window root_window_;
  DesktopSize root_window_size_;
  XImage* x_image_;
  XShmSegmentInfo* shm_segment_info_;
  Pixmap shm_pixmap_;
  GC shm_gc_;

  DISALLOW_COPY_AND_ASSIGN(XServerPixelBuffer);
};

}  

#endif  
