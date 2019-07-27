





#ifndef ScriptProcessorNode_h_
#define ScriptProcessorNode_h_

#include "AudioNode.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {

class AudioContext;
class SharedBuffers;

class ScriptProcessorNode : public AudioNode
{
public:
  ScriptProcessorNode(AudioContext* aContext,
                      uint32_t aBufferSize,
                      uint32_t aNumberOfInputChannels,
                      uint32_t aNumberOfOutputChannels);

  NS_DECL_ISUPPORTS_INHERITED

  IMPL_EVENT_HANDLER(audioprocess)

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

  virtual void Connect(AudioNode& aDestination, uint32_t aOutput,
                       uint32_t aInput, ErrorResult& aRv) MOZ_OVERRIDE
  {
    AudioNode::Connect(aDestination, aOutput, aInput, aRv);
    if (!aRv.Failed()) {
      MarkActive();
    }
  }

  virtual void Connect(AudioParam& aDestination, uint32_t aOutput,
                       ErrorResult& aRv) MOZ_OVERRIDE
  {
    AudioNode::Connect(aDestination, aOutput, aRv);
    if (!aRv.Failed()) {
      MarkActive();
    }
  }

  virtual void Disconnect(uint32_t aOutput, ErrorResult& aRv) MOZ_OVERRIDE
  {
    AudioNode::Disconnect(aOutput, aRv);
    if (!aRv.Failed() && OutputNodes().IsEmpty() && OutputParams().IsEmpty()) {
      MarkInactive();
    }
  }

  virtual void SetChannelCount(uint32_t aChannelCount, ErrorResult& aRv) MOZ_OVERRIDE
  {
    if (aChannelCount != ChannelCount()) {
      aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    }
    return;
  }
  virtual void SetChannelCountModeValue(ChannelCountMode aMode, ErrorResult& aRv) MOZ_OVERRIDE
  {
    if (aMode != ChannelCountMode::Explicit) {
      aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    }
    return;
  }

  uint32_t BufferSize() const
  {
    return mBufferSize;
  }

  SharedBuffers* GetSharedBuffers() const
  {
    return mSharedBuffers;
  }

  uint32_t NumberOfOutputChannels() const
  {
    return mNumberOfOutputChannels;
  }

  using DOMEventTargetHelper::DispatchTrustedEvent;

  virtual const char* NodeType() const MOZ_OVERRIDE
  {
    return "ScriptProcessorNode";
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;
  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

protected:
  virtual ~ScriptProcessorNode();

private:
  nsAutoPtr<SharedBuffers> mSharedBuffers;
  const uint32_t mBufferSize;
  const uint32_t mNumberOfOutputChannels;
};

}
}

#endif

