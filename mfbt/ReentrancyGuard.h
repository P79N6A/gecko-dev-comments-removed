






#ifndef mozilla_ReentrancyGuard_h_
#define mozilla_ReentrancyGuard_h_

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/GuardObjects.h"

namespace mozilla {


class ReentrancyGuard
{
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
#ifdef DEBUG
    bool& entered;
#endif

  public:
    template<class T>
#ifdef DEBUG
    ReentrancyGuard(T& obj
                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : entered(obj.entered)
#else
    ReentrancyGuard(T&
                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
#endif
    {
      MOZ_GUARD_OBJECT_NOTIFIER_INIT;
#ifdef DEBUG
      MOZ_ASSERT(!entered);
      entered = true;
#endif
    }
    ~ReentrancyGuard()
    {
#ifdef DEBUG
      entered = false;
#endif
    }

  private:
    ReentrancyGuard(const ReentrancyGuard&) MOZ_DELETE;
    void operator=(const ReentrancyGuard&) MOZ_DELETE;
};

} 

#endif 
