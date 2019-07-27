





#include "OscillatorNode.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "AudioDestinationNode.h"
#include "WebAudioUtils.h"
#include "blink/PeriodicWave.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED(OscillatorNode, AudioNode,
                                   mPeriodicWave, mFrequency, mDetune)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(OscillatorNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(OscillatorNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(OscillatorNode, AudioNode)

class OscillatorNodeEngine final : public AudioNodeEngine
{
public:
  OscillatorNodeEngine(AudioNode* aNode, AudioDestinationNode* aDestination)
    : AudioNodeEngine(aNode)
    , mSource(nullptr)
    , mDestination(static_cast<AudioNodeStream*> (aDestination->Stream()))
    , mStart(-1)
    , mStop(STREAM_TIME_MAX)
    
    , mFrequency(440.f)
    , mDetune(0.f)
    , mType(OscillatorType::Sine)
    , mPhase(0.)
    , mRecomputeParameters(true)
    , mCustomLength(0)
  {
  }

  void SetSourceStream(AudioNodeStream* aSource)
  {
    mSource = aSource;
  }

  enum Parameters {
    FREQUENCY,
    DETUNE,
    TYPE,
    PERIODICWAVE,
    START,
    STOP,
  };
  void SetTimelineParameter(uint32_t aIndex,
                            const AudioParamTimeline& aValue,
                            TrackRate aSampleRate) override
  {
    mRecomputeParameters = true;
    switch (aIndex) {
    case FREQUENCY:
      MOZ_ASSERT(mSource && mDestination);
      mFrequency = aValue;
      WebAudioUtils::ConvertAudioParamToTicks(mFrequency, mSource, mDestination);
      break;
    case DETUNE:
      MOZ_ASSERT(mSource && mDestination);
      mDetune = aValue;
      WebAudioUtils::ConvertAudioParamToTicks(mDetune, mSource, mDestination);
      break;
    default:
      NS_ERROR("Bad OscillatorNodeEngine TimelineParameter");
    }
  }

  virtual void SetStreamTimeParameter(uint32_t aIndex, StreamTime aParam) override
  {
    switch (aIndex) {
    case START: mStart = aParam; break;
    case STOP: mStop = aParam; break;
    default:
      NS_ERROR("Bad OscillatorNodeEngine StreamTimeParameter");
    }
  }

  virtual void SetInt32Parameter(uint32_t aIndex, int32_t aParam) override
  {
    switch (aIndex) {
      case TYPE:
        
        mType = static_cast<OscillatorType>(aParam);
        if (mType == OscillatorType::Sine) {
          
          mCustomLength = 0;
          mCustom = nullptr;
          mPeriodicWave = nullptr;
          mRecomputeParameters = true;
        }
        switch (mType) {
          case OscillatorType::Sine:
            mPhase = 0.0;
            break;
          case OscillatorType::Square:
            mPeriodicWave = WebCore::PeriodicWave::createSquare(mSource->SampleRate());
            break;
          case OscillatorType::Triangle:
            mPeriodicWave = WebCore::PeriodicWave::createTriangle(mSource->SampleRate());
            break;
          case OscillatorType::Sawtooth:
            mPeriodicWave = WebCore::PeriodicWave::createSawtooth(mSource->SampleRate());
            break;
          case OscillatorType::Custom:
            break;
          default:
            NS_ERROR("Bad OscillatorNodeEngine type parameter.");
        }
        
        break;
      case PERIODICWAVE:
        MOZ_ASSERT(aParam >= 0, "negative custom array length");
        mCustomLength = static_cast<uint32_t>(aParam);
        break;
      default:
        NS_ERROR("Bad OscillatorNodeEngine Int32Parameter.");
    }
    
  }

  virtual void SetBuffer(already_AddRefed<ThreadSharedFloatArrayBufferList> aBuffer) override
  {
    MOZ_ASSERT(mCustomLength, "Custom buffer sent before length");
    mCustom = aBuffer;
    MOZ_ASSERT(mCustom->GetChannels() == 2,
               "PeriodicWave should have sent two channels");
    mPeriodicWave = WebCore::PeriodicWave::create(mSource->SampleRate(),
    mCustom->GetData(0), mCustom->GetData(1), mCustomLength);
  }

  void IncrementPhase()
  {
    const float twoPiFloat = float(2 * M_PI);
    mPhase += mPhaseIncrement;
    if (mPhase > twoPiFloat) {
      mPhase -= twoPiFloat;
    } else if (mPhase < -twoPiFloat) {
      mPhase += twoPiFloat;
    }
  }

  void UpdateParametersIfNeeded(StreamTime ticks, size_t count)
  {
    double frequency, detune;

    
    
    if (!ParametersMayNeedUpdate()) {
      return;
    }

    bool simpleFrequency = mFrequency.HasSimpleValue();
    bool simpleDetune = mDetune.HasSimpleValue();

    if (simpleFrequency) {
      frequency = mFrequency.GetValue();
    } else {
      frequency = mFrequency.GetValueAtTime(ticks, count);
    }
    if (simpleDetune) {
      detune = mDetune.GetValue();
    } else {
      detune = mDetune.GetValueAtTime(ticks, count);
    }

    mFinalFrequency = frequency * pow(2., detune / 1200.);
    float signalPeriod = mSource->SampleRate() / mFinalFrequency;
    mRecomputeParameters = false;

    mPhaseIncrement = 2 * M_PI / signalPeriod;
  }

  void FillBounds(float* output, StreamTime ticks,
                  uint32_t& start, uint32_t& end)
  {
    MOZ_ASSERT(output);
    static_assert(StreamTime(WEBAUDIO_BLOCK_SIZE) < UINT_MAX,
        "WEBAUDIO_BLOCK_SIZE overflows interator bounds.");
    start = 0;
    if (ticks < mStart) {
      start = mStart - ticks;
      for (uint32_t i = 0; i < start; ++i) {
        output[i] = 0.0;
      }
    }
    end = WEBAUDIO_BLOCK_SIZE;
    if (ticks + end > mStop) {
      end = mStop - ticks;
      for (uint32_t i = end; i < WEBAUDIO_BLOCK_SIZE; ++i) {
        output[i] = 0.0;
      }
    }
  }

  void ComputeSine(float * aOutput, StreamTime ticks, uint32_t aStart, uint32_t aEnd)
  {
    for (uint32_t i = aStart; i < aEnd; ++i) {
      UpdateParametersIfNeeded(ticks, i);

      aOutput[i] = sin(mPhase);

      IncrementPhase();
    }
  }

  bool ParametersMayNeedUpdate()
  {
    return !mDetune.HasSimpleValue() ||
           !mFrequency.HasSimpleValue() ||
           mRecomputeParameters;
  }

  void ComputeCustom(float* aOutput,
                     StreamTime ticks,
                     uint32_t aStart,
                     uint32_t aEnd)
  {
    MOZ_ASSERT(mPeriodicWave, "No custom waveform data");

    uint32_t periodicWaveSize = mPeriodicWave->periodicWaveSize();
    
    uint32_t indexMask = periodicWaveSize - 1;
    MOZ_ASSERT(periodicWaveSize && (periodicWaveSize & indexMask) == 0,
               "periodicWaveSize must be power of 2");
    float* higherWaveData = nullptr;
    float* lowerWaveData = nullptr;
    float tableInterpolationFactor;
    
    
    float basePhaseIncrement = mPeriodicWave->rateScale();

    UpdateParametersIfNeeded(ticks, aStart);

    bool parametersMayNeedUpdate = ParametersMayNeedUpdate();
    mPeriodicWave->waveDataForFundamentalFrequency(mFinalFrequency,
                                                   lowerWaveData,
                                                   higherWaveData,
                                                   tableInterpolationFactor);

    for (uint32_t i = aStart; i < aEnd; ++i) {
      if (parametersMayNeedUpdate) {
        mPeriodicWave->waveDataForFundamentalFrequency(mFinalFrequency,
                                                       lowerWaveData,
                                                       higherWaveData,
                                                       tableInterpolationFactor);
        UpdateParametersIfNeeded(ticks, i);
      }
      
      float floorPhase = floorf(mPhase);
      int j1Signed = static_cast<int>(floorPhase);
      uint32_t j1 = j1Signed & indexMask;
      uint32_t j2 = j1 + 1;
      j2 &= indexMask;

      float sampleInterpolationFactor = mPhase - floorPhase;

      float lower = (1.0f - sampleInterpolationFactor) * lowerWaveData[j1] +
                    sampleInterpolationFactor * lowerWaveData[j2];
      float higher = (1.0f - sampleInterpolationFactor) * higherWaveData[j1] +
                    sampleInterpolationFactor * higherWaveData[j2];
      aOutput[i] = (1.0f - tableInterpolationFactor) * lower +
                   tableInterpolationFactor * higher;

      
      
      mPhase =
        j1 + sampleInterpolationFactor + basePhaseIncrement * mFinalFrequency;
    }
  }

  void ComputeSilence(AudioChunk *aOutput)
  {
    aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
  }

  virtual void ProcessBlock(AudioNodeStream* aStream,
                            const AudioChunk& aInput,
                            AudioChunk* aOutput,
                            bool* aFinished) override
  {
    MOZ_ASSERT(mSource == aStream, "Invalid source stream");

    StreamTime ticks = aStream->GetCurrentPosition();
    if (mStart == -1) {
      ComputeSilence(aOutput);
      return;
    }

    if (ticks >= mStop) {
      
      ComputeSilence(aOutput);
      *aFinished = true;
      return;
    }
    if (ticks + WEBAUDIO_BLOCK_SIZE <= mStart) {
      
      ComputeSilence(aOutput);
      return;
    }

    AllocateAudioBlock(1, aOutput);
    float* output = static_cast<float*>(
        const_cast<void*>(aOutput->mChannelData[0]));

    uint32_t start, end;
    FillBounds(output, ticks, start, end);

    
    switch(mType) {
      case OscillatorType::Sine:
        ComputeSine(output, ticks, start, end);
        break;
      case OscillatorType::Square:
      case OscillatorType::Triangle:
      case OscillatorType::Sawtooth:
      case OscillatorType::Custom:
        ComputeCustom(output, ticks, start, end);
        break;
      default:
        ComputeSilence(aOutput);
    };

  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    size_t amount = AudioNodeEngine::SizeOfExcludingThis(aMallocSizeOf);

    
    
    
    
    

    if (mCustom) {
      amount += mCustom->SizeOfIncludingThis(aMallocSizeOf);
    }

    if (mPeriodicWave) {
      amount += mPeriodicWave->sizeOfIncludingThis(aMallocSizeOf);
    }

    return amount;
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  AudioNodeStream* mSource;
  AudioNodeStream* mDestination;
  StreamTime mStart;
  StreamTime mStop;
  AudioParamTimeline mFrequency;
  AudioParamTimeline mDetune;
  OscillatorType mType;
  float mPhase;
  float mFinalFrequency;
  float mPhaseIncrement;
  bool mRecomputeParameters;
  nsRefPtr<ThreadSharedFloatArrayBufferList> mCustom;
  uint32_t mCustomLength;
  nsAutoPtr<WebCore::PeriodicWave> mPeriodicWave;
};

OscillatorNode::OscillatorNode(AudioContext* aContext)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Max,
              ChannelInterpretation::Speakers)
  , mType(OscillatorType::Sine)
  , mFrequency(new AudioParam(this, SendFrequencyToStream, 440.0f, "frequency"))
  , mDetune(new AudioParam(this, SendDetuneToStream, 0.0f, "detune"))
  , mStartCalled(false)
{
  OscillatorNodeEngine* engine = new OscillatorNodeEngine(this, aContext->Destination());
  mStream = aContext->Graph()->CreateAudioNodeStream(engine, MediaStreamGraph::SOURCE_STREAM);
  engine->SetSourceStream(static_cast<AudioNodeStream*> (mStream.get()));
  mStream->AddMainThreadListener(this);
}

OscillatorNode::~OscillatorNode()
{
}

size_t
OscillatorNode::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t amount = AudioNode::SizeOfExcludingThis(aMallocSizeOf);

  
  if (mPeriodicWave) {
    amount += mPeriodicWave->SizeOfIncludingThisIfNotShared(aMallocSizeOf);
  }
  amount += mFrequency->SizeOfIncludingThis(aMallocSizeOf);
  amount += mDetune->SizeOfIncludingThis(aMallocSizeOf);
  return amount;
}

size_t
OscillatorNode::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

JSObject*
OscillatorNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return OscillatorNodeBinding::Wrap(aCx, this, aGivenProto);
}

