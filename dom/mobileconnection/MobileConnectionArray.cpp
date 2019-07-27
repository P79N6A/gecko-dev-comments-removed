





#include "mozilla/dom/MobileConnectionArray.h"
#include "mozilla/dom/MozMobileConnectionArrayBinding.h"
#include "mozilla/Preferences.h"
#include "nsServiceManagerUtils.h"


#include "ipc/MobileConnectionIPCService.h"
#if defined(MOZ_WIDGET_GONK) && defined(MOZ_B2G_RIL)
#include "nsIGonkMobileConnectionService.h"
#endif
#include "nsXULAppAPI.h" 

using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(MobileConnectionArray,
                                      mWindow,
                                      mMobileConnections)

NS_IMPL_CYCLE_COLLECTING_ADDREF(MobileConnectionArray)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MobileConnectionArray)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MobileConnectionArray)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

MobileConnectionArray::MobileConnectionArray(nsPIDOMWindow* aWindow)
  : mLengthInitialized(false)
  , mWindow(aWindow)
{
  SetIsDOMBinding();
}

MobileConnectionArray::~MobileConnectionArray()
{
}

nsPIDOMWindow*
MobileConnectionArray::GetParentObject() const
{
  MOZ_ASSERT(mWindow);
  return mWindow;
}

JSObject*
MobileConnectionArray::WrapObject(JSContext* aCx)
{
  return MozMobileConnectionArrayBinding::Wrap(aCx, this);
}

MobileConnection*
MobileConnectionArray::Item(uint32_t aIndex)
{
  bool unused;
  return IndexedGetter(aIndex, unused);
}

uint32_t
MobileConnectionArray::Length()
{
  if (!mLengthInitialized) {
    mLengthInitialized = true;

    nsCOMPtr<nsIMobileConnectionService> service =
      do_GetService(NS_MOBILE_CONNECTION_SERVICE_CONTRACTID);
    NS_ENSURE_TRUE(service, 0);

    uint32_t length = 0;
    nsresult rv = service->GetNumItems(&length);
    NS_ENSURE_SUCCESS(rv, 0);

    mMobileConnections.SetLength(length);
  }

  return mMobileConnections.Length();
}

MobileConnection*
MobileConnectionArray::IndexedGetter(uint32_t aIndex, bool& aFound)
{

  aFound = aIndex < Length();
  if (!aFound) {
    return nullptr;
  }

  if (!mMobileConnections[aIndex]) {
    mMobileConnections[aIndex] = new MobileConnection(mWindow, aIndex);
  }

  return mMobileConnections[aIndex];
}

already_AddRefed<nsIMobileConnectionService>
NS_CreateMobileConnectionService()
{
  nsCOMPtr<nsIMobileConnectionService> service;

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    service = new mozilla::dom::mobileconnection::MobileConnectionIPCService();
  } else {
#if defined(MOZ_WIDGET_GONK) && defined(MOZ_B2G_RIL)
    service = do_CreateInstance(GONK_MOBILECONNECTION_SERVICE_CONTRACTID);
#endif
  }

  return service.forget();
}
