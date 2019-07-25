






#ifndef mozilla_X11Util_h
#define mozilla_X11Util_h



#if defined(MOZ_WIDGET_GTK)
#  include <gdk/gdk.h>
#  include <gdk/gdkx.h>
#elif defined(MOZ_WIDGET_QT)
#include "gfxQtPlatform.h"
#undef CursorShape
#  include <X11/Xlib.h>
#else
#  error Unknown toolkit
#endif 

#include "mozilla/Scoped.h"

#include "gfxCore.h"
#include "nsDebug.h"

namespace mozilla {




inline Display*
DefaultXDisplay()
{
#if defined(MOZ_WIDGET_GTK)
  return GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
#elif defined(MOZ_WIDGET_QT)
  return gfxQtPlatform::GetXDisplay();
#endif
}







void
FindVisualAndDepth(Display* aDisplay, VisualID aVisualID,
                   Visual** aVisual, int* aDepth);











void
FinishX(Display* aDisplay);





template <typename T>
struct ScopedXFreePtrTraits
{
  typedef T *type;
  static T *empty() { return NULL; }
  static void release(T *ptr) { if (ptr!=NULL) XFree(ptr); }
};
SCOPED_TEMPLATE(ScopedXFree, ScopedXFreePtrTraits)
















class NS_GFX ScopedXErrorHandler
{
public:
    
    struct ErrorEvent
    {
        XErrorEvent mError;

        ErrorEvent()
        {
            memset(this, 0, sizeof(ErrorEvent));
        }
    };

private:

    
    ErrorEvent mXError;

    
    static ErrorEvent* sXErrorPtr;

    
    ErrorEvent* mOldXErrorPtr;

    
    int (*mOldErrorHandler)(Display *, XErrorEvent *);

public:

    static int
    ErrorHandler(Display *, XErrorEvent *ev);

    ScopedXErrorHandler();

    ~ScopedXErrorHandler();

    





    bool SyncAndGetError(Display *dpy, XErrorEvent *ev = nullptr);
};

} 

#endif  
