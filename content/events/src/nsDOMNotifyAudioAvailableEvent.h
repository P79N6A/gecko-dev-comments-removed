





#ifndef nsDOMNotifyAudioAvailableEvent_h_
#define nsDOMNotifyAudioAvailableEvent_h_

#include "nsIDOMNotifyAudioAvailableEvent.h"
#include "nsDOMEvent.h"
#include "nsPresContext.h"
#include "nsCycleCollectionParticipant.h"

class nsDOMNotifyAudioAvailableEvent : public nsDOMEvent,
                                       public nsIDOMNotifyAudioAvailableEvent
{
public:
  nsDOMNotifyAudioAvailableEvent(mozilla::dom::EventTarget* aOwner,
                                 nsPresContext* aPresContext, nsEvent* aEvent,
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
                                        nsEvent *aEvent,
                                        uint32_t aEventType,
                                        float * aFrameBuffer,
                                        uint32_t aFrameBufferLength,
                                        float aTime);

  ~nsDOMNotifyAudioAvailableEvent();

private:
  nsAutoArrayPtr<float> mFrameBuffer;
  uint32_t mFrameBufferLength;
  float mTime;
  JSObject* mCachedArray;
  bool mAllowAudioData;
};

#endif 
