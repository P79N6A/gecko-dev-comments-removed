





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
  SetIsDOMBinding();
}

JSObject*
AudioProcessingEvent::WrapObject(JSContext* aCx)
{
  return AudioProcessingEventBinding::Wrap(aCx, this);
}

void
AudioProcessingEvent::LazilyCreateBuffer(nsRefPtr<AudioBuffer>& aBuffer,
                                         uint32_t aNumberOfChannels)
{
  
  JSObject* global = mNode->Context()->GetGlobalJSObject();
  if (NS_WARN_IF(!global)) {
    return;
  }

  AutoJSAPI jsapi;
  JSContext* cx = jsapi.cx();
  JSAutoCompartment ac(cx, global);

  aBuffer = new AudioBuffer(mNode->Context(), mNode->BufferSize(),
                            mNode->Context()->SampleRate());
  aBuffer->InitializeBuffers(aNumberOfChannels, cx);
}

}
}

