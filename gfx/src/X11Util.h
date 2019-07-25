






































#ifndef mozilla_X11Util_h
#define mozilla_X11Util_h



#if defined(MOZ_WIDGET_GTK2)
#  include <gdk/gdkx.h>
#elif defined(MOZ_WIDGET_QT)


#undef CursorShape
#  include <QX11Info>
#  include <X11/Xlib.h>
#else
#  error Unknown toolkit
#endif 

#include "gfxCore.h"
#include "nsDebug.h"

namespace mozilla {




inline Display*
DefaultXDisplay()
{
#if defined(MOZ_WIDGET_GTK2)
  return GDK_DISPLAY();
#elif defined(MOZ_WIDGET_QT)
  return QX11Info::display();
#endif
}





template<typename T>
struct ScopedXFree
{
  ScopedXFree() : mPtr(NULL) {}
  ScopedXFree(T* aPtr) : mPtr(aPtr) {}

  ~ScopedXFree() { Assign(NULL); }

  ScopedXFree& operator=(T* aPtr) { Assign(aPtr); return *this; }

  operator T*() const { return get(); }
  T* operator->() const { return get(); }
  T* get() const { return mPtr; }

private:
  void Assign(T* aPtr)
  {
    NS_ASSERTION(!mPtr || mPtr != aPtr, "double-XFree() imminent");

    if (mPtr)
      XFree(mPtr);
    mPtr = aPtr;
  }

  T* mPtr;

  
  ScopedXFree(const ScopedXFree&);
  ScopedXFree& operator=(const ScopedXFree&);
  static void* operator new (size_t);
  static void operator delete (void*);
};
















class NS_GFX ScopedXErrorHandler
{
    
    struct ErrorEvent
    {
        XErrorEvent mError;

        ErrorEvent()
        {
            memset(this, 0, sizeof(ErrorEvent));
        }
    };

    
    ErrorEvent mXError;

    
    static ErrorEvent* sXErrorPtr;

    
    ErrorEvent* mOldXErrorPtr;

    
    int (*mOldErrorHandler)(Display *, XErrorEvent *);

public:

    static int
    ErrorHandler(Display *, XErrorEvent *ev);

    ScopedXErrorHandler();

    ~ScopedXErrorHandler();

    



    bool SyncAndGetError(Display *dpy, XErrorEvent *ev = nsnull);
};


} 

#endif  
