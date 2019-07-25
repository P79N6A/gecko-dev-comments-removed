




#ifndef _mozilla_time_change_observer_h_
#define _mozilla_time_change_observer_h_

#include "mozilla/Hal.h"
#include "mozilla/Observer.h"
#include "mozilla/HalTypes.h"
#include "nsPIDOMWindow.h"
#include "nsWeakPtr.h"

typedef mozilla::Observer<mozilla::hal::SystemTimeChange> SystemTimeChangeObserver;

class nsSystemTimeChangeObserver : public SystemTimeChangeObserver
{
public:
  static nsSystemTimeChangeObserver* GetInstance();
  void Notify(const mozilla::hal::SystemTimeChange& aReason);
  nsresult AddWindowListener(nsIDOMWindow *aWindow);
  nsresult RemoveWindowListener(nsIDOMWindow *aWindow);
private:
  nsSystemTimeChangeObserver() {};
  nsTArray<nsWeakPtr> mWindowListeners;
};

#endif 
