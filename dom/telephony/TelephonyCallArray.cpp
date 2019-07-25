






































#include "TelephonyCallArray.h"


#include "nsINode.h"

#include "dombindings.h"
#include "nsDOMClassInfo.h"

#include "Telephony.h"
#include "TelephonyCall.h"

USING_TELEPHONY_NAMESPACE

TelephonyCallArray::TelephonyCallArray(Telephony* aTelephony)
: mTelephony(aTelephony)
{
  SetIsProxy();
}

TelephonyCallArray::~TelephonyCallArray()
{ }


already_AddRefed<TelephonyCallArray>
TelephonyCallArray::Create(Telephony* aTelephony)
{
  nsRefPtr<TelephonyCallArray> array = new TelephonyCallArray(aTelephony);
  return array.forget();
}

NS_IMPL_CYCLE_COLLECTION_CLASS(TelephonyCallArray)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(TelephonyCallArray)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mTelephony");
  cb.NoteXPCOMChild(tmp->mTelephony->ToISupports());
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(TelephonyCallArray)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(TelephonyCallArray)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mTelephony)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(TelephonyCallArray)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TelephonyCallArray)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TelephonyCallArray)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(TelephonyCallArray)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTelephonyCallArray)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

DOMCI_DATA(TelephonyCallArray, TelephonyCallArray)

NS_IMETHODIMP
TelephonyCallArray::GetLength(PRUint32* aLength)
{
  NS_ASSERTION(aLength, "Null pointer!");
  *aLength = mTelephony->Calls().Length();
  return NS_OK;
}

nsIDOMTelephonyCall*
TelephonyCallArray::GetCallAt(PRUint32 aIndex)
{
  return mTelephony->Calls().SafeElementAt(aIndex);
}

nsISupports*
TelephonyCallArray::GetParentObject() const
{
  return mTelephony->ToISupports();
}

JSObject*
TelephonyCallArray::WrapObject(JSContext* aCx, XPCWrappedNativeScope* aScope,
                               bool* aTriedToWrap)
{
  return mozilla::dom::binding::TelephonyCallArray::create(aCx, aScope, this,
                                                           aTriedToWrap);
}
