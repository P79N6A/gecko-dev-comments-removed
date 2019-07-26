




#ifndef nsDOMMessageEvent_h__
#define nsDOMMessageEvent_h__

#include "nsIDOMMessageEvent.h"
#include "nsDOMEvent.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {
namespace dom {
class MessageEventInit;
class MessagePort;
class MessagePortBase;
class MessagePortList;
class OwningWindowProxyOrMessagePort;
}
}








class nsDOMMessageEvent : public nsDOMEvent,
                          public nsIDOMMessageEvent
{
public:
  nsDOMMessageEvent(mozilla::dom::EventTarget* aOwner,
                    nsPresContext* aPresContext,
                    mozilla::WidgetEvent* aEvent);
  ~nsDOMMessageEvent();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(nsDOMMessageEvent,
                                                         nsDOMEvent)

  NS_DECL_NSIDOMMESSAGEEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  JS::Value GetData(JSContext* aCx, mozilla::ErrorResult& aRv);

  void GetSource(Nullable<mozilla::dom::OwningWindowProxyOrMessagePort>& aValue) const;

  mozilla::dom::MessagePortList* GetPorts()
  {
    return mPorts;
  }

  void SetPorts(mozilla::dom::MessagePortList* aPorts);

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
  nsCOMPtr<mozilla::dom::MessagePortBase> mPortSource;
  nsRefPtr<mozilla::dom::MessagePortList> mPorts;
};

#endif 
