






































#include "xlibxtbin.h"
#include "xlibrgb.h"

#include <stdlib.h>
#include <stdio.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Shell.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

xtbin::xtbin() {
  initialized = False;
  xtwindow    = None;
}

xtbin::~xtbin() {
}

void xtbin::xtbin_init() {
  mXlibRgbHandle = xxlib_find_handle(XXLIBRGB_DEFAULT_HANDLE);
  xtdisplay = xxlib_rgb_get_display(mXlibRgbHandle);
  if (!xtdisplay)
    abort();
  app_context = XtDisplayToApplicationContext(xtdisplay);
  initialized = True;
}

void xtbin::xtbin_realize() {
  Arg args[2];
  int n;
  Widget top_widget;
  Widget embedded;
  XSetWindowAttributes attr;
  unsigned long mask;

  attr.bit_gravity = NorthWestGravity;
  attr.event_mask = 
    ButtonMotionMask |
    ButtonPressMask |
    ButtonReleaseMask |
    EnterWindowMask |
    ExposureMask |
    KeyPressMask |
    KeyReleaseMask |
    LeaveWindowMask |
    PointerMotionMask |
    StructureNotifyMask |
    VisibilityChangeMask |
    FocusChangeMask;

  attr.colormap         = xxlib_rgb_get_cmap(mXlibRgbHandle);
  attr.background_pixel = xxlib_rgb_xpixel_from_rgb(mXlibRgbHandle, 0xFFC0C0C0);
  attr.border_pixel     = xxlib_rgb_xpixel_from_rgb(mXlibRgbHandle, 0xFF646464);

#ifdef DEBUG  
  printf("attr.background_pixel = %lx, attr.border_pixel = %lx, parent_window = %x\n", 
         (long)attr.background_pixel,
         (long)attr.border_pixel, (int)parent_window);
#endif 
  
  mask = CWBitGravity | CWEventMask | CWBorderPixel | CWBackPixel;

  if (attr.colormap)
    mask |= CWColormap;

  window = XCreateWindow(xtdisplay, parent_window,
                         x, y, width, height, 0, 
                         xxlib_rgb_get_depth(mXlibRgbHandle),
                         InputOutput, xxlib_rgb_get_visual(mXlibRgbHandle),
                         mask, &attr);
  XSetWindowBackgroundPixmap(xtdisplay, window, None);
  XSelectInput(xtdisplay, window, ExposureMask);

  XMapWindow(xtdisplay, window);
  XFlush(xtdisplay);

  top_widget = XtAppCreateShell("drawingArea", "Wrapper",
                                applicationShellWidgetClass, xtdisplay,
                                NULL, 0);

  xtwidget = top_widget;

  n = 0;
  XtSetArg(args[n], XtNheight, height); n++;
  XtSetArg(args[n], XtNwidth,  width);  n++;
  XtSetValues(top_widget, args, n);

  embedded = XtVaCreateWidget("form", compositeWidgetClass, top_widget, NULL);

  n = 0;
  XtSetArg(args[n], XtNheight, height); n++;
  XtSetArg(args[n], XtNwidth,  width);  n++;
  XtSetValues(embedded, args, n);

  oldwindow = top_widget->core.window;
  top_widget->core.window = window;

  XtRegisterDrawable(xtdisplay, window, top_widget);

  XtRealizeWidget(embedded);
  XtRealizeWidget(top_widget);
  XtManageChild(embedded);

  
  xtwindow = XtWindow(embedded);

  
  XSetWindowBackgroundPixmap(xtdisplay, XtWindow(top_widget), None);
  XSetWindowBackgroundPixmap(xtdisplay, XtWindow(embedded),   None);

  
  XSelectInput(xtdisplay, XtWindow(top_widget), 0x0fffff);
  XSelectInput(xtdisplay, XtWindow(embedded),   0x0fffff);

  sync();
}

void xtbin::xtbin_new(Window aParent) {
  parent_window = aParent;
}

void xtbin::sync() {
  
  XSync(xtdisplay, False);
}

void xtbin::xtbin_destroy() {
  sync();
  XtUnregisterDrawable(xtdisplay, xtwindow);
  sync();
  xtwidget->core.window = oldwindow;
  XtUnrealizeWidget(xtwidget);
  initialized = False;
}

void xtbin::xtbin_resize(int aX, int aY, int aWidth, int aHeight) {
  x = aX;
  y = aY;
  width = aWidth;
  height = aHeight;
}

int xtbin::xtbin_initialized() {
  return initialized;
}

