





#include "CallEvent.h"
#include "mozilla/dom/CallEventBinding.h"

#include "TelephonyCall.h"

USING_TELEPHONY_NAMESPACE
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
CallEvent::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return CallEventBinding::Wrap(aCx, aScope, this);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(CallEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(CallEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mCall)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(CallEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCall)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(CallEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(CallEvent, nsDOMEvent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(CallEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)




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
