




































#ifndef mozilla_dom_battery_BatteryManager_h
#define mozilla_dom_battery_BatteryManager_h

#include "nsIDOMBatteryManager.h"
#include "nsDOMEventTargetWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Observer.h"
#include "Types.h"

class nsPIDOMWindow;
class nsIScriptContext;

namespace mozilla {

namespace hal {
class BatteryInformation;
} 

namespace dom {
namespace battery {

class BatteryManager : public nsDOMEventTargetWrapperCache
                     , public nsIDOMMozBatteryManager
                     , public BatteryObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZBATTERYMANAGER
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetWrapperCache::)

  BatteryManager();
  virtual ~BatteryManager();

  void Init(nsPIDOMWindow *aWindow, nsIScriptContext* aScriptContext);
  void Shutdown();

  
  void Notify(const hal::BatteryInformation& aBatteryInfo);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BatteryManager,
                                           nsDOMEventTargetWrapperCache)

  



  static bool HasSupport();


private:
  


  nsresult DispatchTrustedEventToSelf(const nsAString& aEventName);

  



  void UpdateFromBatteryInfo(const hal::BatteryInformation& aBatteryInfo);

  double mLevel;
  bool   mCharging;
  



  double mRemainingTime;

  NS_DECL_EVENT_HANDLER(levelchange)
  NS_DECL_EVENT_HANDLER(chargingchange)
  NS_DECL_EVENT_HANDLER(chargingtimechange)
  NS_DECL_EVENT_HANDLER(dischargingtimechange)
};

} 
} 
} 

#endif
