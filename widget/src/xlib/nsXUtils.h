




































#ifndef __nsXUtils_h
#define __nsXUtils_h

#include <X11/Xlib.h>

struct nsXUtils
{
  









  static void XFlashWindow(Display *       aDisplay,
                           Window          aWindow,
                           unsigned int    aTimes,
                           unsigned long   aInterval,
                           XRectangle *    aArea);
};

#endif  
