





#include "AudioNode.h"
#include "mozilla/ErrorResult.h"
#include "AudioNodeStream.h"
#include "AudioNodeEngine.h"
#include "mozilla/dom/AudioParam.h"
#include "mozilla/Services.h"
#include "nsIObserverService.h"

namespace mozilla {
namespace dom {

static const uint32_t INVALID_PORT = 0xffffffff;
static uint32_t gId = 0;

NS_IMPL_CYCLE_COLLECTION_CLASS(AudioNode)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(AudioNode, DOMEventTargetHelper)
  tmp->DisconnectFromGraph();
  if (tmp->mContext) {
    tmp->mContext->UpdateNodeCount(-1);
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mOutputNodes)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mOutputParams)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(AudioNode,
                                                  DOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOutputNodes)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOutputParams)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(AudioNode, DOMEventTargetHelper)

NS_IMETHODIMP_(MozExternalRefCountType)
AudioNode::Release()
{
  if (mRefCnt.get() == 1) {
    
    
    DisconnectFromGraph();
  }
  nsrefcnt r = DOMEventTargetHelper::Release();
  NS_LOG_RELEASE(this, r, "AudioNode");
  return r;
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(AudioNode)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

AudioNode::AudioNode(AudioContext* aContext,
                     uint32_t aChannelCount,
                     ChannelCountMode aChannelCountMode,
                     ChannelInterpretation aChannelInterpretation)
  : DOMEventTargetHelper(aContext->GetParentObject())
  , mContext(aContext)
  , mChannelCount(aChannelCount)
  , mChannelCountMode(aChannelCountMode)
  , mChannelInterpretation(aChannelInterpretation)
  , mId(gId++)
  , mPassThrough(false)
#ifdef DEBUG
  , mDemiseNotified(false)
#endif
{
  MOZ_ASSERT(aContext);
  DOMEventTargetHelper::BindToOwner(aContext->GetParentObject());
  aContext->UpdateNodeCount(1);
}

AudioNode::~AudioNode()
{
  MOZ_ASSERT(mInputNodes.IsEmpty());
  MOZ_ASSERT(mOutputNodes.IsEmpty());
  MOZ_ASSERT(mOutputParams.IsEmpty());
#ifdef DEBUG
  MOZ_ASSERT(mDemiseNotified,
             "The webaudio-node-demise notification must have been sent");
#endif
  if (mContext) {
    mContext->UpdateNodeCount(-1);
  }
}

size_t
AudioNode::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  
  
  
  size_t amount = 0;

  amount += mInputNodes.SizeOfExcludingThis(aMallocSizeOf);
  for (size_t i = 0; i < mInputNodes.Length(); i++) {
    amount += mInputNodes[i].SizeOfExcludingThis(aMallocSizeOf);
  }

  
  
  amount += mOutputNodes.SizeOfExcludingThis(aMallocSizeOf);

  amount += mOutputParams.SizeOfExcludingThis(aMallocSizeOf);
  for (size_t i = 0; i < mOutputParams.Length(); i++) {
    amount += mOutputParams[i]->SizeOfIncludingThis(aMallocSizeOf);
  }

  return amount;
}

size_t
AudioNode::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

template <class InputNode>
static size_t
FindIndexOfNode(const nsTArray<InputNode>& aInputNodes, const AudioNode* aNode)
{
  for (size_t i = 0; i < aInputNodes.Length(); ++i) {
    if (aInputNodes[i].mInputNode == aNode) {
      return i;
    }
  }
  return nsTArray<InputNode>::NoIndex;
}

template <class InputNode>
static size_t
FindIndexOfNodeWithPorts(const nsTArray<InputNode>& aInputNodes, const AudioNode* aNode,
                         uint32_t aInputPort, uint32_t aOutputPort)
{
  for (size_t i = 0; i < aInputNodes.Length(); ++i) {
    if (aInputNodes[i].mInputNode == aNode &&
        aInputNodes[i].mInputPort == aInputPort &&
        aInputNodes[i].mOutputPort == aOutputPort) {
      return i;
    }
  }
  return nsTArray<InputNode>::NoIndex;
}

