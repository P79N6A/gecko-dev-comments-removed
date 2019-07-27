



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

namespace mozilla {
namespace dom {
namespace alarm {

typedef Observer<void_t> AlarmObserver;
typedef Observer<hal::SystemTimezoneChangeInformation> SystemTimezoneChangeObserver;
typedef Observer<int64_t> SystemClockChangeObserver;

class AlarmHalService : public nsIAlarmHalService,
                        public AlarmObserver,
                        public SystemTimezoneChangeObserver,
                        public SystemClockChangeObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIALARMHALSERVICE

  void Init();

  static already_AddRefed<AlarmHalService> GetInstance();

  
  void Notify(const void_t& aVoid) override;

  
  void Notify(const hal::SystemTimezoneChangeInformation& aSystemTimezoneChangeInfo) override;

  
  void Notify(const int64_t& aClockDeltaMS);

private:
  virtual ~AlarmHalService();

  bool mAlarmEnabled;
  static StaticRefPtr<AlarmHalService> sSingleton;

  nsCOMPtr<nsIAlarmFiredCb> mAlarmFiredCb;
  nsCOMPtr<nsITimezoneChangedCb> mTimezoneChangedCb;
  nsCOMPtr<nsISystemClockChangedCb> mSystemClockChangedCb;
};

} 
} 
} 

#endif 
