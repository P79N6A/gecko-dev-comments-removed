



#ifndef mozilla_dom_time_TimeManager_h
#define mozilla_dom_time_TimeManager_h

#include "mozilla/HalTypes.h"
#include "nsIDOMTimeManager.h"
#include "mozilla/Observer.h"

class nsPIDOMWindow;

namespace mozilla {

typedef Observer<hal::SystemTimeChange> SystemTimeObserver;

namespace dom {
namespace time {
class TimeManager : public nsIDOMMozTimeManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZTIMEMANAGER
};

} 
} 
} 

#endif
