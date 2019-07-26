



#ifndef  MEDIA_ENGINE_WRAPPER_H_
#define MEDIA_ENGINE_WRAPPER_H_

#include <mozilla/Scoped.h>



namespace mozilla
{







template<typename T>
struct ScopedCustomReleaseTraits0 : public ScopedFreePtrTraits<T>
{
  static void release(T* ptr)
  {
    if(ptr)
    {
      (ptr)->Release();
    }
  }
};

SCOPED_TEMPLATE(ScopedCustomReleasePtr, ScopedCustomReleaseTraits0)
}


#endif
