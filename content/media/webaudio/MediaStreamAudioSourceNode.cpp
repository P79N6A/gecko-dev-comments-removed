





#include "MediaStreamAudioSourceNode.h"
#include "mozilla/dom/MediaStreamAudioSourceNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeExternalInputStream.h"
#include "DOMMediaStream.h"

namespace mozilla {
namespace dom {

MediaStreamAudioSourceNode::MediaStreamAudioSourceNode(AudioContext* aContext,
                                                       const DOMMediaStream* aMediaStream)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Max,
              ChannelInterpretation::Speakers)
{
  AudioNodeEngine* engine = new AudioNodeEngine(this);
  mStream = aContext->Graph()->CreateAudioNodeExternalInputStream(engine);
  ProcessedMediaStream* outputStream = static_cast<ProcessedMediaStream*>(mStream.get());
  mInputPort = outputStream->AllocateInputPort(aMediaStream->GetStream(),
                                               MediaInputPort::FLAG_BLOCK_INPUT);
}

void
MediaStreamAudioSourceNode::DestroyMediaStream()
{
  if (mInputPort) {
    mInputPort->Destroy();
    mInputPort = nullptr;
  }
  AudioNode::DestroyMediaStream();
}

JSObject*
MediaStreamAudioSourceNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return MediaStreamAudioSourceNodeBinding::Wrap(aCx, aScope, this);
}

}
}

