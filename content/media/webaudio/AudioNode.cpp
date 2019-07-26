





#include "AudioNode.h"
#include "AudioContext.h"
#include "nsContentUtils.h"
#include "mozilla/ErrorResult.h"
#include "AudioNodeStream.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(AudioNode, nsDOMEventTargetHelper)
  tmp->DisconnectFromGraph();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mOutputNodes)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(AudioNode, nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOutputNodes)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(AudioNode, nsDOMEventTargetHelper)

NS_IMETHODIMP_(nsrefcnt)
AudioNode::Release()
{
  if (mRefCnt.get() == 1) {
    
    
    DisconnectFromGraph();
  }
  nsrefcnt r = nsDOMEventTargetHelper::Release();
  NS_LOG_RELEASE(this, r, "AudioNode");
  return r;
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(AudioNode)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

AudioNode::AudioNode(AudioContext* aContext)
  : mContext(aContext)
{
  MOZ_ASSERT(aContext);
  nsDOMEventTargetHelper::BindToOwner(aContext->GetParentObject());
  SetIsDOMBinding();
}

AudioNode::~AudioNode()
{
  MOZ_ASSERT(mInputNodes.IsEmpty());
  MOZ_ASSERT(mOutputNodes.IsEmpty());
}

static uint32_t
FindIndexOfNode(const nsTArray<AudioNode::InputNode>& aInputNodes, const AudioNode* aNode)
{
  for (uint32_t i = 0; i < aInputNodes.Length(); ++i) {
    if (aInputNodes[i].mInputNode == aNode) {
      return i;
    }
  }
  return nsTArray<AudioNode::InputNode>::NoIndex;
}

static uint32_t
FindIndexOfNodeWithPorts(const nsTArray<AudioNode::InputNode>& aInputNodes, const AudioNode* aNode,
                         uint32_t aInputPort, uint32_t aOutputPort)
{
  for (uint32_t i = 0; i < aInputNodes.Length(); ++i) {
    if (aInputNodes[i].mInputNode == aNode &&
        aInputNodes[i].mInputPort == aInputPort &&
        aInputNodes[i].mOutputPort == aOutputPort) {
      return i;
    }
  }
  return nsTArray<AudioNode::InputNode>::NoIndex;
}

void
AudioNode::DisconnectFromGraph()
{
  
  
  nsRefPtr<AudioNode> kungFuDeathGrip = this;

  
  

  
  while (!mInputNodes.IsEmpty()) {
    uint32_t i = mInputNodes.Length() - 1;
    nsRefPtr<AudioNode> input = mInputNodes[i].mInputNode;
    mInputNodes.RemoveElementAt(i);
    input->mOutputNodes.RemoveElement(this);
  }

  while (!mOutputNodes.IsEmpty()) {
    uint32_t i = mOutputNodes.Length() - 1;
    nsRefPtr<AudioNode> output = mOutputNodes[i].forget();
    mOutputNodes.RemoveElementAt(i);
    uint32_t inputIndex = FindIndexOfNode(output->mInputNodes, this);
    
    
    output->mInputNodes.RemoveElementAt(inputIndex);
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
  if (SupportsMediaStreams() && aDestination.mStream) {
    
    MOZ_ASSERT(aDestination.mStream->AsProcessedStream());
    ProcessedMediaStream* ps =
      static_cast<ProcessedMediaStream*>(aDestination.mStream.get());
    input->mStreamPort =
      ps->AllocateInputPort(mStream, MediaInputPort::FLAG_BLOCK_INPUT);
  }

  
  Context()->UpdatePannerSource();
}

void
AudioNode::SendDoubleParameterToStream(uint32_t aIndex, double aValue)
{
  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  MOZ_ASSERT(ns, "How come we don't have a stream here?");
  ns->SetDoubleParameter(aIndex, aValue);
}

void
AudioNode::SendInt32ParameterToStream(uint32_t aIndex, int32_t aValue)
{
  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  MOZ_ASSERT(ns, "How come we don't have a stream here?");
  ns->SetInt32Parameter(aIndex, aValue);
}

void
AudioNode::SendThreeDPointParameterToStream(uint32_t aIndex, const ThreeDPoint& aValue)
{
  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  MOZ_ASSERT(ns, "How come we don't have a stream here?");
  ns->SetThreeDPointParameter(aIndex, aValue);
}

void
AudioNode::SendTimelineParameterToStream(AudioNode* aNode, uint32_t aIndex,
                                         const AudioParamTimeline& aValue)
{
  AudioNodeStream* ns = static_cast<AudioNodeStream*>(aNode->mStream.get());
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

  for (int32_t i = mOutputNodes.Length() - 1; i >= 0; --i) {
    AudioNode* dest = mOutputNodes[i];
    for (int32_t j = dest->mInputNodes.Length() - 1; j >= 0; --j) {
      InputNode& input = dest->mInputNodes[j];
      if (input.mInputNode == this && input.mOutputPort == aOutput) {
        dest->mInputNodes.RemoveElementAt(j);
        
        
        
        mOutputNodes.RemoveElementAt(i);
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
    {
      
      
      
      
      AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
      MutexAutoLock lock(ns->Engine()->NodeMutex());
      MOZ_ASSERT(ns, "How come we don't have a stream here?");
      MOZ_ASSERT(ns->Engine()->Node() == this, "Invalid node reference");
      ns->Engine()->ClearNode();
    }

    mStream->Destroy();
    mStream = nullptr;
  }
}

}
}
