









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_FRAME_WIN_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_FRAME_WIN_H_

#include <windows.h>

#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {



class DesktopFrameWin : public DesktopFrame {
 public:
  virtual ~DesktopFrameWin();
  static DesktopFrameWin* Create(DesktopSize size,
                                 SharedMemory* shared_memory,
                                 HDC hdc);

  HBITMAP bitmap() { return bitmap_; }

 private:
  DesktopFrameWin(DesktopSize size,
                  int stride,
                  uint8_t* data,
                  SharedMemory* shared_memory,
                  HBITMAP bitmap);

  HBITMAP bitmap_;
  scoped_ptr<SharedMemory> owned_shared_memory_;

  DISALLOW_COPY_AND_ASSIGN(DesktopFrameWin);
};

}  

#endif  