void
AudioNode::DisconnectFromGraph()
{
  
  
  nsRefPtr<AudioNode> kungFuDeathGrip = this;

  
  

  
  while (!mInputNodes.IsEmpty()) {
    size_t i = mInputNodes.Length() - 1;
    nsRefPtr<AudioNode> input = mInputNodes[i].mInputNode;
    mInputNodes.RemoveElementAt(i);
    input->mOutputNodes.RemoveElement(this);
  }

  while (!mOutputNodes.IsEmpty()) {
    size_t i = mOutputNodes.Length() - 1;
    nsRefPtr<AudioNode> output = mOutputNodes[i].forget();
    mOutputNodes.RemoveElementAt(i);
    size_t inputIndex = FindIndexOfNode(output->mInputNodes, this);
    
    
    output->mInputNodes.RemoveElementAt(inputIndex);
  }

  while (!mOutputParams.IsEmpty()) {
    size_t i = mOutputParams.Length() - 1;
    nsRefPtr<AudioParam> output = mOutputParams[i].forget();
    mOutputParams.RemoveElementAt(i);
    size_t inputIndex = FindIndexOfNode(output->InputNodes(), this);
    
    
    output->RemoveInputNode(inputIndex);
  }

  DestroyMediaStream();
}

void
AudioNode::Connect(AudioNode& aDestination, uint32_t aOutput,
                   uint32_t aInput, ErrorResult& aRv)
{
  if (aOutput >= NumberOfOutputs() ||
      aInput >= aDestination.NumberOfInputs()) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  if (Context() != aDestination.Context()) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }

  if (FindIndexOfNodeWithPorts(aDestination.mInputNodes, this, aInput, aOutput) !=
      nsTArray<AudioNode::InputNode>::NoIndex) {
    
    return;
  }

  
  

  mOutputNodes.AppendElement(&aDestination);
  InputNode* input = aDestination.mInputNodes.AppendElement();
  input->mInputNode = this;
  input->mInputPort = aInput;
  input->mOutputPort = aOutput;
  AudioNodeStream* destinationStream = aDestination.mStream;
  if (mStream && destinationStream) {
    
    MOZ_ASSERT(aInput <= UINT16_MAX, "Unexpected large input port number");
    MOZ_ASSERT(aOutput <= UINT16_MAX, "Unexpected large output port number");
    input->mStreamPort = destinationStream->
      AllocateInputPort(mStream, MediaInputPort::FLAG_BLOCK_INPUT,
                            static_cast<uint16_t>(aInput),
                            static_cast<uint16_t>(aOutput));
  }

  
  Context()->UpdatePannerSource();
}

void
AudioNode::Connect(AudioParam& aDestination, uint32_t aOutput,
                   ErrorResult& aRv)
{
  if (aOutput >= NumberOfOutputs()) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  if (Context() != aDestination.GetParentObject()) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }

  if (FindIndexOfNodeWithPorts(aDestination.InputNodes(), this, INVALID_PORT, aOutput) !=
      nsTArray<AudioNode::InputNode>::NoIndex) {
    
    return;
  }

  mOutputParams.AppendElement(&aDestination);
  InputNode* input = aDestination.AppendInputNode();
  input->mInputNode = this;
  input->mInputPort = INVALID_PORT;
  input->mOutputPort = aOutput;

  MediaStream* stream = aDestination.Stream();
  MOZ_ASSERT(stream->AsProcessedStream());
  ProcessedMediaStream* ps = static_cast<ProcessedMediaStream*>(stream);
  if (mStream) {
    
    MOZ_ASSERT(aOutput <= UINT16_MAX, "Unexpected large output port number");
    input->mStreamPort =
      ps->AllocateInputPort(mStream, MediaInputPort::FLAG_BLOCK_INPUT,
                            0, static_cast<uint16_t>(aOutput));
  }
}

void
AudioNode::SendDoubleParameterToStream(uint32_t aIndex, double aValue)
{
  MOZ_ASSERT(mStream, "How come we don't have a stream here?");
  mStream->SetDoubleParameter(aIndex, aValue);
}

void
AudioNode::SendInt32ParameterToStream(uint32_t aIndex, int32_t aValue)
{
  MOZ_ASSERT(mStream, "How come we don't have a stream here?");
  mStream->SetInt32Parameter(aIndex, aValue);
}

