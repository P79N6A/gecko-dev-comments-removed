





#include "AudioNode.h"
#include "AudioContext.h"
#include "nsContentUtils.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {
namespace dom {

template <typename T>
static void
TraverseElements(nsCycleCollectionTraversalCallback& cb,
                 const nsTArray<T>& array,
                 const char* name)
{
  for (uint32_t i = 0, length = array.Length(); i < length; ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, name);
    AudioNode* node = array[i].get();
    cb.NoteXPCOMChild(node);
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(AudioNode)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(AudioNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mInputs)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mOutputs)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(AudioNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mContext, AudioContext)
  TraverseElements(cb, tmp->mInputs, "mInputs[i]");
  TraverseElements(cb, tmp->mOutputs, "mOutputs[i]");
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(AudioNode)

NS_IMPL_CYCLE_COLLECTING_ADDREF(AudioNode)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AudioNode)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AudioNode)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

AudioNode::AudioNode(AudioContext* aContext)
  : mContext(aContext)
{
  MOZ_ASSERT(aContext);
  SetIsDOMBinding();
}

void
AudioNode::Connect(AudioNode& aDestination, uint32_t aOutput,
                   uint32_t aInput, ErrorResult& aRv)
{
  if (aOutput >= MaxNumberOfOutputs() ||
      aInput >= aDestination.MaxNumberOfInputs()) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  if (Context() != aDestination.Context()) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }

  

  Output output(&aDestination, aInput);
  mOutputs.EnsureLengthAtLeast(aOutput + 1);
  mOutputs.ReplaceElementAt(aOutput, output);
  Input input(this, aOutput);
  aDestination.mInputs.EnsureLengthAtLeast(aInput + 1);
  aDestination.mInputs.ReplaceElementAt(aInput, input);
}

void
AudioNode::Disconnect(uint32_t aOutput, ErrorResult& aRv)
{
  if (aOutput >= NumberOfOutputs()) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  
  
  const Output output = mOutputs[aOutput];
  if (!output) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }
  const Input input = output.mDestination->mInputs[output.mInput];

  MOZ_ASSERT(Context() == output.mDestination->Context());
  MOZ_ASSERT(aOutput == input.mOutput);

  if (!input || input.mSource != this) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }

  output.mDestination->mInputs.ReplaceElementAt(output.mInput, Input());
  mOutputs.ReplaceElementAt(input.mOutput, Output());
}

}
}

