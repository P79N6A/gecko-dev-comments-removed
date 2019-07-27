









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_X11_X_ERROR_TRAP_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_X11_X_ERROR_TRAP_H_

#include <X11/Xlibint.h>
#undef max // Xlibint.h defines this and it breaks std::max
#undef min // Xlibint.h defines this and it breaks std::min

#include "webrtc/system_wrappers/interface/constructor_magic.h"

namespace webrtc {






class XErrorTrap {
 public:
  explicit XErrorTrap(Display* display);
  ~XErrorTrap();

  
  
  int GetLastErrorAndDisable();

 private:
  static Bool XServerErrorHandler(Display* display, xReply* rep,
                                  char* , int ,
                                  XPointer data);

  _XAsyncHandler async_handler_;
  Display* display_;
  unsigned long last_ignored_request_;
  int last_xserver_error_code_;
  bool enabled_;

  DISALLOW_COPY_AND_ASSIGN(XErrorTrap);
};

}  

#endif  
