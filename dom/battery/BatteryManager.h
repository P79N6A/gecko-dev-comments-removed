




































#ifndef mozilla_dom_battery_BatteryManager_h
#define mozilla_dom_battery_BatteryManager_h

#include "nsIDOMBatteryManager.h"
#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Observer.h"
#include "Types.h"
#include "nsDOMEventTargetHelper.h"

class nsPIDOMWindow;
class nsIScriptContext;

namespace mozilla {

namespace hal {
class BatteryInformation;
} 

namespace dom {
namespace battery {

class BatteryManager : public nsDOMEventTargetHelper
                     , public nsIDOMMozBatteryManager
                     , public BatteryObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZBATTERYMANAGER
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  BatteryManager();

  void Init(nsPIDOMWindow *aWindow, nsIScriptContext* aScriptContext);
  void Shutdown();

  
  void Notify(const hal::BatteryInformation& aBatteryInfo);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BatteryManager,
                                           nsDOMEventTargetHelper)

  



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
