





#ifndef mozilla_dom_NotifyAudioAvailableEvent_h_
#define mozilla_dom_NotifyAudioAvailableEvent_h_

#include "mozilla/dom/Event.h"
#include "mozilla/dom/NotifyAudioAvailableEventBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMNotifyAudioAvailableEvent.h"

class nsPresContext;

namespace mozilla {
namespace dom {

class NotifyAudioAvailableEvent : public Event,
                                  public nsIDOMNotifyAudioAvailableEvent
{
public:
  NotifyAudioAvailableEvent(EventTarget* aOwner,
                            nsPresContext* aPresContext,
                            WidgetEvent* aEvent,
                            uint32_t aEventType,
                            float* aFrameBuffer,
                            uint32_t aFrameBufferLength,
                            float aTime);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(
    NotifyAudioAvailableEvent, Event)

  NS_DECL_NSIDOMNOTIFYAUDIOAVAILABLEEVENT
  NS_FORWARD_NSIDOMEVENT(Event::)

  ~NotifyAudioAvailableEvent();

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return NotifyAudioAvailableEventBinding::Wrap(aCx, aScope, this);
  }

  JSObject* GetFrameBuffer(JSContext* aCx, ErrorResult& aRv)
  {
    JS::Rooted<JS::Value> dummy(aCx);
    aRv = GetFrameBuffer(aCx, &dummy);
    return mCachedArray;
  }

  float Time()
  {
    return mTime;
  }

  void InitAudioAvailableEvent(const nsAString& aType,
                               bool aCanBubble,
                               bool aCancelable,
                               const Nullable<Sequence<float> >& aFrameBuffer,
                               uint32_t aFrameBufferLength,
                               float aTime,
                               bool aAllowAudioData,
                               ErrorResult& aRv);
private:
  nsAutoArrayPtr<float> mFrameBuffer;
  uint32_t mFrameBufferLength;
  float mTime;
  JS::Heap<JSObject*> mCachedArray;
  bool mAllowAudioData;
};

} 
} 

#endif 
