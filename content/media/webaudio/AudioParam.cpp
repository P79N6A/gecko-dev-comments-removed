





#include "AudioParam.h"
#include "nsContentUtils.h"
#include "nsIDOMWindow.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/AudioParamBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(AudioParam)
  tmp->DisconnectFromGraphAndDestroyStream();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(AudioParam)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(AudioParam)

NS_IMPL_CYCLE_COLLECTING_NATIVE_ADDREF(AudioParam)

NS_IMETHODIMP_(nsrefcnt)
AudioParam::Release()
{
  if (mRefCnt.get() == 1) {
    
    
    DisconnectFromGraphAndDestroyStream();
  }
  NS_IMPL_CC_NATIVE_RELEASE_BODY(AudioParam)
}

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(AudioParam, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(AudioParam, Release)

AudioParam::AudioParam(AudioNode* aNode,
                       AudioParam::CallbackType aCallback,
                       float aDefaultValue)
  : AudioParamTimeline(aDefaultValue)
  , mNode(aNode)
  , mCallback(aCallback)
  , mDefaultValue(aDefaultValue)
{
  SetIsDOMBinding();
}

AudioParam::~AudioParam()
{
  MOZ_ASSERT(mInputNodes.IsEmpty());
}

JSObject*
AudioParam::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return AudioParamBinding::Wrap(aCx, aScope, this);
}

void
AudioParam::DisconnectFromGraphAndDestroyStream()
{
  
  
  nsRefPtr<AudioParam> kungFuDeathGrip = this;

  while (!mInputNodes.IsEmpty()) {
    uint32_t i = mInputNodes.Length() - 1;
    nsRefPtr<AudioNode> input = mInputNodes[i].mInputNode;
    mInputNodes.RemoveElementAt(i);
    input->RemoveOutputParam(this);
  }

  if (mNodeStreamPort) {
    mNodeStreamPort->Destroy();
    mNodeStreamPort = nullptr;
  }

  if (mStream) {
    mStream->Destroy();
    mStream = nullptr;
  }
}

MediaStream*
AudioParam::Stream()
{
  if (mStream) {
    return mStream;
  }

  AudioNodeEngine* engine = new AudioNodeEngine(nullptr);
  nsRefPtr<AudioNodeStream> stream = mNode->Context()->Graph()->CreateAudioNodeStream(engine, MediaStreamGraph::INTERNAL_STREAM);

  
  
  stream->SetChannelMixingParametersImpl(1, ChannelCountMode::Explicit, ChannelInterpretation::Speakers);
  
  stream->SetAudioParamHelperStream();

  mStream = stream.forget();

  
  MediaStream* nodeStream = mNode->Stream();
  MOZ_ASSERT(nodeStream->AsProcessedStream());
  ProcessedMediaStream* ps = static_cast<ProcessedMediaStream*>(nodeStream);
  mNodeStreamPort = ps->AllocateInputPort(mStream, MediaInputPort::FLAG_BLOCK_INPUT);

  
  mCallback(mNode);

  return mStream;
}

}
}

