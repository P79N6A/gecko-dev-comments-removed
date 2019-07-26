









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_SHARED_DESKTOP_FRAME_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_SHARED_DESKTOP_FRAME_H_

#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"

namespace webrtc {



class SharedDesktopFrame : public DesktopFrame {
 public:
  virtual ~SharedDesktopFrame();

  static SharedDesktopFrame* Wrap(DesktopFrame* desktop_frame);

  
  DesktopFrame* GetUnderlyingFrame();

  
  SharedDesktopFrame* Share();

  
  
  bool IsShared();

 private:
  class Core;

  SharedDesktopFrame(scoped_refptr<Core> core);

  scoped_refptr<Core> core_;

  DISALLOW_COPY_AND_ASSIGN(SharedDesktopFrame);
};

}  

#endif  
