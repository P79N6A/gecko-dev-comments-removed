






































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
















class ScopedXErrorHandler
{
    
    struct ErrorEvent
    {
        XErrorEvent m_error;

        ErrorEvent()
        {
            memset(this, 0, sizeof(ErrorEvent));
        }
    };

    
    ErrorEvent m_xerror;

    
    static ErrorEvent* s_xerrorptr;

    
    ErrorEvent* m_oldxerrorptr;

    
    int (*m_oldErrorHandler)(Display *, XErrorEvent *);

public:

    static int
    ErrorHandler(Display *, XErrorEvent *ev)
    {
        s_xerrorptr->m_error = *ev;
        return 0;
    }

    ScopedXErrorHandler()
    {
        
        
        m_oldxerrorptr = s_xerrorptr;
        s_xerrorptr = &m_xerror;
        m_oldErrorHandler = XSetErrorHandler(ErrorHandler);
    }

    ~ScopedXErrorHandler()
    {
        s_xerrorptr = m_oldxerrorptr;
        XSetErrorHandler(m_oldErrorHandler);
    }

    



    bool SyncAndGetError(Display *dpy, XErrorEvent *ev = nsnull)
    {
        XSync(dpy, False);
        bool retval = m_xerror.m_error.error_code != 0;
        if (ev)
            *ev = m_xerror.m_error;
        m_xerror = ErrorEvent(); 
        return retval;
    }
};


} 

#endif  
