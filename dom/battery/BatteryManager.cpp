




#include <limits>
#include "mozilla/Hal.h"
#include "BatteryManager.h"
#include "nsIDOMClassInfo.h"
#include "Constants.h"
#include "nsDOMEvent.h"
#include "mozilla/Preferences.h"
#include "nsDOMEventTargetHelper.h"
#include "mozilla/dom/BatteryManagerBinding.h"





#define LEVELCHANGE_EVENT_NAME           NS_LITERAL_STRING("levelchange")
#define CHARGINGCHANGE_EVENT_NAME        NS_LITERAL_STRING("chargingchange")
#define DISCHARGINGTIMECHANGE_EVENT_NAME NS_LITERAL_STRING("dischargingtimechange")
#define CHARGINGTIMECHANGE_EVENT_NAME    NS_LITERAL_STRING("chargingtimechange")

namespace mozilla {
namespace dom {
namespace battery {

BatteryManager::BatteryManager()
  : mLevel(kDefaultLevel)
  , mCharging(kDefaultCharging)
  , mRemainingTime(kDefaultRemainingTime)
{
  SetIsDOMBinding();
}

void
BatteryManager::Init(nsPIDOMWindow *aWindow)
{
  BindToOwner(aWindow);

  hal::RegisterBatteryObserver(this);

  hal::BatteryInformation batteryInfo;
  hal::GetCurrentBatteryInformation(&batteryInfo);

  UpdateFromBatteryInfo(batteryInfo);
}

void
BatteryManager::Shutdown()
{
  hal::UnregisterBatteryObserver(this);
}

JSObject*
BatteryManager::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return BatteryManagerBinding::Wrap(aCx, aScope, this);
}

double
BatteryManager::DischargingTime() const
{
  if (mCharging || mRemainingTime == kUnknownRemainingTime) {
    return std::numeric_limits<double>::infinity();
  }

  return mRemainingTime;
}

double
BatteryManager::ChargingTime() const
{
  if (!mCharging || mRemainingTime == kUnknownRemainingTime) {
    return std::numeric_limits<double>::infinity();
  }

  return mRemainingTime;
}

void
BatteryManager::UpdateFromBatteryInfo(const hal::BatteryInformation& aBatteryInfo)
{
  mLevel = aBatteryInfo.level();
  mCharging = aBatteryInfo.charging();
  mRemainingTime = aBatteryInfo.remainingTime();

  
  if (mLevel == 1.0 && mCharging == true &&
      mRemainingTime != kDefaultRemainingTime) {
    mRemainingTime = kDefaultRemainingTime;
    NS_ERROR("Battery API: When charging and level at 1.0, remaining time "
             "should be 0. Please fix your backend!");
  }
}

void
BatteryManager::Notify(const hal::BatteryInformation& aBatteryInfo)
{
  double previousLevel = mLevel;
  bool previousCharging = mCharging;
  double previousRemainingTime = mRemainingTime;

  UpdateFromBatteryInfo(aBatteryInfo);

  if (previousCharging != mCharging) {
    DispatchTrustedEvent(CHARGINGCHANGE_EVENT_NAME);
  }

  if (previousLevel != mLevel) {
    DispatchTrustedEvent(LEVELCHANGE_EVENT_NAME);
  }

  








  if (mCharging != previousCharging) {
    if (previousRemainingTime != kUnknownRemainingTime) {
      DispatchTrustedEvent(previousCharging ? CHARGINGTIMECHANGE_EVENT_NAME
                                            : DISCHARGINGTIMECHANGE_EVENT_NAME);
    }
    if (mRemainingTime != kUnknownRemainingTime) {
      DispatchTrustedEvent(mCharging ? CHARGINGTIMECHANGE_EVENT_NAME
                                     : DISCHARGINGTIMECHANGE_EVENT_NAME);
    }
  } else if (previousRemainingTime != mRemainingTime) {
    DispatchTrustedEvent(mCharging ? CHARGINGTIMECHANGE_EVENT_NAME
                                   : DISCHARGINGTIMECHANGE_EVENT_NAME);
  }
}

 bool
BatteryManager::HasSupport()
{
  return Preferences::GetBool("dom.battery.enabled", true);
}

} 
} 
} 
