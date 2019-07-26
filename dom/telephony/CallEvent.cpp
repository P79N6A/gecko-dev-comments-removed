





#include "CallEvent.h"
#include "mozilla/dom/CallEventBinding.h"

#include "TelephonyCall.h"

using namespace mozilla::dom;
using mozilla::ErrorResult;


already_AddRefed<CallEvent>
CallEvent::Create(EventTarget* aOwner, const nsAString& aType,
                  TelephonyCall* aCall, bool aCanBubble,
                  bool aCancelable)
{
  nsRefPtr<CallEvent> event = new CallEvent(aOwner, nullptr, nullptr);
  event->mCall = aCall;
  event->InitEvent(aType, aCanBubble, aCancelable);
  return event.forget();
}

JSObject*
CallEvent::WrapObject(JSContext* aCx)
{
  return CallEventBinding::Wrap(aCx, this);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(CallEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(CallEvent, Event)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mCall)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(CallEvent, Event)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCall)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(CallEvent, Event)
NS_IMPL_RELEASE_INHERITED(CallEvent, Event)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(CallEvent)
NS_INTERFACE_MAP_END_INHERITING(Event)




already_AddRefed<CallEvent>
CallEvent::Constructor(const GlobalObject& aGlobal, const nsAString& aType,
                       const CallEventInit& aOptions, ErrorResult& aRv)
{
  nsCOMPtr<EventTarget> target = do_QueryInterface(aGlobal.GetAsSupports());

  if (!target) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<CallEvent> event = Create(target, aType, aOptions.mCall, false, false);

  return event.forget();
}

already_AddRefed<TelephonyCall>
CallEvent::GetCall() const
{
  nsRefPtr<TelephonyCall> call = mCall;
  return call.forget();
}
