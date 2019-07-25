






































#ifndef mozilla_X11Util_h
#define mozilla_X11Util_h



#if defined(MOZ_WIDGET_GTK2)
#  include <gdk/gdkx.h>
#elif defined(MOZ_WIDGET_QT)


#undef CursorShape
#  include <QX11Info>
#else
#  error Unknown toolkit
#endif 

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
  ScopedXFree(T* aPtr) : mPtr(aPtr) {}

  ~ScopedXFree()
  {
    if (mPtr)
      XFree(mPtr);
  }

  operator T*() const { return get(); }
  T* operator->() const { return get(); }
  T* get() const { return mPtr; }


private:
  T* mPtr;

  
  ScopedXFree(const ScopedXFree&);
  ScopedXFree& operator=(const ScopedXFree&);
  static void* operator new (size_t);
  static void operator delete (void*);
};

} 

#endif  
