




#ifndef mozilla_dom_MessageEvent_h_
#define mozilla_dom_MessageEvent_h_

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








class MessageEvent : public nsDOMEvent,
                     public nsIDOMMessageEvent
{
public:
  MessageEvent(EventTarget* aOwner,
               nsPresContext* aPresContext,
               WidgetEvent* aEvent);
  ~MessageEvent();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(MessageEvent,
                                                         nsDOMEvent)

  NS_DECL_NSIDOMMESSAGEEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  JS::Value GetData(JSContext* aCx, ErrorResult& aRv);

  void GetSource(Nullable<OwningWindowProxyOrMessagePort>& aValue) const;

  MessagePortList* GetPorts()
  {
    return mPorts;
  }

  void SetPorts(MessagePortList* aPorts);

  static already_AddRefed<MessageEvent>
  Constructor(const GlobalObject& aGlobal, JSContext* aCx,
              const nsAString& aType,
              const MessageEventInit& aEventInit,
              ErrorResult& aRv);

private:
  JS::Heap<JS::Value> mData;
  nsString mOrigin;
  nsString mLastEventId;
  nsCOMPtr<nsIDOMWindow> mWindowSource;
  nsRefPtr<MessagePortBase> mPortSource;
  nsRefPtr<MessagePortList> mPorts;
};

} 
} 

#endif 
