




#ifndef mozilla_dom_network_Types_h
#define mozilla_dom_network_Types_h

namespace mozilla {
namespace hal {
class NetworkInformation;
} 

template <class T>
class Observer;

typedef Observer<hal::NetworkInformation> NetworkObserver;

} 

#endif 
