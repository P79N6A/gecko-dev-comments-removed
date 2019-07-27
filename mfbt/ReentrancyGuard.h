







#ifndef mozilla_ReentrancyGuard_h
#define mozilla_ReentrancyGuard_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/GuardObjects.h"

namespace mozilla {


class ReentrancyGuard
{
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
#ifdef DEBUG
  bool& mEntered;
#endif

public:
  template<class T>
#ifdef DEBUG
  ReentrancyGuard(T& aObj
                  MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mEntered(aObj.mEntered)
#else
  ReentrancyGuard(T&
                  MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
#endif
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
#ifdef DEBUG
    MOZ_ASSERT(!mEntered);
    mEntered = true;
#endif
  }
  ~ReentrancyGuard()
  {
#ifdef DEBUG
    mEntered = false;
#endif
  }

private:
  ReentrancyGuard(const ReentrancyGuard&) MOZ_DELETE;
  void operator=(const ReentrancyGuard&) MOZ_DELETE;
};

} 

#endif 
