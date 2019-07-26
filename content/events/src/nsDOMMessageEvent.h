




#ifndef nsDOMMessageEvent_h__
#define nsDOMMessageEvent_h__

#include "nsIDOMMessageEvent.h"
#include "nsDOMEvent.h"
#include "nsCycleCollectionParticipant.h"
#include "jsapi.h"
#include "mozilla/dom/MessageEventBinding.h"








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

  already_AddRefed<nsIDOMWindow> GetSource()
  {
    nsCOMPtr<nsIDOMWindow> ret = mSource;
    return ret.forget();
  }

  void InitMessageEvent(JSContext* aCx,
                        const nsAString& aType,
                        bool aCanBubble,
                        bool aCancelable,
                        JS::Handle<JS::Value> aData,
                        const nsAString& aOrigin,
                        const nsAString& aLastEventId,
                        nsIDOMWindow* aSource,
                        mozilla::ErrorResult& aRv)
  {
    aRv = InitMessageEvent(aType, aCanBubble, aCancelable, aData,
                           aOrigin, aLastEventId, aSource);
  }

private:
  JS::Heap<JS::Value> mData;
  nsString mOrigin;
  nsString mLastEventId;
  nsCOMPtr<nsIDOMWindow> mSource;
};

#endif 
