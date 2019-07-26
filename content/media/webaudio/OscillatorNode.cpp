





#include "OscillatorNode.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "AudioDestinationNode.h"
#include "WebAudioUtils.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(OscillatorNode)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(OscillatorNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPeriodicWave)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mFrequency)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDetune)
  if (tmp->Context()) {
    tmp->Context()->UnregisterOscillatorNode(tmp);
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END_INHERITED(AudioNode);

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(OscillatorNode, AudioNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPeriodicWave)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mFrequency)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDetune)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(OscillatorNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(OscillatorNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(OscillatorNode, AudioNode)

static const float sLeak = 0.995f;

class DCBlocker
{
public:
  
  DCBlocker(float aLastInput = 0.0f,
            float aLastOutput = 0.0f,
            float aPole = 0.995)
    :mLastInput(aLastInput),
     mLastOutput(aLastOutput),
     mPole(aPole)
  {
    MOZ_ASSERT(aPole > 0);
  }

  inline float Process(float aInput)
  {
    float out;

    out = mLastOutput * mPole + aInput - mLastInput;
    mLastOutput = out;
    mLastInput = aInput;

    return out;
  }
private:
  float mLastInput;
  float mLastOutput;
  float mPole;
};


class OscillatorNodeEngine : public AudioNodeEngine
{
public:
  OscillatorNodeEngine(AudioNode* aNode, AudioDestinationNode* aDestination)
    : AudioNodeEngine(aNode)
    , mSource(nullptr)
    , mDestination(static_cast<AudioNodeStream*> (aDestination->Stream()))
    , mStart(-1)
    , mStop(TRACK_TICKS_MAX)
    
    , mFrequency(440.f)
    , mDetune(0.f)
    , mType(OscillatorType::Sine)
    , mPhase(0.)
    , mFinalFrequency(0.0)
    , mNumberOfHarmonics(0)
    , mSignalPeriod(0.0)
    , mAmplitudeAtZero(0.0)
    , mPhaseIncrement(0.0)
    , mSquare(0.0)
    , mTriangle(0.0)
    , mSaw(0.0)
    , mPhaseWrap(0.0)
    , mRecomputeFrequency(true)
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
                            TrackRate aSampleRate) MOZ_OVERRIDE
  {
    mRecomputeFrequency = true;
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

  virtual void SetStreamTimeParameter(uint32_t aIndex, TrackTicks aParam)
  {
    switch (aIndex) {
    case START: mStart = aParam; break;
    case STOP: mStop = aParam; break;
    default:
      NS_ERROR("Bad OscillatorNodeEngine StreamTimeParameter");
    }
  }

  virtual void SetInt32Parameter(uint32_t aIndex, int32_t aParam)
  {
    mType = static_cast<OscillatorType>(aParam);
    
    switch (mType) {
      case OscillatorType::Sine:
        mPhase = 0.0;
        break;
      case OscillatorType::Square:
        mPhase = 0.0;
        
        
        mSquare = -0.5;
        break;
      case OscillatorType::Triangle:
        
        
        
        mPhase = (float)(M_PI / 2);
        mSquare = 0.5;
        mTriangle = 0.0;
        break;
      case OscillatorType::Sawtooth:
        

        mPhase = (float)(M_PI / 2);
        
        mSaw = 0.0;
        break;
      default:
        NS_ERROR("Bad OscillatorNodeEngine Int32Parameter.");
    };
  }

  void IncrementPhase()
  {
    mPhase += mPhaseIncrement;
    if (mPhase > mPhaseWrap) {
      mPhase -= mPhaseWrap;
    }
  }

  
  
  bool UsesBipolarBLIT() {
    return mType == OscillatorType::Square || mType == OscillatorType::Triangle;
  }

  void UpdateFrequencyIfNeeded(TrackTicks ticks, size_t count)
  {
    double frequency, detune;

    bool simpleFrequency = mFrequency.HasSimpleValue();
    bool simpleDetune = mDetune.HasSimpleValue();

    
    
    if (simpleFrequency && simpleDetune && !mRecomputeFrequency) {
      return;
    }

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
    mRecomputeFrequency = false;

    
    
    mSignalPeriod = UsesBipolarBLIT() ? 0.5 * mSource->SampleRate() / mFinalFrequency
                                      : mSource->SampleRate() / mFinalFrequency;
    
    mPhaseWrap = UsesBipolarBLIT() || mType == OscillatorType::Sine ? 2 * M_PI
                                   : M_PI;
    
    mNumberOfHarmonics = UsesBipolarBLIT() ? 2 * floor(0.5 * mSignalPeriod)
                                           : 2 * floor(0.5 * mSignalPeriod) + 1;
    mPhaseIncrement = mType == OscillatorType::Sine ? 2 * M_PI / mSignalPeriod
                                                    : M_PI / mSignalPeriod;
    mAmplitudeAtZero = mNumberOfHarmonics / mSignalPeriod;
  }

  void FillBounds(float* output, TrackTicks ticks,
                  uint32_t& start, uint32_t& end)
  {
    MOZ_ASSERT(output);
    static_assert(TrackTicks(WEBAUDIO_BLOCK_SIZE) < UINT_MAX,
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

  float BipolarBLIT()
  {
    float blit;
    float denom = sin(mPhase);

    if (fabs(denom) < std::numeric_limits<float>::epsilon()) {
      if (mPhase < 0.1f || mPhase > 2 * M_PI - 0.1f) {
        blit = mAmplitudeAtZero;
      } else {
        blit = -mAmplitudeAtZero;
      }
    } else {
      blit = sin(mNumberOfHarmonics * mPhase);
      blit /= mSignalPeriod * denom;
    }
    return blit;
  }

  float UnipolarBLIT()
  {
    float blit;
    float denom = sin(mPhase);

    if (fabs(denom) <= std::numeric_limits<float>::epsilon()) {
      blit = mAmplitudeAtZero;
    } else {
      blit = sin(mNumberOfHarmonics * mPhase);
      blit /= mSignalPeriod * denom;
    }

    return blit;
  }

  void ComputeSine(float * aOutput, TrackTicks ticks, uint32_t aStart, uint32_t aEnd)
  {
    for (uint32_t i = aStart; i < aEnd; ++i) {
      UpdateFrequencyIfNeeded(ticks, i);

      aOutput[i] = sin(mPhase);

      IncrementPhase();
    }
  }

  void ComputeSquare(float * aOutput, TrackTicks ticks, uint32_t aStart, uint32_t aEnd)
  {
    for (uint32_t i = aStart; i < aEnd; ++i) {
      UpdateFrequencyIfNeeded(ticks, i);
      
      
      mSquare += BipolarBLIT();
      aOutput[i] = mSquare;
      
      aOutput[i] *= 1.5;
      IncrementPhase();
    }
  }

  void ComputeSawtooth(float * aOutput, TrackTicks ticks, uint32_t aStart, uint32_t aEnd)
  {
    float dcoffset;
    for (uint32_t i = aStart; i < aEnd; ++i) {
      UpdateFrequencyIfNeeded(ticks, i);
      
      dcoffset = mFinalFrequency / mSource->SampleRate();
      
      
      mSaw += UnipolarBLIT() - dcoffset;
      
      aOutput[i] = -mSaw * 1.5;

      IncrementPhase();
    }
  }

  void ComputeTriangle(float * aOutput, TrackTicks ticks, uint32_t aStart, uint32_t aEnd)
  {
    for (uint32_t i = aStart; i < aEnd; ++i) {
      UpdateFrequencyIfNeeded(ticks, i);
      
      mSquare += BipolarBLIT();
      
      
      
      
      float C6 = 0.25 / (mSource->SampleRate() / mFinalFrequency);
      mTriangle = mTriangle * sLeak + mSquare + C6;
      
      aOutput[i] = mDCBlocker.Process(mTriangle) / (mSignalPeriod/2) * 1.5;

      IncrementPhase();
    }
  }

  void ComputeSilence(AudioChunk *aOutput)
  {
    aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
  }

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished) MOZ_OVERRIDE
  {
    MOZ_ASSERT(mSource == aStream, "Invalid source stream");

    TrackTicks ticks = aStream->GetCurrentPosition();
    if (mStart == -1) {
      ComputeSilence(aOutput);
      return;
    }

    if (ticks + WEBAUDIO_BLOCK_SIZE < mStart) {
      
      ComputeSilence(aOutput);
      return;
    }
    if (ticks >= mStop) {
      
      ComputeSilence(aOutput);
      *aFinished = true;
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
        ComputeSquare(output, ticks, start, end);
        break;
      case OscillatorType::Triangle:
        ComputeTriangle(output, ticks, start, end);
        break;
      case OscillatorType::Sawtooth:
        ComputeSawtooth(output, ticks, start, end);
        break;
      default:
        ComputeSilence(aOutput);
    };

  }

  DCBlocker mDCBlocker;
  AudioNodeStream* mSource;
  AudioNodeStream* mDestination;
  TrackTicks mStart;
  TrackTicks mStop;
  AudioParamTimeline mFrequency;
  AudioParamTimeline mDetune;
  OscillatorType mType;
  float mPhase;
  float mFinalFrequency;
  uint32_t mNumberOfHarmonics;
  float mSignalPeriod;
  float mAmplitudeAtZero;
  float mPhaseIncrement;
  float mSquare;
  float mTriangle;
  float mSaw;
  float mPhaseWrap;
  bool mRecomputeFrequency;
};

OscillatorNode::OscillatorNode(AudioContext* aContext)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Max,
              ChannelInterpretation::Speakers)
  , mType(OscillatorType::Sine)
  , mFrequency(new AudioParam(MOZ_THIS_IN_INITIALIZER_LIST(),
               SendFrequencyToStream, 440.0f))
  , mDetune(new AudioParam(MOZ_THIS_IN_INITIALIZER_LIST(),
            SendDetuneToStream, 0.0f))
  , mStartCalled(false)
  , mStopped(false)
{
  OscillatorNodeEngine* engine = new OscillatorNodeEngine(this, aContext->Destination());
  mStream = aContext->Graph()->CreateAudioNodeStream(engine, MediaStreamGraph::SOURCE_STREAM);
  engine->SetSourceStream(static_cast<AudioNodeStream*> (mStream.get()));
}

OscillatorNode::~OscillatorNode()
{
  if (Context()) {
    Context()->UnregisterOscillatorNode(this);
  }
}

JSObject*
OscillatorNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return OscillatorNodeBinding::Wrap(aCx, aScope, this);
}

void
OscillatorNode::SendFrequencyToStream(AudioNode* aNode)
{
  OscillatorNode* This = static_cast<OscillatorNode*>(aNode);
  SendTimelineParameterToStream(This, OscillatorNodeEngine::FREQUENCY, *This->mFrequency);
}

void
OscillatorNode::SendDetuneToStream(AudioNode* aNode)
{
  OscillatorNode* This = static_cast<OscillatorNode*>(aNode);
  SendTimelineParameterToStream(This, OscillatorNodeEngine::DETUNE, *This->mDetune);
}

void
OscillatorNode::SendTypeToStream()
{
  SendInt32ParameterToStream(OscillatorNodeEngine::TYPE, static_cast<int32_t>(mType));
  if (mType == OscillatorType::Custom) {
    
  }
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
                             Context()->DestinationStream(),
                             aWhen);

  MOZ_ASSERT(!mPlayingRef, "We can only accept a successful start() call once");
  mPlayingRef.Take(this);
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

  mPlayingRef.Drop(this);

  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  if (!ns || !Context()) {
    
    return;
  }

  
  ns->SetStreamTimeParameter(OscillatorNodeEngine::STOP,
                             Context()->DestinationStream(),
                             std::max(0.0, aWhen));
}

void
OscillatorNode::NotifyMainThreadStateChanged()
{
  if (mStream->IsFinished()) {
    class EndedEventDispatcher : public nsRunnable
    {
    public:
      explicit EndedEventDispatcher(OscillatorNode* aNode)
        : mNode(aNode) {}
      NS_IMETHODIMP Run()
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
    if (!mStopped) {
      
      NS_DispatchToMainThread(new EndedEventDispatcher(this));
      mStopped = true;
    }

    
    
    mPlayingRef.Drop(this);
  }
}

}
}

