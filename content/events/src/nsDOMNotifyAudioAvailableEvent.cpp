





#include "nsError.h"
#include "nsDOMNotifyAudioAvailableEvent.h"
#include "nsDOMClassInfoID.h" 
#include "nsContentUtils.h" 
#include "jsfriendapi.h"

nsDOMNotifyAudioAvailableEvent::nsDOMNotifyAudioAvailableEvent(mozilla::dom::EventTarget* aOwner,
                                                               nsPresContext* aPresContext,
                                                               nsEvent* aEvent,
                                                               uint32_t aEventType,
                                                               float* aFrameBuffer,
                                                               uint32_t aFrameBufferLength,
                                                               float aTime)
  : nsDOMEvent(aOwner, aPresContext, aEvent),
    mFrameBuffer(aFrameBuffer),
    mFrameBufferLength(aFrameBufferLength),
    mTime(aTime),
    mCachedArray(nullptr),
    mAllowAudioData(false)
{
  MOZ_COUNT_CTOR(nsDOMNotifyAudioAvailableEvent);
  if (mEvent) {
    mEvent->message = aEventType;
  }
}

DOMCI_DATA(NotifyAudioAvailableEvent, nsDOMNotifyAudioAvailableEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMNotifyAudioAvailableEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMNotifyAudioAvailableEvent, nsDOMEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMNotifyAudioAvailableEvent, nsDOMEvent)
  if (tmp->mCachedArray) {
    tmp->mCachedArray = nullptr;
    NS_DROP_JS_OBJECTS(tmp, nsDOMNotifyAudioAvailableEvent);
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMNotifyAudioAvailableEvent, nsDOMEvent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(nsDOMNotifyAudioAvailableEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mCachedArray)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMNotifyAudioAvailableEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNotifyAudioAvailableEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(NotifyAudioAvailableEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

nsDOMNotifyAudioAvailableEvent::~nsDOMNotifyAudioAvailableEvent()
{
  MOZ_COUNT_DTOR(nsDOMNotifyAudioAvailableEvent);
  if (mCachedArray) {
    mCachedArray = nullptr;
    NS_DROP_JS_OBJECTS(this, nsDOMNotifyAudioAvailableEvent);
  }
}

NS_IMETHODIMP
nsDOMNotifyAudioAvailableEvent::GetFrameBuffer(JSContext* aCx, jsval* aResult)
{
  if (!mAllowAudioData) {
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (mCachedArray) {
    *aResult = OBJECT_TO_JSVAL(mCachedArray);
    return NS_OK;
  }

  
  NS_HOLD_JS_OBJECTS(this, nsDOMNotifyAudioAvailableEvent);

  mCachedArray = JS_NewFloat32Array(aCx, mFrameBufferLength);
  if (!mCachedArray) {
    NS_DROP_JS_OBJECTS(this, nsDOMNotifyAudioAvailableEvent);
    return NS_ERROR_FAILURE;
  }
  memcpy(JS_GetFloat32ArrayData(mCachedArray), mFrameBuffer.get(), mFrameBufferLength * sizeof(float));

  *aResult = OBJECT_TO_JSVAL(mCachedArray);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMNotifyAudioAvailableEvent::GetTime(float *aRetVal)
{
  *aRetVal = mTime;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMNotifyAudioAvailableEvent::InitAudioAvailableEvent(const nsAString& aType,
                                                        bool aCanBubble,
                                                        bool aCancelable,
                                                        float* aFrameBuffer,
                                                        uint32_t aFrameBufferLength,
                                                        float aTime,
                                                        bool aAllowAudioData)
{
  
  
  
  nsAutoArrayPtr<float> frameBuffer(aFrameBuffer);
  nsresult rv = nsDOMEvent::InitEvent(aType, aCanBubble, aCancelable);
  NS_ENSURE_SUCCESS(rv, rv);

  mFrameBuffer = frameBuffer.forget();
  mFrameBufferLength = aFrameBufferLength;
  mTime = aTime;
  mAllowAudioData = aAllowAudioData;
  return NS_OK;
}

nsresult NS_NewDOMAudioAvailableEvent(nsIDOMEvent** aInstancePtrResult,
                                      mozilla::dom::EventTarget* aOwner,
                                      nsPresContext* aPresContext,
                                      nsEvent *aEvent,
                                      uint32_t aEventType,
                                      float* aFrameBuffer,
                                      uint32_t aFrameBufferLength,
                                      float aTime)
{
  nsDOMNotifyAudioAvailableEvent* it =
    new nsDOMNotifyAudioAvailableEvent(aOwner, aPresContext, aEvent, aEventType,
                                       aFrameBuffer, aFrameBufferLength, aTime);
  return CallQueryInterface(it, aInstancePtrResult);
}
