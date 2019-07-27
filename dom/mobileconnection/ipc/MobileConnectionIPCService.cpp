



#include "MobileConnectionIPCService.h"

#include "mozilla/dom/ContentChild.h"
#include "mozilla/Preferences.h"
#include "mozilla/StaticPtr.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::dom::mobileconnection;

NS_IMPL_ISUPPORTS(MobileConnectionIPCService, nsIMobileConnectionService)

StaticRefPtr<MobileConnectionIPCService> sService;

MobileConnectionIPCService*
MobileConnectionIPCService::GetSingleton()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (sService) {
    return sService;
  }

  sService = new MobileConnectionIPCService();
  return sService;
}

MobileConnectionIPCService::MobileConnectionIPCService()
{
  int32_t numRil = Preferences::GetInt("ril.numRadioInterfaces", 1);
  mItems.SetLength(numRil);
}

MobileConnectionIPCService::~MobileConnectionIPCService()
{
  uint32_t count = mItems.Length();
  for (uint32_t i = 0; i < count; i++) {
    if (mItems[i]) {
      mItems[i]->Shutdown();
    }
  }
}



NS_IMETHODIMP
MobileConnectionIPCService::GetNumItems(uint32_t* aNumItems)
{
  *aNumItems = mItems.Length();
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionIPCService::GetItemByServiceId(uint32_t aServiceId,
                                               nsIMobileConnection** aItem)
{
  NS_ENSURE_TRUE(aServiceId < mItems.Length(), NS_ERROR_INVALID_ARG);

  if (!mItems[aServiceId]) {
    nsRefPtr<MobileConnectionChild> child = new MobileConnectionChild(aServiceId);

    
    
    ContentChild::GetSingleton()->SendPMobileConnectionConstructor(child,
                                                                   aServiceId);
    child->Init();

    mItems[aServiceId] = child;
  }

  nsRefPtr<nsIMobileConnection> item(mItems[aServiceId]);
  item.forget(aItem);

  return NS_OK;
}
