




































#include "BatteryManager.h"
#include "nsIDOMClassInfo.h"
#include "Constants.h"

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
  return RemoveAddEventListener(NS_LITERAL_STRING("levelchange"),
                                mOnLevelChangeListener, aOnlevelchange);
}

NS_IMETHODIMP
BatteryManager::GetOnchargingchange(nsIDOMEventListener** aOnchargingchange)
{
  return GetInnerEventListener(mOnChargingChangeListener, aOnchargingchange);
}

NS_IMETHODIMP
BatteryManager::SetOnchargingchange(nsIDOMEventListener* aOnchargingchange)
{
  return RemoveAddEventListener(NS_LITERAL_STRING("chargingchange"),
                                mOnChargingChangeListener, aOnchargingchange);
}

} 
} 
} 
