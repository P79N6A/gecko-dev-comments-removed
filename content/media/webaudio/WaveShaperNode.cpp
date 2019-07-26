





#include "WaveShaperNode.h"
#include "mozilla/dom/WaveShaperNodeBinding.h"
#include "AudioNode.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "mozilla/PodOperations.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(WaveShaperNode)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(WaveShaperNode, AudioNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
  tmp->ClearCurve();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(WaveShaperNode, AudioNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(WaveShaperNode)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mCurve)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(WaveShaperNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(WaveShaperNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(WaveShaperNode, AudioNode)

class WaveShaperNodeEngine : public AudioNodeEngine
{
public:
  explicit WaveShaperNodeEngine(AudioNode* aNode)
    : AudioNodeEngine(aNode)
    , mType(OverSampleType::None)
  {
  }

  enum Parameteres {
    TYPE
  };

  virtual void SetRawArrayData(nsTArray<float>& aCurve) MOZ_OVERRIDE
  {
    mCurve.SwapElements(aCurve);
  }

  virtual void SetInt32Parameter(uint32_t aIndex, int32_t aValue) MOZ_OVERRIDE
  {
    switch (aIndex) {
    case TYPE:
      mType = static_cast<OverSampleType>(aValue);
      break;
    default:
      NS_ERROR("Bad WaveShaperNode Int32Parameter");
    }
  }

  template <uint32_t blocks>
  void ProcessCurve(const float* aInputBuffer, float* aOutputBuffer)
  {
    for (uint32_t j = 0; j < WEBAUDIO_BLOCK_SIZE*blocks; ++j) {
      
      
      
      float index = std::max(0.0f, std::min(float(mCurve.Length() - 1),
                                            mCurve.Length() * (aInputBuffer[j] + 1) / 2));
      uint32_t indexLower = uint32_t(index);
      uint32_t indexHigher = uint32_t(index + 1.0f);
      if (indexHigher == mCurve.Length()) {
        aOutputBuffer[j] = mCurve[indexLower];
      } else {
        float interpolationFactor = index - indexLower;
        aOutputBuffer[j] = (1.0f - interpolationFactor) * mCurve[indexLower] +
                                   interpolationFactor  * mCurve[indexHigher];
      }
    }
  }

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished)
  {
    uint32_t channelCount = aInput.mChannelData.Length();
    if (!mCurve.Length() || !channelCount) {
      
      
      *aOutput = aInput;
      return;
    }

    AllocateAudioBlock(channelCount, aOutput);
    for (uint32_t i = 0; i < channelCount; ++i) {
      const float* inputBuffer = static_cast<const float*>(aInput.mChannelData[i]);
      float* outputBuffer = const_cast<float*> (static_cast<const float*>(aOutput->mChannelData[i]));

      switch (mType) {
      case OverSampleType::None:
        ProcessCurve<1>(inputBuffer, outputBuffer);
        break;
      case OverSampleType::_2x:
        
        break;
      case OverSampleType::_4x:
        
        break;
      default:
        NS_NOTREACHED("We should never reach here");
      }
    }
  }

private:
  nsTArray<float> mCurve;
  OverSampleType mType;
};

WaveShaperNode::WaveShaperNode(AudioContext* aContext)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Max,
              ChannelInterpretation::Speakers)
  , mCurve(nullptr)
  , mType(OverSampleType::None)
{
  NS_HOLD_JS_OBJECTS(this, WaveShaperNode);

  WaveShaperNodeEngine* engine = new WaveShaperNodeEngine(this);
  mStream = aContext->Graph()->CreateAudioNodeStream(engine, MediaStreamGraph::INTERNAL_STREAM);
}

WaveShaperNode::~WaveShaperNode()
{
  ClearCurve();
}

void
WaveShaperNode::ClearCurve()
{
  mCurve = nullptr;
  NS_DROP_JS_OBJECTS(this, WaveShaperNode);
}

JSObject*
WaveShaperNode::WrapObject(JSContext *aCx, JS::Handle<JSObject*> aScope)
{
  return WaveShaperNodeBinding::Wrap(aCx, aScope, this);
}

void
WaveShaperNode::SetCurve(const Nullable<Float32Array>& aCurve)
{
  nsTArray<float> curve;
  if (!aCurve.IsNull()) {
    mCurve = aCurve.Value().Obj();

    curve.SetLength(aCurve.Value().Length());
    PodCopy(curve.Elements(), aCurve.Value().Data(), aCurve.Value().Length());
  } else {
    mCurve = nullptr;
  }

  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  MOZ_ASSERT(ns, "Why don't we have a stream here?");
  ns->SetRawArrayData(curve);
}

void
WaveShaperNode::SetOversample(OverSampleType aType)
{
  mType = aType;
  SendInt32ParameterToStream(WaveShaperNodeEngine::TYPE, static_cast<int32_t>(aType));
}

}
}
