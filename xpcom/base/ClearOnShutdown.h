





#ifndef mozilla_ClearOnShutdown_h
#define mozilla_ClearOnShutdown_h

#include "mozilla/LinkedList.h"
#include "mozilla/StaticPtr.h"
#include "MainThreadUtils.h"

























namespace mozilla {
namespace ClearOnShutdown_Internal {

class ShutdownObserver : public LinkedListElement<ShutdownObserver>
{
public:
  virtual void Shutdown() = 0;
  virtual ~ShutdownObserver()
  {
  }
};

template<class SmartPtr>
class PointerClearer : public ShutdownObserver
{
public:
  explicit PointerClearer(SmartPtr* aPtr)
    : mPtr(aPtr)
  {
  }

  virtual void Shutdown() MOZ_OVERRIDE
  {
    if (mPtr) {
      *mPtr = nullptr;
    }
  }

private:
  SmartPtr* mPtr;
};

extern bool sHasShutDown;
extern StaticAutoPtr<LinkedList<ShutdownObserver>> sShutdownObservers;

} 

template<class SmartPtr>
inline void
ClearOnShutdown(SmartPtr* aPtr)
{
  using namespace ClearOnShutdown_Internal;

  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!sHasShutDown);

  if (!sShutdownObservers) {
    sShutdownObservers = new LinkedList<ShutdownObserver>();
  }
  sShutdownObservers->insertBack(new PointerClearer<SmartPtr>(aPtr));
}



inline void
KillClearOnShutdown()
{
  using namespace ClearOnShutdown_Internal;

  MOZ_ASSERT(NS_IsMainThread());

  if (sShutdownObservers) {
    while (ShutdownObserver* observer = sShutdownObservers->popFirst()) {
      observer->Shutdown();
      delete observer;
    }
  }

  sShutdownObservers = nullptr;
  sHasShutDown = true;
}

} 

#endif
