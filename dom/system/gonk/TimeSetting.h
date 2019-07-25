



#ifndef mozilla_system_timesetting_h__
#define mozilla_system_timesetting_h__

#include "jspubtd.h"
#include "nsIObserver.h"

namespace mozilla {
namespace system {

class ResultListener;

class TimeSetting : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  TimeSetting();
  virtual ~TimeSetting();
  static nsresult SetTimezone(const JS::Value &aValue, JSContext *aContext);
};

} 
} 

#endif 

