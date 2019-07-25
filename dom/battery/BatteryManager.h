




































#ifndef mozilla_dom_battery_BatteryManager_h
#define mozilla_dom_battery_BatteryManager_h

#include "nsIDOMBatteryManager.h"
#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Observer.h"
#include "Types.h"

namespace mozilla {

namespace hal {
class BatteryInformation;
} 

namespace dom {
namespace battery {

class BatteryManager : public nsIDOMBatteryManager
                     , public nsDOMEventTargetHelper
                     , public BatteryObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMBATTERYMANAGER
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  BatteryManager();
  virtual ~BatteryManager();

  void Init();
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
  double mDischargingTime;

  nsRefPtr<nsDOMEventListenerWrapper> mOnLevelChangeListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnChargingChangeListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnDischargingTimeChangeListener;
};

} 
} 
} 

#endif 
