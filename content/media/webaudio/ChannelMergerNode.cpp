





#include "mozilla/dom/ChannelMergerNode.h"
#include "mozilla/dom/ChannelMergerNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS_INHERITED0(ChannelMergerNode, AudioNode)

class ChannelMergerNodeEngine : public AudioNodeEngine
{
public:
  ChannelMergerNodeEngine(ChannelMergerNode* aNode)
    : AudioNodeEngine(aNode)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  virtual void ProcessBlocksOnPorts(AudioNodeStream* aStream,
                                    const OutputChunks& aInput,
                                    OutputChunks& aOutput,
                                    bool* aFinished) MOZ_OVERRIDE
  {
    MOZ_ASSERT(aInput.Length() >= 1, "Should have one or more input ports");

    
    uint32_t channelCount = 0;
    for (uint16_t i = 0; i < InputCount(); ++i) {
      channelCount += aInput[i].mChannelData.Length();
    }
    if (channelCount == 0) {
      aOutput[0].SetNull(WEBAUDIO_BLOCK_SIZE);
      return;
    }
    AllocateAudioBlock(channelCount, &aOutput[0]);

    
    uint32_t channelIndex = 0;
    for (uint16_t i = 0; i < InputCount(); ++i) {
      for (uint32_t j = 0; j < aInput[i].mChannelData.Length(); ++j) {
        AudioBlockCopyChannelWithScale(
            static_cast<const float*>(aInput[i].mChannelData[j]),
            aInput[i].mVolume,
            static_cast<float*>(const_cast<void*>(aOutput[0].mChannelData[channelIndex])));
        ++channelIndex;
      }
    }
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }
};

ChannelMergerNode::ChannelMergerNode(AudioContext* aContext,
                                     uint16_t aInputCount)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Max,
              ChannelInterpretation::Speakers)
  , mInputCount(aInputCount)
{
  mStream = aContext->Graph()->CreateAudioNodeStream(new ChannelMergerNodeEngine(this),
                                                     MediaStreamGraph::INTERNAL_STREAM);
}

JSObject*
ChannelMergerNode::WrapObject(JSContext* aCx)
{
  return ChannelMergerNodeBinding::Wrap(aCx, this);
}

}
}

