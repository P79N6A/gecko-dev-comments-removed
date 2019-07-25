






































#include "nsDOMNotifyAudioAvailableEvent.h"
#include "jstypedarray.h"

nsDOMNotifyAudioAvailableEvent::nsDOMNotifyAudioAvailableEvent(nsPresContext* aPresContext,
                                                               nsEvent* aEvent,
                                                               PRUint32 aEventType,
                                                               float* aFrameBuffer,
                                                               PRUint32 aFrameBufferLength,
                                                               float aTime)
  : nsDOMEvent(aPresContext, aEvent),
    mFrameBuffer(aFrameBuffer),
    mFrameBufferLength(aFrameBufferLength),
    mTime(aTime),
    mCachedArray(nsnull),
    mAllowAudioData(PR_FALSE)
{
  MOZ_COUNT_CTOR(nsDOMNotifyAudioAvailableEvent);
  if (mEvent) {
    mEvent->message = aEventType;
  }
}

DOMCI_DATA(NotifyAudioAvailableEvent, nsDOMNotifyAudioAvailableEvent)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMNotifyAudioAvailableEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMNotifyAudioAvailableEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMNotifyAudioAvailableEvent, nsDOMEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMNotifyAudioAvailableEvent, nsDOMEvent)
  if (tmp->mCachedArray) {
    NS_DROP_JS_OBJECTS(tmp, nsDOMNotifyAudioAvailableEvent);
    tmp->mCachedArray = nsnull;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMNotifyAudioAvailableEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsDOMNotifyAudioAvailableEvent)
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
    NS_DROP_JS_OBJECTS(this, nsDOMNotifyAudioAvailableEvent);
    mCachedArray = nsnull;
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

  mCachedArray = js_CreateTypedArray(aCx, js::TypedArray::TYPE_FLOAT32, mFrameBufferLength);
  if (!mCachedArray) {
    NS_DROP_JS_OBJECTS(this, nsDOMNotifyAudioAvailableEvent);
    NS_ERROR("Failed to get audio signal!");
    return NS_ERROR_FAILURE;
  }

  js::TypedArray *tdest = js::TypedArray::fromJSObject(mCachedArray);
  memcpy(tdest->data, mFrameBuffer.get(), mFrameBufferLength * sizeof(float));

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
                                                        PRBool aCanBubble,
                                                        PRBool aCancelable,
                                                        float* aFrameBuffer,
                                                        PRUint32 aFrameBufferLength,
                                                        float aTime,
                                                        PRBool aAllowAudioData)
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
                                      nsPresContext* aPresContext,
                                      nsEvent *aEvent,
                                      PRUint32 aEventType,
                                      float* aFrameBuffer,
                                      PRUint32 aFrameBufferLength,
                                      float aTime)
{
  nsDOMNotifyAudioAvailableEvent* it =
    new nsDOMNotifyAudioAvailableEvent(aPresContext, aEvent, aEventType,
                                       aFrameBuffer, aFrameBufferLength, aTime);
  return CallQueryInterface(it, aInstancePtrResult);
}
