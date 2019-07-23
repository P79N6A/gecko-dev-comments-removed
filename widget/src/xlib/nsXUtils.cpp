




































#include "nsXUtils.h"

#include <unistd.h>
#include <string.h>

#ifdef NEED_USLEEP_PROTOTYPE
extern "C" int usleep(unsigned int);
#endif
#if defined(__QNX__)
#define usleep(s)	sleep(s)
#endif


 void
nsXUtils::XFlashWindow(Display *       aDisplay,
                       Window          aWindow,
                       unsigned int    aTimes,
                       unsigned long   aInterval,
                       XRectangle *    aArea)
{
  Window       root_window = 0;
  Window       child_window = 0;
  GC           gc;
  int          x;
  int          y;
  unsigned int width;
  unsigned int height;
  unsigned int border_width;
  unsigned int depth;
  int          root_x;
  int          root_y;
  unsigned int i;
  XGCValues    gcv;
  
  XGetGeometry(aDisplay,
               aWindow,
               &root_window,
               &x,
               &y,
               &width,
               &height,
               &border_width,
               &depth);
  
  XTranslateCoordinates(aDisplay, 
                        aWindow,
                        root_window, 
                        0, 
                        0,
                        &root_x,
                        &root_y,
                        &child_window);
  
  memset(&gcv, 0, sizeof(XGCValues));
  
  gcv.function = GXxor;
  gcv.foreground = XWhitePixel(aDisplay, XDefaultScreen(aDisplay));
  gcv.subwindow_mode = IncludeInferiors;
  
  if (gcv.foreground == 0)
    gcv.foreground = 1;
  
  gc = XCreateGC(aDisplay,
                 root_window,
                 GCFunction | GCForeground | GCSubwindowMode, 
                 &gcv);
  
  XGrabServer(aDisplay);

  
  
  if (aArea)
  {
	root_x += aArea->x;
	root_y += aArea->y;

	width = aArea->width;
	height = aArea->height;
  }

  
  
  for (i = 0; i < aTimes * 2; i++)
  {
	XFillRectangle(aDisplay,
				   root_window,
				   gc,
				   root_x,
				   root_y,
				   width,
				   height);
	
	XSync(aDisplay, False);
	
	usleep(aInterval);
  }
  
  
  XFreeGC(aDisplay, gc);  
  
  XUngrabServer(aDisplay);
}

