




#ifndef nsDOMMessageEvent_h__
#define nsDOMMessageEvent_h__

#include "nsIDOMMessageEvent.h"
#include "nsDOMEvent.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/MessageEventBinding.h"

namespace mozilla {
namespace dom {
class MessagePortList;
class WindowProxyOrMessagePortReturnValue;
}
}








class nsDOMMessageEvent : public nsDOMEvent,
                          public nsIDOMMessageEvent
{
public:
  nsDOMMessageEvent(mozilla::dom::EventTarget* aOwner,
                    nsPresContext* aPresContext, nsEvent* aEvent);
  ~nsDOMMessageEvent();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(nsDOMMessageEvent,
                                                         nsDOMEvent)

  NS_DECL_NSIDOMMESSAGEEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return mozilla::dom::MessageEventBinding::Wrap(aCx, aScope, this);
  }

  JS::Value GetData(JSContext* aCx, mozilla::ErrorResult& aRv);

  void GetSource(Nullable<mozilla::dom::WindowProxyOrMessagePortReturnValue>& aValue) const;

  mozilla::dom::MessagePortList* GetPorts()
  {
    return mPorts;
  }

  static already_AddRefed<nsDOMMessageEvent>
  Constructor(const mozilla::dom::GlobalObject& aGlobal, JSContext* aCx,
              const nsAString& aType,
              const mozilla::dom::MessageEventInit& aEventInit,
              mozilla::ErrorResult& aRv);

private:
  JS::Heap<JS::Value> mData;
  nsString mOrigin;
  nsString mLastEventId;
  nsCOMPtr<nsIDOMWindow> mWindowSource;
  nsCOMPtr<mozilla::dom::MessagePort> mPortSource;
  nsRefPtr<mozilla::dom::MessagePortList> mPorts;
};

#endif 
