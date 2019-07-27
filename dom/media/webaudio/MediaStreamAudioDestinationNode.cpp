





#include "MediaStreamAudioDestinationNode.h"
#include "nsIDocument.h"
#include "mozilla/dom/MediaStreamAudioDestinationNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "DOMMediaStream.h"
#include "TrackUnionStream.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED(MediaStreamAudioDestinationNode, AudioNode, mDOMStream)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MediaStreamAudioDestinationNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(MediaStreamAudioDestinationNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(MediaStreamAudioDestinationNode, AudioNode)

MediaStreamAudioDestinationNode::MediaStreamAudioDestinationNode(AudioContext* aContext)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Explicit,
              ChannelInterpretation::Speakers)
  , mDOMStream(DOMAudioNodeMediaStream::CreateTrackUnionStream(GetOwner(), this))
{
  
  mDOMStream->CreateDOMTrack(AudioNodeStream::AUDIO_TRACK, MediaSegment::AUDIO);

  ProcessedMediaStream* outputStream = mDOMStream->GetStream()->AsProcessedStream();
  MOZ_ASSERT(!!outputStream);
  AudioNodeEngine* engine = new AudioNodeEngine(this);
  mStream = aContext->Graph()->CreateAudioNodeStream(engine, MediaStreamGraph::EXTERNAL_STREAM);
  mPort = outputStream->AllocateInputPort(mStream);

  nsIDocument* doc = aContext->GetParentObject()->GetExtantDoc();
  if (doc) {
    mDOMStream->CombineWithPrincipal(doc->NodePrincipal());
  }
}

MediaStreamAudioDestinationNode::~MediaStreamAudioDestinationNode()
{
}

size_t
MediaStreamAudioDestinationNode::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  
  
  size_t amount = AudioNode::SizeOfExcludingThis(aMallocSizeOf);
  amount += mPort->SizeOfIncludingThis(aMallocSizeOf);
  return amount;
}

size_t
MediaStreamAudioDestinationNode::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

void
MediaStreamAudioDestinationNode::DestroyMediaStream()
{
  AudioNode::DestroyMediaStream();
  if (mPort) {
    mPort->Destroy();
    mPort = nullptr;
  }
}

JSObject*
MediaStreamAudioDestinationNode::WrapObject(JSContext* aCx)
{
  return MediaStreamAudioDestinationNodeBinding::Wrap(aCx, this);
}

}
}
