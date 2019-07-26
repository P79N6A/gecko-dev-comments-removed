





#include "AudioNode.h"
#include "AudioContext.h"
#include "nsContentUtils.h"
#include "mozilla/ErrorResult.h"
#include "AudioNodeStream.h"

namespace mozilla {
namespace dom {

inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            AudioNode::InputNode& aField,
                            const char* aName,
                            unsigned aFlags)
{
  CycleCollectionNoteChild(aCallback, aField.mInputNode.get(), aName, aFlags);
}

inline void
ImplCycleCollectionUnlink(nsCycleCollectionTraversalCallback& aCallback,
                          AudioNode::InputNode& aField,
                          const char* aName,
                          unsigned aFlags)
{
  aField.mInputNode = nullptr;
}

NS_IMPL_CYCLE_COLLECTION_3(AudioNode, mContext, mInputNodes, mOutputNodes)

NS_IMPL_CYCLE_COLLECTING_ADDREF(AudioNode)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AudioNode)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AudioNode)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

AudioNode::AudioNode(AudioContext* aContext)
  : mContext(aContext)
  , mJSBindingFinalized(false)
  , mCanProduceOwnOutput(false)
  , mOutputEnded(false)
{
  MOZ_ASSERT(aContext);
}

AudioNode::~AudioNode()
{
  DestroyMediaStream();
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
AudioNode::UpdateOutputEnded()
{
  if (mOutputEnded) {
    
    return;
  }
  if (mCanProduceOwnOutput ||
      !mInputNodes.IsEmpty() ||
      (!mJSBindingFinalized && NumberOfInputs() > 0)) {
    
    return;
  }

  mOutputEnded = true;

  
  
  nsRefPtr<AudioNode> kungFuDeathGrip = this;

  
  

  
  while (!mInputNodes.IsEmpty()) {
    uint32_t i = mInputNodes.Length() - 1;
    nsRefPtr<AudioNode> input = mInputNodes[i].mInputNode.forget();
    mInputNodes.RemoveElementAt(i);
    NS_ASSERTION(mOutputNodes.Contains(this), "input/output inconsistency");
    input->mOutputNodes.RemoveElement(this);
  }

  while (!mOutputNodes.IsEmpty()) {
    uint32_t i = mOutputNodes.Length() - 1;
    nsRefPtr<AudioNode> output = mOutputNodes[i].forget();
    mOutputNodes.RemoveElementAt(i);
    uint32_t inputIndex = FindIndexOfNode(output->mInputNodes, this);
    NS_ASSERTION(inputIndex != nsTArray<AudioNode::InputNode>::NoIndex, "input/output inconsistency");
    
    
    output->mInputNodes.RemoveElementAt(inputIndex);

    output->UpdateOutputEnded();
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

  if (IsOutputEnded() || aDestination.IsOutputEnded()) {
    
    
    return;
  }
  if (FindIndexOfNodeWithPorts(aDestination.mInputNodes, this, aInput, aOutput) !=
      nsTArray<AudioNode::InputNode>::NoIndex) {
    
    return;
  }

  
  

  
  nsRefPtr<AudioNode> kungFuDeathGrip = this;

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
      ps->AllocateInputPort(mStream, MediaInputPort::FLAG_BLOCK_OUTPUT);
  }
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
AudioNode::Disconnect(uint32_t aOutput, ErrorResult& aRv)
{
  if (aOutput >= NumberOfOutputs()) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  
  
  nsAutoTArray<nsRefPtr<AudioNode>,4> outputsToUpdate;

  for (int32_t i = mOutputNodes.Length() - 1; i >= 0; --i) {
    AudioNode* dest = mOutputNodes[i];
    for (int32_t j = dest->mInputNodes.Length() - 1; j >= 0; --j) {
      InputNode& input = dest->mInputNodes[j];
      if (input.mInputNode == this && input.mOutputPort == aOutput) {
        dest->mInputNodes.RemoveElementAt(j);
        
        
        
        *outputsToUpdate.AppendElement() = mOutputNodes[i].forget();
        mOutputNodes.RemoveElementAt(i);
        break;
      }
    }
  }

  for (uint32_t i = 0; i < outputsToUpdate.Length(); ++i) {
    outputsToUpdate[i]->UpdateOutputEnded();
  }
}

}
}
