




#ifdef MOZ_X11
#include <X11/Xlib.h>
#if (MOZ_WIDGET_GTK != 3)
void InstallX11ErrorHandler();
#endif
extern "C" int X11Error(Display *display, XErrorEvent *event);
#endif
