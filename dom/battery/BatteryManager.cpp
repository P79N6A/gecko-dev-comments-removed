




































#include "mozilla/Hal.h"
#include "BatteryManager.h"
#include "nsIDOMClassInfo.h"
#include "Constants.h"
#include "nsDOMEvent.h"
#include "mozilla/Preferences.h"





#define LEVELCHANGE_EVENT_NAME    NS_LITERAL_STRING("levelchange")
#define CHARGINGCHANGE_EVENT_NAME NS_LITERAL_STRING("chargingchange")

DOMCI_DATA(BatteryManager, mozilla::dom::battery::BatteryManager)

namespace mozilla {
namespace dom {
namespace battery {

NS_IMPL_CYCLE_COLLECTION_CLASS(BatteryManager)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(BatteryManager,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnLevelChangeListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnChargingChangeListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(BatteryManager,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnLevelChangeListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnChargingChangeListener)
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
BatteryManager::GetLevel(float* aLevel)
{
  *aLevel = mLevel;

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
}

void
BatteryManager::Notify(const hal::BatteryInformation& aBatteryInfo)
{
  float previousLevel = mLevel;
  bool previousCharging = mCharging;

  UpdateFromBatteryInfo(aBatteryInfo);

  if (previousCharging != mCharging) {
    DispatchTrustedEventToSelf(CHARGINGCHANGE_EVENT_NAME);
  }

  if (previousLevel != mLevel) {
    DispatchTrustedEventToSelf(LEVELCHANGE_EVENT_NAME);
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
