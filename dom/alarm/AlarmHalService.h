


 
#ifndef mozilla_dom_alarm_AlarmHalService_h
#define mozilla_dom_alarm_AlarmHalService_h

#include "base/basictypes.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/Hal.h"
#include "mozilla/Services.h"
#include "nsIAlarmHalService.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "prtime.h"

namespace mozilla {
namespace dom {
namespace alarm {

class AlarmHalService : public nsIAlarmHalService, 
                        mozilla::hal::AlarmObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIALARMHALSERVICE

  void Init();
  virtual ~AlarmHalService();

  static already_AddRefed<nsIAlarmHalService> GetInstance();

  
  void Notify(const mozilla::void_t& aVoid);

private:
  bool mAlarmEnabled;
  nsCOMPtr<nsIAlarmFiredCb> mAlarmFiredCb;
  static StaticRefPtr<AlarmHalService> sSingleton;

  
  
  
  
  
  
  nsCOMPtr<nsITimezoneChangedCb> mTimezoneChangedCb;

  int32_t GetTimezoneOffset(bool aIgnoreDST);
};

} 
} 
} 

#endif 
