

































#include "nsDOMPopStateEvent.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMClassInfoID.h"
#include "nsIClassInfo.h"
#include "nsIXPCScriptable.h"

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMPopStateEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMPopStateEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMPopStateEvent, nsDOMEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMPopStateEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mState)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMPopStateEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mState)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

DOMCI_DATA(PopStateEvent, nsDOMPopStateEvent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMPopStateEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMPopStateEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(PopStateEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

nsDOMPopStateEvent::~nsDOMPopStateEvent()
{
}

NS_IMETHODIMP
nsDOMPopStateEvent::GetState(nsIVariant **aState)
{
  NS_PRECONDITION(aState, "null state arg");
  NS_IF_ADDREF(*aState = mState);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMPopStateEvent::InitPopStateEvent(const nsAString &aTypeArg,
                                      bool aCanBubbleArg,
                                      bool aCancelableArg,
                                      nsIVariant *aStateArg)
{
  nsresult rv = nsDOMEvent::InitEvent(aTypeArg, aCanBubbleArg, aCancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  mState = aStateArg;
  return NS_OK;
}

nsresult NS_NewDOMPopStateEvent(nsIDOMEvent** aInstancePtrResult,
                                nsPresContext* aPresContext,
                                nsEvent* aEvent)
{
  nsDOMPopStateEvent* event =
    new nsDOMPopStateEvent(aPresContext, aEvent);

  if (!event) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(event, aInstancePtrResult);
}
