




































#ifndef mozilla_dom_battery_Types_h
#define mozilla_dom_battery_Types_h

namespace mozilla {
namespace hal {
class BatteryInformation;
} 

template <class T>
class Observer;

typedef Observer<hal::BatteryInformation> BatteryObserver;

} 

#endif 

