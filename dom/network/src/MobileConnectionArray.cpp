





#include "MobileConnectionArray.h"
#include "mozilla/dom/MozMobileConnectionArrayBinding.h"
#include "mozilla/Preferences.h"

using namespace mozilla::dom::network;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_2(MobileConnectionArray,
                                        mWindow,
                                        mMobileConnections)

NS_IMPL_CYCLE_COLLECTING_ADDREF(MobileConnectionArray)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MobileConnectionArray)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MobileConnectionArray)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

MobileConnectionArray::MobileConnectionArray(nsPIDOMWindow* aWindow)
: mWindow(aWindow)
{
  int32_t numRil = mozilla::Preferences::GetInt("ril.numRadioInterfaces", 1);
  MOZ_ASSERT(numRil > 0);

  for (int32_t id = 0; id < numRil; id++) {
    nsRefPtr<MobileConnection> mobileConnection = new MobileConnection(id);
    mobileConnection->Init(aWindow);
    mMobileConnections.AppendElement(mobileConnection);
  }

  SetIsDOMBinding();
}

MobileConnectionArray::~MobileConnectionArray()
{
  for (uint32_t i = 0; i < mMobileConnections.Length(); i++) {
    mMobileConnections[i]->Shutdown();
  }
  mMobileConnections.Clear();
}

nsPIDOMWindow*
MobileConnectionArray::GetParentObject() const
{
  MOZ_ASSERT(mWindow);
  return mWindow;
}

JSObject*
MobileConnectionArray::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return MozMobileConnectionArrayBinding::Wrap(aCx, aScope, this);
}

nsIDOMMozMobileConnection*
MobileConnectionArray::Item(uint32_t aIndex) const
{
  bool unused;
  return IndexedGetter(aIndex, unused);
}

uint32_t
MobileConnectionArray::Length() const
{
  return mMobileConnections.Length();
}

nsIDOMMozMobileConnection*
MobileConnectionArray::IndexedGetter(uint32_t aIndex, bool& aFound) const
{
  aFound = false;
  aFound = aIndex < mMobileConnections.Length();

  return aFound ? mMobileConnections[aIndex] : nullptr;
}