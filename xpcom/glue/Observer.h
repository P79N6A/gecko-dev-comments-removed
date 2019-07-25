




#ifndef mozilla_Observer_h
#define mozilla_Observer_h

#include "nsTArray.h"

namespace mozilla {









template <class T>
class Observer
{
public:
  virtual ~Observer() { }
  virtual void Notify(const T& aParam) = 0;
};









template <class T>
class ObserverList
{
public:
  






  void AddObserver(Observer<T>* aObserver) {
    mObservers.AppendElement(aObserver);
  }

  



  bool RemoveObserver(Observer<T>* aObserver) {
    return mObservers.RemoveElement(aObserver);
  }

  uint32_t Length() {
    return mObservers.Length();
  }

  void Broadcast(const T& aParam) {
    uint32_t size = mObservers.Length();
    for (uint32_t i=0; i<size; ++i) {
      mObservers[i]->Notify(aParam);
    }
  }

protected:
  nsTArray<Observer<T>*> mObservers;
};

} 

#endif 
