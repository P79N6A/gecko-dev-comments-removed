




#ifndef mozilla_dom_battery_BatteryManager_h
#define mozilla_dom_battery_BatteryManager_h

#include "Types.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/Observer.h"
#include "nsCycleCollectionParticipant.h"

class nsPIDOMWindow;
class nsIScriptContext;

namespace mozilla {

namespace hal {
class BatteryInformation;
} 

namespace dom {
namespace battery {

class BatteryManager : public DOMEventTargetHelper
                     , public BatteryObserver
{
public:
  BatteryManager(nsPIDOMWindow* aWindow);

  void Init();
  void Shutdown();

  
  void Notify(const hal::BatteryInformation& aBatteryInfo);

  



  nsPIDOMWindow* GetParentObject() const
  {
     return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  bool Charging() const
  {
    return mCharging;
  }

  double ChargingTime() const;

  double DischargingTime() const;

  double Level() const
  {
    return mLevel;
  }

  IMPL_EVENT_HANDLER(chargingchange)
  IMPL_EVENT_HANDLER(chargingtimechange)
  IMPL_EVENT_HANDLER(dischargingtimechange)
  IMPL_EVENT_HANDLER(levelchange)

private:
  



  void UpdateFromBatteryInfo(const hal::BatteryInformation& aBatteryInfo);

  double mLevel;
  bool   mCharging;
  



  double mRemainingTime;
};

} 
} 
} 

#endif 
