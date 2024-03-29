





#ifndef mozilla_dom_MessageEvent_h_
#define mozilla_dom_MessageEvent_h_

#include "mozilla/dom/Event.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMMessageEvent.h"
#include "mozilla/dom/MessagePortList.h"

namespace mozilla {
namespace dom {

struct MessageEventInit;
class MessagePort;
class MessagePortBase;
class MessagePortList;
class OwningWindowProxyOrMessagePortOrClient;

namespace workers {

class ServiceWorkerClient;

} 








class MessageEvent final : public Event,
                           public nsIDOMMessageEvent
{
public:
  MessageEvent(EventTarget* aOwner,
               nsPresContext* aPresContext,
               WidgetEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(MessageEvent, Event)

  NS_DECL_NSIDOMMESSAGEEVENT

  
  NS_FORWARD_TO_EVENT

  virtual JSObject* WrapObjectInternal(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  void GetData(JSContext* aCx, JS::MutableHandle<JS::Value> aData,
               ErrorResult& aRv);

  void GetSource(Nullable<OwningWindowProxyOrMessagePortOrClient>& aValue) const;

  MessagePortList* GetPorts()
  {
    return mPorts;
  }

  void SetPorts(MessagePortList* aPorts);

  
  void SetSource(mozilla::dom::MessagePort* aPort);

  void SetSource(workers::ServiceWorkerClient* aClient);

  void SetSource(nsPIDOMWindow* aWindow)
  {
    mWindowSource = aWindow;
  }

  static already_AddRefed<MessageEvent>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const MessageEventInit& aEventInit,
              ErrorResult& aRv);

  static already_AddRefed<MessageEvent>
  Constructor(EventTarget* aEventTarget,
              const nsAString& aType,
              const MessageEventInit& aEventInit,
              ErrorResult& aRv);

protected:
  ~MessageEvent();

private:
  JS::Heap<JS::Value> mData;
  nsString mOrigin;
  nsString mLastEventId;
  nsCOMPtr<nsIDOMWindow> mWindowSource;
  nsRefPtr<MessagePortBase> mPortSource;
  nsRefPtr<workers::ServiceWorkerClient> mClientSource;
  nsRefPtr<MessagePortList> mPorts;
};

} 
} 

#endif 
