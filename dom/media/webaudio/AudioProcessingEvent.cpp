





#include "AudioProcessingEvent.h"
#include "mozilla/dom/AudioProcessingEventBinding.h"
#include "mozilla/dom/ScriptSettings.h"
#include "AudioContext.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED(AudioProcessingEvent, Event,
                                   mInputBuffer, mOutputBuffer, mNode)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(AudioProcessingEvent)
NS_INTERFACE_MAP_END_INHERITING(Event)

NS_IMPL_ADDREF_INHERITED(AudioProcessingEvent, Event)
NS_IMPL_RELEASE_INHERITED(AudioProcessingEvent, Event)

AudioProcessingEvent::AudioProcessingEvent(ScriptProcessorNode* aOwner,
                                           nsPresContext* aPresContext,
                                           WidgetEvent* aEvent)
  : Event(aOwner, aPresContext, aEvent)
  , mPlaybackTime(0.0)
  , mNode(aOwner)
{
}

AudioProcessingEvent::~AudioProcessingEvent()
{
}

JSObject*
AudioProcessingEvent::WrapObjectInternal(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return AudioProcessingEventBinding::Wrap(aCx, this, aGivenProto);
}

already_AddRefed<AudioBuffer>
AudioProcessingEvent::LazilyCreateBuffer(uint32_t aNumberOfChannels,
                                         ErrorResult& aRv)
{
  AutoJSAPI jsapi;
  if (NS_WARN_IF(!jsapi.Init(mNode->GetOwner()))) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }
  JSContext* cx = jsapi.cx();

  nsRefPtr<AudioBuffer> buffer =
    AudioBuffer::Create(mNode->Context(), aNumberOfChannels,
                        mNode->BufferSize(),
                        mNode->Context()->SampleRate(), cx, aRv);
  MOZ_ASSERT(buffer || aRv.ErrorCodeIs(NS_ERROR_OUT_OF_MEMORY));
  return buffer.forget();
}

}
}