void
OscillatorNode::SendFrequencyToStream(AudioNode* aNode)
{
  OscillatorNode* This = static_cast<OscillatorNode*>(aNode);
  if (!This->mStream) {
    return;
  }
  SendTimelineParameterToStream(This, OscillatorNodeEngine::FREQUENCY, *This->mFrequency);
}

void
OscillatorNode::SendDetuneToStream(AudioNode* aNode)
{
  OscillatorNode* This = static_cast<OscillatorNode*>(aNode);
  if (!This->mStream) {
    return;
  }
  SendTimelineParameterToStream(This, OscillatorNodeEngine::DETUNE, *This->mDetune);
}

void
OscillatorNode::SendTypeToStream()
{
  if (!mStream) {
    return;
  }
  if (mType == OscillatorType::Custom) {
    
    SendPeriodicWaveToStream();
  }
  SendInt32ParameterToStream(OscillatorNodeEngine::TYPE, static_cast<int32_t>(mType));
}

void OscillatorNode::SendPeriodicWaveToStream()
{
  NS_ASSERTION(mType == OscillatorType::Custom,
               "Sending custom waveform to engine thread with non-custom type");
  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  MOZ_ASSERT(ns, "Missing node stream.");
  MOZ_ASSERT(mPeriodicWave, "Send called without PeriodicWave object.");
  SendInt32ParameterToStream(OscillatorNodeEngine::PERIODICWAVE,
                             mPeriodicWave->DataLength());
  nsRefPtr<ThreadSharedFloatArrayBufferList> data =
    mPeriodicWave->GetThreadSharedBuffer();
  ns->SetBuffer(data.forget());
}

