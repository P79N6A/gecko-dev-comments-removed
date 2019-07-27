





#include "mozilla/dom/ChannelMergerNode.h"
#include "mozilla/dom/ChannelMergerNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS_INHERITED0(ChannelMergerNode, AudioNode)

class ChannelMergerNodeEngine final : public AudioNodeEngine
{
public:
  explicit ChannelMergerNodeEngine(ChannelMergerNode* aNode)
    : AudioNodeEngine(aNode)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  virtual void ProcessBlocksOnPorts(AudioNodeStream* aStream,
                                    const OutputChunks& aInput,
                                    OutputChunks& aOutput,
                                    bool* aFinished) override
  {
    MOZ_ASSERT(aInput.Length() >= 1, "Should have one or more input ports");

    
    size_t channelCount = 0;
    for (uint16_t i = 0; i < InputCount(); ++i) {
      channelCount += aInput[i].mChannelData.Length();
    }
    if (channelCount == 0) {
      aOutput[0].SetNull(WEBAUDIO_BLOCK_SIZE);
      return;
    }
    channelCount = std::min(channelCount, WebAudioUtils::MaxChannelCount);
    AllocateAudioBlock(channelCount, &aOutput[0]);

    
    size_t channelIndex = 0;
    for (uint16_t i = 0; true; ++i) {
      MOZ_ASSERT(i < InputCount());
      for (size_t j = 0; j < aInput[i].mChannelData.Length(); ++j) {
        AudioBlockCopyChannelWithScale(
            static_cast<const float*>(aInput[i].mChannelData[j]),
            aInput[i].mVolume,
            static_cast<float*>(const_cast<void*>(aOutput[0].mChannelData[channelIndex])));
        ++channelIndex;
        if (channelIndex >= channelCount) {
          return;
        }
      }
    }
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
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

ChannelMergerNode::~ChannelMergerNode()
{
}

JSObject*
ChannelMergerNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return ChannelMergerNodeBinding::Wrap(aCx, this, aGivenProto);
}

}
}

