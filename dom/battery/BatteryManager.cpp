




































#include <limits>
#include "mozilla/Hal.h"
#include "BatteryManager.h"
#include "nsIDOMClassInfo.h"
#include "Constants.h"
#include "nsDOMEvent.h"
#include "mozilla/Preferences.h"





#define LEVELCHANGE_EVENT_NAME           NS_LITERAL_STRING("levelchange")
#define CHARGINGCHANGE_EVENT_NAME        NS_LITERAL_STRING("chargingchange")
#define DISCHARGINGTIMECHANGE_EVENT_NAME NS_LITERAL_STRING("dischargingtimechange")
#define CHARGINGTIMECHANGE_EVENT_NAME    NS_LITERAL_STRING("chargingtimechange")

DOMCI_DATA(BatteryManager, mozilla::dom::battery::BatteryManager)

namespace mozilla {
namespace dom {
namespace battery {

NS_IMPL_CYCLE_COLLECTION_CLASS(BatteryManager)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(BatteryManager,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnLevelChangeListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnChargingChangeListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnDischargingTimeChangeListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(BatteryManager,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnLevelChangeListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnChargingChangeListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnDischargingTimeChangeListener)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BatteryManager)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBatteryManager)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(BatteryManager)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BatteryManager, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BatteryManager, nsDOMEventTargetHelper)

BatteryManager::BatteryManager()
  : mLevel(kDefaultLevel)
  , mCharging(kDefaultCharging)
  , mRemainingTime(kUnknownRemainingTime)
{
}

BatteryManager::~BatteryManager()
{
  if (mListenerManager) {
    mListenerManager->Disconnect();
  }
}

void
BatteryManager::Init()
{
  hal::RegisterBatteryObserver(this);

  hal::BatteryInformation* batteryInfo = new hal::BatteryInformation();
  hal::GetCurrentBatteryInformation(batteryInfo);

  UpdateFromBatteryInfo(*batteryInfo);

  delete batteryInfo;
}

void
BatteryManager::Shutdown()
{
  hal::UnregisterBatteryObserver(this);
}

NS_IMETHODIMP
BatteryManager::GetCharging(bool* aCharging)
{
  *aCharging = mCharging;

  return NS_OK;
}

NS_IMETHODIMP
BatteryManager::GetLevel(double* aLevel)
{
  *aLevel = mLevel;

  return NS_OK;
}

NS_IMETHODIMP
BatteryManager::GetDischargingTime(double* aDischargingTime)
{
  if (mCharging || mRemainingTime == kUnknownRemainingTime) {
    *aDischargingTime = std::numeric_limits<double>::infinity();
    return NS_OK;
  }

  *aDischargingTime = mRemainingTime;

  return NS_OK;
}

NS_IMETHODIMP
BatteryManager::GetChargingTime(double* aChargingTime)
{
  if (!mCharging || mRemainingTime == kUnknownRemainingTime) {
    *aChargingTime = std::numeric_limits<double>::infinity();
    return NS_OK;
  }

  *aChargingTime = mRemainingTime;

  return NS_OK;
}

NS_IMETHODIMP
BatteryManager::GetOnlevelchange(nsIDOMEventListener** aOnlevelchange)
{
  return GetInnerEventListener(mOnLevelChangeListener, aOnlevelchange);
}

NS_IMETHODIMP
BatteryManager::SetOnlevelchange(nsIDOMEventListener* aOnlevelchange)
{
  return RemoveAddEventListener(LEVELCHANGE_EVENT_NAME, mOnLevelChangeListener,
                                aOnlevelchange);
}

NS_IMETHODIMP
BatteryManager::GetOnchargingchange(nsIDOMEventListener** aOnchargingchange)
{
  return GetInnerEventListener(mOnChargingChangeListener, aOnchargingchange);
}

NS_IMETHODIMP
BatteryManager::SetOnchargingchange(nsIDOMEventListener* aOnchargingchange)
{
  return RemoveAddEventListener(CHARGINGCHANGE_EVENT_NAME,
                                mOnChargingChangeListener, aOnchargingchange);
}

NS_IMETHODIMP
BatteryManager::GetOndischargingtimechange(nsIDOMEventListener** aOndischargingtimechange)
{
  return GetInnerEventListener(mOnDischargingTimeChangeListener,
                               aOndischargingtimechange);
}

NS_IMETHODIMP
BatteryManager::SetOndischargingtimechange(nsIDOMEventListener* aOndischargingtimechange)
{
  return RemoveAddEventListener(DISCHARGINGTIMECHANGE_EVENT_NAME,
                                mOnDischargingTimeChangeListener,
                                aOndischargingtimechange);
}

NS_IMETHODIMP
BatteryManager::GetOnchargingtimechange(nsIDOMEventListener** aOnchargingtimechange)
{
  return GetInnerEventListener(mOnChargingTimeChangeListener,
                               aOnchargingtimechange);
}

NS_IMETHODIMP
BatteryManager::SetOnchargingtimechange(nsIDOMEventListener* aOnchargingtimechange)
{
  return RemoveAddEventListener(CHARGINGTIMECHANGE_EVENT_NAME,
                                mOnChargingTimeChangeListener,
                                aOnchargingtimechange);
}

nsresult
BatteryManager::DispatchTrustedEventToSelf(const nsAString& aEventName)
{
  nsRefPtr<nsDOMEvent> event = new nsDOMEvent(nsnull, nsnull);
  nsresult rv = event->InitEvent(aEventName, false, false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = event->SetTrusted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  bool dummy;
  rv = DispatchEvent(event, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
BatteryManager::UpdateFromBatteryInfo(const hal::BatteryInformation& aBatteryInfo)
{
  mLevel = aBatteryInfo.level();
  mCharging = aBatteryInfo.charging();
  mRemainingTime = aBatteryInfo.remainingTime();
}

void
BatteryManager::Notify(const hal::BatteryInformation& aBatteryInfo)
{
  double previousLevel = mLevel;
  bool previousCharging = mCharging;
  double previousRemainingTime = mRemainingTime;

  UpdateFromBatteryInfo(aBatteryInfo);

  if (previousCharging != mCharging) {
    DispatchTrustedEventToSelf(CHARGINGCHANGE_EVENT_NAME);
  }

  if (previousLevel != mLevel) {
    DispatchTrustedEventToSelf(LEVELCHANGE_EVENT_NAME);
  }

  








  if (mCharging != previousCharging) {
    if (previousRemainingTime != kUnknownRemainingTime) {
      DispatchTrustedEventToSelf(previousCharging ? CHARGINGTIMECHANGE_EVENT_NAME
                                                  : DISCHARGINGTIMECHANGE_EVENT_NAME);
    }
    if (mRemainingTime != kUnknownRemainingTime) {
      DispatchTrustedEventToSelf(mCharging ? CHARGINGTIMECHANGE_EVENT_NAME
                                           : DISCHARGINGTIMECHANGE_EVENT_NAME);
    }
  } else if (previousRemainingTime != mRemainingTime) {
    DispatchTrustedEventToSelf(mCharging ? CHARGINGTIMECHANGE_EVENT_NAME
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
