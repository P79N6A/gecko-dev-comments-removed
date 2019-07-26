





#include "mozilla/dom/ChannelSplitterNode.h"
#include "mozilla/dom/ChannelSplitterNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "mozilla/PodOperations.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS_INHERITED0(ChannelSplitterNode, AudioNode)

class ChannelSplitterNodeEngine : public AudioNodeEngine
{
public:
  ChannelSplitterNodeEngine(ChannelSplitterNode* aNode)
    : AudioNodeEngine(aNode)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  virtual void ProduceAudioBlocksOnPorts(AudioNodeStream* aStream,
                                         const OutputChunks& aInput,
                                         OutputChunks& aOutput,
                                         bool* aFinished) MOZ_OVERRIDE
  {
    MOZ_ASSERT(aInput.Length() == 1, "Should only have one input port");

    aOutput.SetLength(OutputCount());
    for (uint16_t i = 0; i < OutputCount(); ++i) {
      if (i < aInput[0].mChannelData.Length()) {
        
        AllocateAudioBlock(1, &aOutput[i]);
        PodCopy(static_cast<float*>(const_cast<void*>(aOutput[i].mChannelData[0])),
                static_cast<const float*>(aInput[0].mChannelData[i]),
                WEBAUDIO_BLOCK_SIZE);
      } else {
        
        aOutput[i].SetNull(WEBAUDIO_BLOCK_SIZE);
      }
    }
  }
};

ChannelSplitterNode::ChannelSplitterNode(AudioContext* aContext,
                                         uint16_t aOutputCount)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Max,
              ChannelInterpretation::Speakers)
  , mOutputCount(aOutputCount)
{
  mStream = aContext->Graph()->CreateAudioNodeStream(new ChannelSplitterNodeEngine(this),
                                                     MediaStreamGraph::INTERNAL_STREAM);
}

JSObject*
ChannelSplitterNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return ChannelSplitterNodeBinding::Wrap(aCx, aScope, this);
}

}
}

