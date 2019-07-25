




































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

  void RemoveObserver(Observer<T>* aObserver) {
    mObservers.RemoveElement(aObserver);
  }

  PRUint32 Length() {
    return mObservers.Length();
  }

  void Broadcast(const T& aParam) {
    PRUint32 size = mObservers.Length();
    for (PRUint32 i=0; i<size; ++i) {
      mObservers[i]->Notify(aParam);
    }
  }

protected:
  nsTArray<Observer<T>*> mObservers;
};

} 

#endif 
