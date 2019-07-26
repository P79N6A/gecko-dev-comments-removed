




#include "nsDOMClassInfoID.h"
#include "nsDOMBeforeUnloadEvent.h"

NS_IMPL_ADDREF_INHERITED(nsDOMBeforeUnloadEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMBeforeUnloadEvent, nsDOMEvent)

DOMCI_DATA(BeforeUnloadEvent, nsDOMBeforeUnloadEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMBeforeUnloadEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBeforeUnloadEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(BeforeUnloadEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMETHODIMP
nsDOMBeforeUnloadEvent::SetReturnValue(const nsAString& aReturnValue)
{
  mText = aReturnValue;
  return NS_OK;  
}

NS_IMETHODIMP
nsDOMBeforeUnloadEvent::GetReturnValue(nsAString& aReturnValue)
{
  aReturnValue = mText;
  return NS_OK;  
}

nsresult NS_NewDOMBeforeUnloadEvent(nsIDOMEvent** aInstancePtrResult,
                                    mozilla::dom::EventTarget* aOwner,
                                    nsPresContext* aPresContext,
                                    nsEvent *aEvent) 
{
  nsDOMBeforeUnloadEvent* it =
    new nsDOMBeforeUnloadEvent(aOwner, aPresContext, aEvent);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
