



#include "USSDReceivedEvent.h"
#include "nsIDOMClassInfo.h"
#include "nsDOMClassInfoID.h"
#include "nsContentUtils.h"

DOMCI_DATA(USSDReceivedEvent, mozilla::dom::network::USSDReceivedEvent)

namespace mozilla {
namespace dom {
namespace network {

already_AddRefed<USSDReceivedEvent>
USSDReceivedEvent::Create(nsAString& aMessage, bool aSessionEnded)
{
  NS_ASSERTION(!aMessage.IsEmpty(), "Empty message!");

  nsRefPtr<USSDReceivedEvent> event = new USSDReceivedEvent();

  event->mMessage = aMessage;
  event->mSessionEnded = aSessionEnded;

  return event.forget();
}

NS_IMPL_ADDREF_INHERITED(USSDReceivedEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(USSDReceivedEvent, nsDOMEvent)

NS_INTERFACE_MAP_BEGIN(USSDReceivedEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMUSSDReceivedEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(USSDReceivedEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMETHODIMP
USSDReceivedEvent::GetMessage(nsAString& aMessage)
{
  aMessage.Assign(mMessage);
  return NS_OK;
}

 NS_IMETHODIMP
USSDReceivedEvent::GetSessionEnded(bool* aSessionEnded)
{
  *aSessionEnded = mSessionEnded;
  return NS_OK;
}

}
}
}
