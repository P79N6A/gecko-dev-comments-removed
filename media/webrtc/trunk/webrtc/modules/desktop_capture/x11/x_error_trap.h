









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_X11_X_ERROR_TRAP_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_X11_X_ERROR_TRAP_H_

#include <X11/Xlib.h>

#include "webrtc/system_wrappers/interface/constructor_magic.h"

namespace webrtc {



class XErrorTrap {
 public:
  explicit XErrorTrap(Display* display);
  ~XErrorTrap();

  
  int GetLastErrorAndDisable();

 private:
  XErrorHandler original_error_handler_;
  bool enabled_;

  DISALLOW_COPY_AND_ASSIGN(XErrorTrap);
};

}  

#endif  
