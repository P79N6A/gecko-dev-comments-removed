




#ifndef _mozilla_time_change_observer_h_
#define _mozilla_time_change_observer_h_

#include "mozilla/Hal.h"
#include "mozilla/Observer.h"
#include "mozilla/HalTypes.h"
#include "nsPIDOMWindow.h"
#include "nsWeakPtr.h"
#include "nsTObserverArray.h"

typedef mozilla::Observer<int64_t> SystemClockChangeObserver;
typedef mozilla::Observer<mozilla::hal::SystemTimezoneChangeInformation> SystemTimezoneChangeObserver;

class nsSystemTimeChangeObserver : public SystemClockChangeObserver,
                                   public SystemTimezoneChangeObserver
{
  typedef nsTObserverArray<nsWeakPtr> ListenerArray;
public:
  static nsSystemTimeChangeObserver* GetInstance();
  virtual ~nsSystemTimeChangeObserver();

  
  void Notify(const int64_t& aClockDeltaMS);

  
  void Notify(
    const mozilla::hal::SystemTimezoneChangeInformation& aSystemTimezoneChangeInfo);

  static nsresult AddWindowListener(nsPIDOMWindow* aWindow);
  static nsresult RemoveWindowListener(nsPIDOMWindow* aWindow);
private:
  nsresult AddWindowListenerImpl(nsPIDOMWindow* aWindow);
  nsresult RemoveWindowListenerImpl(nsPIDOMWindow* aWindow);
  nsSystemTimeChangeObserver() { };
  ListenerArray mWindowListeners;
  void FireMozTimeChangeEvent();
};

#endif 
