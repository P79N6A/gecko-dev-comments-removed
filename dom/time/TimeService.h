



#ifndef mozilla_dom_time_TimeService_h
#define mozilla_dom_time_TimeService_h

#include "mozilla/StaticPtr.h"
#include "nsITimeService.h"

namespace mozilla {
namespace dom {
namespace time {




class TimeService : public nsITimeService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMESERVICE

  static already_AddRefed<TimeService> GetInstance();

private:
  virtual ~TimeService() {};

  static StaticRefPtr<TimeService> sSingleton;
};

} 
} 
} 

#endif 
