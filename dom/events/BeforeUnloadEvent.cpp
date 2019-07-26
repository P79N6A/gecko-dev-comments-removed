




#include "mozilla/dom/BeforeUnloadEvent.h"

namespace mozilla {
namespace dom {

NS_IMPL_ADDREF_INHERITED(BeforeUnloadEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(BeforeUnloadEvent, nsDOMEvent)

NS_INTERFACE_MAP_BEGIN(BeforeUnloadEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBeforeUnloadEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMETHODIMP
BeforeUnloadEvent::SetReturnValue(const nsAString& aReturnValue)
{
  mText = aReturnValue;
  return NS_OK;  
}

NS_IMETHODIMP
BeforeUnloadEvent::GetReturnValue(nsAString& aReturnValue)
{
  aReturnValue = mText;
  return NS_OK;  
}

} 
} 

using namespace mozilla;
using namespace mozilla::dom;

nsresult
NS_NewDOMBeforeUnloadEvent(nsIDOMEvent** aInstancePtrResult,
                           EventTarget* aOwner,
                           nsPresContext* aPresContext,
                           WidgetEvent* aEvent) 
{
  BeforeUnloadEvent* it = new BeforeUnloadEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
