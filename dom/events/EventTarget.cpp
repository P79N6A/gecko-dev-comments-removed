




#include "mozilla/EventListenerManager.h"
#include "mozilla/dom/EventTarget.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace dom {

void
EventTarget::RemoveEventListener(const nsAString& aType,
                                 EventListener* aListener,
                                 bool aUseCapture,
                                 ErrorResult& aRv)
{
  EventListenerManager* elm = GetExistingListenerManager();
  if (elm) {
    elm->RemoveEventListener(aType, aListener, aUseCapture);
  }
}

EventHandlerNonNull*
EventTarget::GetEventHandler(nsIAtom* aType, const nsAString& aTypeString)
{
  EventListenerManager* elm = GetExistingListenerManager();
  return elm ? elm->GetEventHandler(aType, aTypeString) : nullptr;
}

void
EventTarget::SetEventHandler(const nsAString& aType,
                             EventHandlerNonNull* aHandler,
                             ErrorResult& aRv)
{
  if (!StringBeginsWith(aType, NS_LITERAL_STRING("on"))) {
    aRv.Throw(NS_ERROR_INVALID_ARG);
    return;
  }
  if (NS_IsMainThread()) {
    nsCOMPtr<nsIAtom> type = do_GetAtom(aType);
    SetEventHandler(type, EmptyString(), aHandler);
    return;
  }
  SetEventHandler(nullptr,
                  Substring(aType, 2), 
                  aHandler);
}

void
EventTarget::SetEventHandler(nsIAtom* aType, const nsAString& aTypeString,
                             EventHandlerNonNull* aHandler)
{
  GetOrCreateListenerManager()->SetEventHandler(aType, aTypeString, aHandler);
}

} 
} 
