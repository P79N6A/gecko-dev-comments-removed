


 
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

using namespace hal;

class AlarmHalService : public nsIAlarmHalService, 
                        public AlarmObserver,
                        public SystemTimeObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIALARMHALSERVICE

  void Init();
  virtual ~AlarmHalService();

  static already_AddRefed<nsIAlarmHalService> GetInstance();

  
  void Notify(const mozilla::void_t& aVoid);

  
  void Notify(const SystemTimeChange& aReason);

private:
  bool mAlarmEnabled;
  static StaticRefPtr<AlarmHalService> sSingleton;

  nsCOMPtr<nsIAlarmFiredCb> mAlarmFiredCb;
  nsCOMPtr<nsITimezoneChangedCb> mTimezoneChangedCb;

  int32_t GetTimezoneOffset(bool aIgnoreDST);
};

} 
} 
} 

#endif 
