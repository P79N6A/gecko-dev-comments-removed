





#ifndef ScriptProcessorNode_h_
#define ScriptProcessorNode_h_

#include "AudioNode.h"
#include "nsAutoPtr.h"

namespace mozilla {

class AudioNodeStream;

namespace dom {

class AudioContext;
class ScriptProcessorNodeEngine;
class SharedBuffers;

class ScriptProcessorNode : public AudioNode
{
public:
  ScriptProcessorNode(AudioContext* aContext,
                      uint32_t aBufferSize,
                      uint32_t aNumberOfInputChannels,
                      uint32_t aNumberOfOutputChannels);
  virtual ~ScriptProcessorNode();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ScriptProcessorNode, AudioNode)

  IMPL_EVENT_HANDLER(audioprocess)

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  virtual void Connect(AudioNode& aDestination, uint32_t aOutput,
                       uint32_t aInput, ErrorResult& aRv) MOZ_OVERRIDE
  {
    AudioNode::Connect(aDestination, aOutput, aInput, aRv);
    if (!aRv.Failed()) {
      mPlayingRef.Take(this);
    }
  }

  virtual void Disconnect(uint32_t aOutput, ErrorResult& aRv) MOZ_OVERRIDE
  {
    AudioNode::Disconnect(aOutput, aRv);
    if (!aRv.Failed()) {
      mPlayingRef.Drop(this);
    }
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

  using nsDOMEventTargetHelper::DispatchTrustedEvent;

  void Stop()
  {
    mPlayingRef.Drop(this);
  }

private:
  nsAutoPtr<SharedBuffers> mSharedBuffers;
  const uint32_t mBufferSize;
  const uint32_t mNumberOfOutputChannels;
  SelfReference<ScriptProcessorNode> mPlayingRef; 
};

}
}

#endif

