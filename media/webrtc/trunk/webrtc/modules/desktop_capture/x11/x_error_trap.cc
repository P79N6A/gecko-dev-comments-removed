









#include "webrtc/modules/desktop_capture/x11/x_error_trap.h"

#include <assert.h>
#include <limits>

namespace webrtc {


Bool XErrorTrap::XServerErrorHandler(Display* display, xReply* rep,
                                     char* , int ,
                                     XPointer data) {
  XErrorTrap* self = reinterpret_cast<XErrorTrap*>(data);

  if (rep->generic.type != X_Error ||
      
      
      self->last_ignored_request_ - display->last_request_read <
      std::numeric_limits<unsigned long>::max() >> 1)
    return False;

  self->last_xserver_error_code_ = rep->error.errorCode;
  return True;
}

XErrorTrap::XErrorTrap(Display* display)
    : display_(display),
      last_xserver_error_code_(0),
      enabled_(true) {
  
  
  
  
  
  
  
  LockDisplay(display);
  async_handler_.next = display->async_handlers;
  async_handler_.handler = XServerErrorHandler;
  async_handler_.data = reinterpret_cast<XPointer>(this);
  display->async_handlers = &async_handler_;
  last_ignored_request_ = display->request;
  UnlockDisplay(display);
}

int XErrorTrap::GetLastErrorAndDisable() {
  assert(enabled_);
  enabled_ = false;
  LockDisplay(display_);
  DeqAsyncHandler(display_, &async_handler_);
  UnlockDisplay(display_);
  return last_xserver_error_code_;
}

XErrorTrap::~XErrorTrap() {
  if (enabled_)
    GetLastErrorAndDisable();
}

}  
