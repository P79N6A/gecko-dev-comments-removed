






































#ifndef mozilla_ClearOnShutdown_h
#define mozilla_ClearOnShutdown_h

#include "mozilla/LinkedList.h"

















namespace mozilla {
namespace ClearOnShutdown_Internal {

class ShutdownObserver : public LinkedListElement<ShutdownObserver>
{
public:
  virtual void Shutdown() = 0;
};

template<class SmartPtr>
class PointerClearer : public ShutdownObserver
{
public:
  PointerClearer(SmartPtr *aPtr)
    : mPtr(aPtr)
  {}

  virtual void Shutdown()
  {
    if (mPtr) {
      *mPtr = NULL;
    }
  }

private:
  SmartPtr *mPtr;
};

extern bool sHasShutDown;
extern LinkedList<ShutdownObserver> sShutdownObservers;

} 

template<class SmartPtr>
inline void ClearOnShutdown(SmartPtr *aPtr)
{
  using namespace ClearOnShutdown_Internal;

  MOZ_ASSERT(!sHasShutDown);
  ShutdownObserver *observer = new PointerClearer<SmartPtr>(aPtr);
  sShutdownObservers.insertBack(observer);
}



inline void KillClearOnShutdown()
{
  using namespace ClearOnShutdown_Internal;

  ShutdownObserver *observer;
  while ((observer = sShutdownObservers.popFirst())) {
    observer->Shutdown();
    delete observer;
  }

  sHasShutDown = true;
}

} 

#endif