void
AudioNode::SendThreeDPointParameterToStream(uint32_t aIndex, const ThreeDPoint& aValue)
{
  MOZ_ASSERT(mStream, "How come we don't have a stream here?");
  mStream->SetThreeDPointParameter(aIndex, aValue);
}

void
AudioNode::SendChannelMixingParametersToStream()
{
  MOZ_ASSERT(mStream, "How come we don't have a stream here?");
  mStream->SetChannelMixingParameters(mChannelCount, mChannelCountMode,
                                 mChannelInterpretation);
}

void
AudioNode::SendTimelineParameterToStream(AudioNode* aNode, uint32_t aIndex,
                                         const AudioParamTimeline& aValue)
{
  AudioNodeStream* ns = aNode->mStream;
  MOZ_ASSERT(ns, "How come we don't have a stream here?");
  ns->SetTimelineParameter(aIndex, aValue);
}

void
AudioNode::Disconnect(uint32_t aOutput, ErrorResult& aRv)
{
  if (aOutput >= NumberOfOutputs()) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  
  
  
  
  
  class RunnableRelease final : public nsRunnable
  {
  public:
    explicit RunnableRelease(already_AddRefed<AudioNode> aNode)
      : mNode(aNode) {}

    NS_IMETHODIMP Run() override
    {
      mNode = nullptr;
      return NS_OK;
    }
  private:
    nsRefPtr<AudioNode> mNode;
  };

  for (int32_t i = mOutputNodes.Length() - 1; i >= 0; --i) {
    AudioNode* dest = mOutputNodes[i];
    for (int32_t j = dest->mInputNodes.Length() - 1; j >= 0; --j) {
      InputNode& input = dest->mInputNodes[j];
      if (input.mInputNode == this && input.mOutputPort == aOutput) {
        
        
        
        dest->mInputNodes.RemoveElementAt(j);
        
        
        
        nsRefPtr<AudioNode> output = mOutputNodes[i].forget();
        mOutputNodes.RemoveElementAt(i);
        if (mStream) {
          nsRefPtr<nsIRunnable> runnable = new RunnableRelease(output.forget());
          mStream->RunAfterPendingUpdates(runnable.forget());
        }
        break;
      }
    }
  }

  for (int32_t i = mOutputParams.Length() - 1; i >= 0; --i) {
    AudioParam* dest = mOutputParams[i];
    for (int32_t j = dest->InputNodes().Length() - 1; j >= 0; --j) {
      const InputNode& input = dest->InputNodes()[j];
      if (input.mInputNode == this && input.mOutputPort == aOutput) {
        dest->RemoveInputNode(j);
        
        
        
        mOutputParams.RemoveElementAt(i);
        break;
      }
    }
  }

  
  Context()->UpdatePannerSource();
}

void
AudioNode::DestroyMediaStream()
{
  if (mStream) {
    
    AudioNodeStream* ns = mStream;
    MOZ_ASSERT(ns, "How come we don't have a stream here?");
    MOZ_ASSERT(ns->Engine()->NodeMainThread() == this,
               "Invalid node reference");
    ns->Engine()->ClearNode();

    mStream->Destroy();
    mStream = nullptr;

    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    if (obs) {
      nsAutoString id;
      id.AppendPrintf("%u", mId);
      obs->NotifyObservers(nullptr, "webaudio-node-demise", id.get());
    }
#ifdef DEBUG
    mDemiseNotified = true;
#endif
  }
}

void
AudioNode::RemoveOutputParam(AudioParam* aParam)
{
  mOutputParams.RemoveElement(aParam);
}

bool
AudioNode::PassThrough() const
{
  MOZ_ASSERT(NumberOfInputs() <= 1 && NumberOfOutputs() == 1);
  return mPassThrough;
}

void
AudioNode::SetPassThrough(bool aPassThrough)
{
  MOZ_ASSERT(NumberOfInputs() <= 1 && NumberOfOutputs() == 1);
  mPassThrough = aPassThrough;
  MOZ_ASSERT(mStream, "How come we don't have a stream here?");
  mStream->SetPassThrough(mPassThrough);
}

} 
} 
