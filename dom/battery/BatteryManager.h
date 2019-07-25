




































#ifndef mozilla_dom_battery_BatteryManager_h
#define mozilla_dom_battery_BatteryManager_h

#include "nsIDOMBatteryManager.h"
#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {
namespace dom {
namespace battery {

class BatteryManager : public nsIDOMBatteryManager
                     , public nsDOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMBATTERYMANAGER
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  BatteryManager();
  virtual ~BatteryManager();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BatteryManager,
                                           nsDOMEventTargetHelper)

private:
  float mLevel;
  bool  mCharging;

  nsRefPtr<nsDOMEventListenerWrapper> mOnLevelChangeListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnChargingChangeListener;
};

} 
} 
} 

#endif 
