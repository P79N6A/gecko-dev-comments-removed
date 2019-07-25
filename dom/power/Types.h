



#ifndef mozilla_dom_power_Types_h
#define mozilla_dom_power_Types_h

namespace mozilla {
namespace hal {
class WakeLockInformation;
} 

template <class T>
class Observer;

typedef Observer<hal::WakeLockInformation> WakeLockObserver;

} 

#endif 

