





#include "mozilla/dom/NotifyAudioAvailableEvent.h"
#include "mozilla/HoldDropJSObjects.h"
#include "nsError.h"
#include "jsfriendapi.h"

namespace mozilla {
namespace dom {

NotifyAudioAvailableEvent::NotifyAudioAvailableEvent(
                             EventTarget* aOwner,
                             nsPresContext* aPresContext,
                             WidgetEvent* aEvent,
                             uint32_t aEventType,
                             float* aFrameBuffer,
                             uint32_t aFrameBufferLength,
                             float aTime)
  : Event(aOwner, aPresContext, aEvent)
  , mFrameBuffer(aFrameBuffer)
  , mFrameBufferLength(aFrameBufferLength)
  , mTime(aTime)
  , mCachedArray(nullptr)
  , mAllowAudioData(false)
{
  MOZ_COUNT_CTOR(NotifyAudioAvailableEvent);
  if (mEvent) {
    mEvent->message = aEventType;
  }
}

NS_IMPL_ADDREF_INHERITED(NotifyAudioAvailableEvent, Event)
NS_IMPL_RELEASE_INHERITED(NotifyAudioAvailableEvent, Event)

NS_IMPL_CYCLE_COLLECTION_CLASS(NotifyAudioAvailableEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(NotifyAudioAvailableEvent,
                                                Event)
  if (tmp->mCachedArray) {
    tmp->mCachedArray = nullptr;
    DropJSObjects(tmp);
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(NotifyAudioAvailableEvent,
                                                  Event)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(NotifyAudioAvailableEvent,
                                               Event)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mCachedArray)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(NotifyAudioAvailableEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNotifyAudioAvailableEvent)
NS_INTERFACE_MAP_END_INHERITING(Event)

NotifyAudioAvailableEvent::~NotifyAudioAvailableEvent()
{
  MOZ_COUNT_DTOR(NotifyAudioAvailableEvent);
  if (mCachedArray) {
    mCachedArray = nullptr;
    mozilla::DropJSObjects(this);
  }
}

NS_IMETHODIMP
NotifyAudioAvailableEvent::GetFrameBuffer(JSContext* aCx,
                                          JS::MutableHandle<JS::Value> aResult)
{
  if (!mAllowAudioData) {
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (mCachedArray) {
    aResult.setObject(*mCachedArray);
    return NS_OK;
  }

  
  mozilla::HoldJSObjects(this);

  mCachedArray = JS_NewFloat32Array(aCx, mFrameBufferLength);
  if (!mCachedArray) {
    mozilla::DropJSObjects(this);
    return NS_ERROR_FAILURE;
  }
  memcpy(JS_GetFloat32ArrayData(mCachedArray), mFrameBuffer.get(),
         mFrameBufferLength * sizeof(float));

  aResult.setObject(*mCachedArray);
  return NS_OK;
}

NS_IMETHODIMP
NotifyAudioAvailableEvent::GetTime(float* aRetVal)
{
  *aRetVal = Time();
  return NS_OK;
}

NS_IMETHODIMP
NotifyAudioAvailableEvent::InitAudioAvailableEvent(const nsAString& aType,
                                                   bool aCanBubble,
                                                   bool aCancelable,
                                                   float* aFrameBuffer,
                                                   uint32_t aFrameBufferLength,
                                                   float aTime,
                                                   bool aAllowAudioData)
{
  
  
  
  nsAutoArrayPtr<float> frameBuffer(aFrameBuffer);
  nsresult rv = Event::InitEvent(aType, aCanBubble, aCancelable);
  NS_ENSURE_SUCCESS(rv, rv);

  mFrameBuffer = frameBuffer.forget();
  mFrameBufferLength = aFrameBufferLength;
  mTime = aTime;
  mAllowAudioData = aAllowAudioData;
  mCachedArray = nullptr;
  return NS_OK;
}

void
NotifyAudioAvailableEvent::InitAudioAvailableEvent(
                             const nsAString& aType,
                             bool aCanBubble,
                             bool aCancelable,
                             const Nullable<Sequence<float> >& aFrameBuffer,
                             uint32_t aFrameBufferLength,
                             float aTime,
                             bool aAllowAudioData,
                             ErrorResult& aRv)
{
  if ((aFrameBuffer.IsNull() && aFrameBufferLength > 0) ||
      (!aFrameBuffer.IsNull() &&
       aFrameBuffer.Value().Length() < aFrameBufferLength)) {
    aRv = NS_ERROR_UNEXPECTED;
    return;
  }

  nsAutoArrayPtr<float> buffer;
  if (!aFrameBuffer.IsNull()) {
    buffer = new float[aFrameBufferLength];
    memcpy(buffer.get(), aFrameBuffer.Value().Elements(),
           aFrameBufferLength * sizeof(float));
  }

  aRv = InitAudioAvailableEvent(aType, aCanBubble, aCancelable,
                                buffer.forget(),
                                aFrameBufferLength,
                                aTime, aAllowAudioData);
}

} 
} 

using namespace mozilla;
using namespace mozilla::dom;

nsresult
NS_NewDOMAudioAvailableEvent(nsIDOMEvent** aInstancePtrResult,
                             EventTarget* aOwner,
                             nsPresContext* aPresContext,
                             WidgetEvent* aEvent,
                             uint32_t aEventType,
                             float* aFrameBuffer,
                             uint32_t aFrameBufferLength,
                             float aTime)
{
  NotifyAudioAvailableEvent* it =
    new NotifyAudioAvailableEvent(aOwner, aPresContext, aEvent, aEventType,
                                  aFrameBuffer, aFrameBufferLength, aTime);
  NS_ADDREF(it);
  *aInstancePtrResult = static_cast<Event*>(it);
  return NS_OK;
}
