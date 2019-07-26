




#include "nsDOMDataContainerEvent.h"
#include "nsDOMClassInfoID.h"
#include "nsContentUtils.h"

nsDOMDataContainerEvent::nsDOMDataContainerEvent(
                                             mozilla::dom::EventTarget* aOwner,
                                             nsPresContext* aPresContext,
                                             nsEvent* aEvent)
  : nsDOMEvent(aOwner, aPresContext, aEvent)
{
  mData.Init();
  SetIsDOMBinding();
}

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMDataContainerEvent,
                                                nsDOMEvent)
  if (tmp->mData.IsInitialized())
    tmp->mData.Clear();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMDataContainerEvent,
                                                  nsDOMEvent)
  tmp->mData.EnumerateRead(TraverseEntry, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsDOMDataContainerEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMDataContainerEvent, nsDOMEvent)

DOMCI_DATA(DataContainerEvent, nsDOMDataContainerEvent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMDataContainerEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDataContainerEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DataContainerEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMETHODIMP
nsDOMDataContainerEvent::GetData(const nsAString& aKey, nsIVariant **aData)
{
  NS_ENSURE_ARG_POINTER(aData);

  NS_ENSURE_STATE(mData.IsInitialized());

  mData.Get(aKey, aData);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataContainerEvent::SetData(const nsAString& aKey, nsIVariant *aData)
{
  NS_ENSURE_ARG(aData);

  
  NS_ENSURE_STATE(!mEvent->mFlags.mIsBeingDispatched);
  NS_ENSURE_STATE(mData.IsInitialized());
  mData.Put(aKey, aData);
  return NS_OK;
}

void
nsDOMDataContainerEvent::SetData(JSContext* aCx, const nsAString& aKey,
                                 JS::Value aVal, mozilla::ErrorResult& aRv)
{
  if (!nsContentUtils::XPConnect()) {
    aRv = NS_ERROR_FAILURE;
    return;
  }
  nsCOMPtr<nsIVariant> val;
  nsresult rv =
    nsContentUtils::XPConnect()->JSToVariant(aCx, aVal, getter_AddRefs(val));
  if (NS_FAILED(rv)) {
    aRv = rv;
    return;
  }
  aRv = SetData(aKey, val);
}

nsresult
NS_NewDOMDataContainerEvent(nsIDOMEvent** aInstancePtrResult,
                            mozilla::dom::EventTarget* aOwner,
                            nsPresContext* aPresContext,
                            nsEvent* aEvent)
{
  nsDOMDataContainerEvent* it =
    new nsDOMDataContainerEvent(aOwner, aPresContext, aEvent);
  NS_ENSURE_TRUE(it, NS_ERROR_OUT_OF_MEMORY);

  return CallQueryInterface(it, aInstancePtrResult);
}

PLDHashOperator
nsDOMDataContainerEvent::TraverseEntry(const nsAString& aKey,
                                       nsIVariant *aDataItem,
                                       void* aUserArg)
{
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aUserArg);
  cb->NoteXPCOMChild(aDataItem);

  return PL_DHASH_NEXT;
}

