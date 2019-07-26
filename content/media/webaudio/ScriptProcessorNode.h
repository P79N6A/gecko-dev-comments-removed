





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

  NS_DECL_ISUPPORTS_INHERITED

  IMPL_EVENT_HANDLER(audioprocess)

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope);

  virtual bool SupportsMediaStreams() const MOZ_OVERRIDE
  {
    return true;
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

private:
  nsAutoPtr<SharedBuffers> mSharedBuffers;
  const uint32_t mBufferSize;
  const uint32_t mNumberOfOutputChannels;
};

}
}

#endif

