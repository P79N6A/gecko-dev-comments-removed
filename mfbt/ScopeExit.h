







#ifndef mozilla_ScopeExit_h
#define mozilla_ScopeExit_h








































































#include "mozilla/GuardObjects.h"
#include "mozilla/Move.h"

namespace mozilla {

template <typename ExitFunction>
class ScopeExit {
  ExitFunction mExitFunction;
  bool mExecuteOnDestruction;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

public:
  explicit ScopeExit(ExitFunction&& cleanup
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
   : mExitFunction(cleanup)
   , mExecuteOnDestruction(true)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  }

  ScopeExit(ScopeExit&& rhs)
   : mExitFunction(mozilla::Move(rhs.mExitFunction))
   , mExecuteOnDestruction(rhs.mExecuteOnDestruction)
  {
    rhs.release();
  }

  ~ScopeExit() {
    if (mExecuteOnDestruction) {
      mExitFunction();
    }
  }

  void release() {
    mExecuteOnDestruction = false;
  }

private:
  explicit ScopeExit(const ScopeExit&) = delete;
  ScopeExit& operator=(const ScopeExit&) = delete;
  ScopeExit& operator=(ScopeExit&&) = delete;
};

template <typename ExitFunction>
ScopeExit<ExitFunction>
MakeScopeExit(ExitFunction&& exitFunction)
{
  return ScopeExit<ExitFunction>(mozilla::Move(exitFunction));
}

} 

#endif 
