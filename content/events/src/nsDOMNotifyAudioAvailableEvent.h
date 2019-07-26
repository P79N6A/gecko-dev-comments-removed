





#ifndef nsDOMNotifyAudioAvailableEvent_h_
#define nsDOMNotifyAudioAvailableEvent_h_

#include "nsIDOMNotifyAudioAvailableEvent.h"
#include "nsDOMEvent.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/NotifyAudioAvailableEventBinding.h"

class nsPresContext;

class nsDOMNotifyAudioAvailableEvent : public nsDOMEvent,
                                       public nsIDOMNotifyAudioAvailableEvent
{
public:
  nsDOMNotifyAudioAvailableEvent(mozilla::dom::EventTarget* aOwner,
                                 nsPresContext* aPresContext,
                                 mozilla::WidgetEvent* aEvent,
                                 uint32_t aEventType, float * aFrameBuffer,
                                 uint32_t aFrameBufferLength, float aTime);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(nsDOMNotifyAudioAvailableEvent,
                                                         nsDOMEvent)

  NS_DECL_NSIDOMNOTIFYAUDIOAVAILABLEEVENT
  NS_FORWARD_NSIDOMEVENT(nsDOMEvent::)

  nsresult NS_NewDOMAudioAvailableEvent(nsIDOMEvent** aInstancePtrResult,
                                        mozilla::dom::EventTarget* aOwner,
                                        nsPresContext* aPresContext,
                                        mozilla::WidgetEvent* aEvent,
                                        uint32_t aEventType,
                                        float * aFrameBuffer,
                                        uint32_t aFrameBufferLength,
                                        float aTime);

  ~nsDOMNotifyAudioAvailableEvent();

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return mozilla::dom::NotifyAudioAvailableEventBinding::Wrap(aCx, aScope, this);
  }

  JSObject* GetFrameBuffer(JSContext* aCx, mozilla::ErrorResult& aRv)
  {
    JS::Rooted<JS::Value> dummy(aCx);
    aRv = GetFrameBuffer(aCx, dummy.address());
    return mCachedArray;
  }

  float Time()
  {
    return mTime;
  }

  void InitAudioAvailableEvent(const nsAString& aType,
                               bool aCanBubble,
                               bool aCancelable,
                               const mozilla::dom::Nullable<mozilla::dom::Sequence<float> >& aFrameBuffer,
                               uint32_t aFrameBufferLength,
                               float aTime,
                               bool aAllowAudioData,
                               mozilla::ErrorResult& aRv);
private:
  nsAutoArrayPtr<float> mFrameBuffer;
  uint32_t mFrameBufferLength;
  float mTime;
  JS::Heap<JSObject*> mCachedArray;
  bool mAllowAudioData;
};

#endif 
