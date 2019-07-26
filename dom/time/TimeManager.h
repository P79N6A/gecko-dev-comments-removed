



#ifndef mozilla_dom_time_TimeManager_h
#define mozilla_dom_time_TimeManager_h

#include "nsIDOMTimeManager.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {
namespace time {

class TimeManager MOZ_FINAL : public nsIDOMMozTimeManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZTIMEMANAGER
};

} 
} 
} 

#endif
