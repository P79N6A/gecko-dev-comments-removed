





#include "MediaStreamAudioSourceNode.h"
#include "mozilla/dom/MediaStreamAudioSourceNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeExternalInputStream.h"
#include "nsIDocument.h"
#include "mozilla/CORSMode.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(MediaStreamAudioSourceNode)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(MediaStreamAudioSourceNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mInputStream)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END_INHERITED(AudioNode)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(MediaStreamAudioSourceNode, AudioNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mInputStream)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MediaStreamAudioSourceNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(MediaStreamAudioSourceNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(MediaStreamAudioSourceNode, AudioNode)

MediaStreamAudioSourceNode::MediaStreamAudioSourceNode(AudioContext* aContext,
                                                       DOMMediaStream* aMediaStream)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Max,
              ChannelInterpretation::Speakers),
    mInputStream(aMediaStream)
{
  AudioNodeEngine* engine = new MediaStreamAudioSourceNodeEngine(this);
  mStream = aContext->Graph()->CreateAudioNodeExternalInputStream(engine);
  ProcessedMediaStream* outputStream = static_cast<ProcessedMediaStream*>(mStream.get());
  mInputPort = outputStream->AllocateInputPort(aMediaStream->GetStream(),
                                               MediaInputPort::FLAG_BLOCK_INPUT);
  mInputStream->AddConsumerToKeepAlive(static_cast<nsIDOMEventTarget*>(this));

  PrincipalChanged(mInputStream); 
  mInputStream->AddPrincipalChangeObserver(this);
}

MediaStreamAudioSourceNode::~MediaStreamAudioSourceNode()
{
  if (mInputStream) {
    mInputStream->RemovePrincipalChangeObserver(this);
  }
}















void
MediaStreamAudioSourceNode::PrincipalChanged(DOMMediaStream* aDOMMediaStream)
{
  bool subsumes = false;
  nsPIDOMWindow* parent = Context()->GetParentObject();
  if (parent) {
    nsIDocument* doc = parent->GetExtantDoc();
    if (doc) {
      nsIPrincipal* docPrincipal = doc->NodePrincipal();
      nsIPrincipal* streamPrincipal = mInputStream->GetPrincipal();
      if (!streamPrincipal || NS_FAILED(docPrincipal->Subsumes(streamPrincipal, &subsumes))) {
        subsumes = false;
      }
    }
  }
  auto stream = static_cast<AudioNodeExternalInputStream*>(mStream.get());
  stream->SetInt32Parameter(MediaStreamAudioSourceNodeEngine::ENABLE,
                            subsumes || aDOMMediaStream->GetCORSMode() != CORS_NONE);
}

size_t
MediaStreamAudioSourceNode::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  
  
  size_t amount = AudioNode::SizeOfExcludingThis(aMallocSizeOf);
  amount += mInputPort->SizeOfIncludingThis(aMallocSizeOf);
  return amount;
}

size_t
MediaStreamAudioSourceNode::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
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
MediaStreamAudioSourceNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return MediaStreamAudioSourceNodeBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
