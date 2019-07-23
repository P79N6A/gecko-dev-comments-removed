



#ifndef CHROME_COMMON_X11_UTIL_INTERNAL_H_
#define CHROME_COMMON_X11_UTIL_INTERNAL_H_







extern "C" {
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xrender.h>
}

namespace x11_util {
  
  

  
  XRenderPictFormat* GetRenderARGB32Format(Display* dpy);
  
  
  XRenderPictFormat* GetRenderVisualFormat(Display* dpy, Visual* visual);
};

#endif  
