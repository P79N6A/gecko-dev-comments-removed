





#include "CallsList.h"
#include "mozilla/dom/CallsListBinding.h"

#include "Telephony.h"
#include "TelephonyCall.h"
#include "TelephonyCallGroup.h"

using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_2(CallsList,
                                        mTelephony,
                                        mGroup)

NS_IMPL_CYCLE_COLLECTING_ADDREF(CallsList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(CallsList)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(CallsList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

CallsList::CallsList(Telephony* aTelephony, TelephonyCallGroup* aGroup)
: mTelephony(aTelephony), mGroup(aGroup)
{
  MOZ_ASSERT(mTelephony);

  SetIsDOMBinding();
}

CallsList::~CallsList()
{
}

nsPIDOMWindow*
CallsList::GetParentObject() const
{
  return mTelephony->GetOwner();
}

JSObject*
CallsList::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return CallsListBinding::Wrap(aCx, aScope, this);
}

already_AddRefed<TelephonyCall>
CallsList::Item(uint32_t aIndex) const
{
  nsRefPtr<TelephonyCall> call;
  call = mGroup ? mGroup->CallsArray().SafeElementAt(aIndex) :
                  mTelephony->CallsArray().SafeElementAt(aIndex);

  return call.forget();
}

uint32_t
CallsList::Length() const
{
  return mGroup ? mGroup->CallsArray().Length() :
                  mTelephony->CallsArray().Length();
}

already_AddRefed<TelephonyCall>
CallsList::IndexedGetter(uint32_t aIndex, bool& aFound) const
{
  nsRefPtr<TelephonyCall> call;
  call = mGroup ? mGroup->CallsArray().SafeElementAt(aIndex) :
                  mTelephony->CallsArray().SafeElementAt(aIndex);
  aFound = call ? true : false;

  return call.forget();
}