void
OscillatorNode::Start(double aWhen, ErrorResult& aRv)
{
  if (!WebAudioUtils::IsTimeValid(aWhen)) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  if (mStartCalled) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  mStartCalled = true;

  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  if (!ns) {
    
    return;
  }

  
  ns->SetStreamTimeParameter(OscillatorNodeEngine::START,
                             Context(), aWhen);

  MarkActive();
}

void
OscillatorNode::Stop(double aWhen, ErrorResult& aRv)
{
  if (!WebAudioUtils::IsTimeValid(aWhen)) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  if (!mStartCalled) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  if (!ns || !Context()) {
    
    return;
  }

  
  ns->SetStreamTimeParameter(OscillatorNodeEngine::STOP,
                             Context(), std::max(0.0, aWhen));
}

void
OscillatorNode::NotifyMainThreadStreamFinished()
{
  MOZ_ASSERT(mStream->IsFinished());

  class EndedEventDispatcher final : public nsRunnable
  {
  public:
    explicit EndedEventDispatcher(OscillatorNode* aNode)
      : mNode(aNode) {}
    NS_IMETHOD Run() override
    {
      
      if (!nsContentUtils::IsSafeToRunScript()) {
        nsContentUtils::AddScriptRunner(this);
        return NS_OK;
      }

      mNode->DispatchTrustedEvent(NS_LITERAL_STRING("ended"));
      return NS_OK;
    }
  private:
    nsRefPtr<OscillatorNode> mNode;
  };

  NS_DispatchToMainThread(new EndedEventDispatcher(this));
  
  
  DestroyMediaStream();

  
  
  MarkInactive();
}

}
}
