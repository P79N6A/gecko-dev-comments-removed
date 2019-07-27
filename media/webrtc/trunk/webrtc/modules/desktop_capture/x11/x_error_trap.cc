









#include "webrtc/modules/desktop_capture/x11/x_error_trap.h"

#include <assert.h>

#if defined(TOOLKIT_GTK)
#include <gdk/gdk.h>
#endif  

namespace webrtc {

namespace {

#if !defined(TOOLKIT_GTK)


static bool g_xserver_error_trap_enabled = false;
static int g_last_xserver_error_code = 0;

int XServerErrorHandler(Display* display, XErrorEvent* error_event) {
  assert(g_xserver_error_trap_enabled);
  g_last_xserver_error_code = error_event->error_code;
  return 0;
}

#endif  

}  

XErrorTrap::XErrorTrap(Display* display)
    : original_error_handler_(NULL),
      enabled_(true) {
#if defined(TOOLKIT_GTK)
  gdk_error_trap_push();
#else  
  assert(!g_xserver_error_trap_enabled);
  original_error_handler_ = XSetErrorHandler(&XServerErrorHandler);
  g_xserver_error_trap_enabled = true;
  g_last_xserver_error_code = 0;
#endif  
}

int XErrorTrap::GetLastErrorAndDisable() {
  enabled_ = false;
#if defined(TOOLKIT_GTK)
  return gdk_error_trap_push();
#else  
  assert(g_xserver_error_trap_enabled);
  XSetErrorHandler(original_error_handler_);
  g_xserver_error_trap_enabled = false;
  return g_last_xserver_error_code;
#endif  
}

XErrorTrap::~XErrorTrap() {
  if (enabled_)
    GetLastErrorAndDisable();
}

}  
