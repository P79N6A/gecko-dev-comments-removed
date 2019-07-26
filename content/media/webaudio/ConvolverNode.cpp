





#include "ConvolverNode.h"
#include "mozilla/dom/ConvolverNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "blink/Reverb.h"
#include "PlayingRefChangeHandler.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(ConvolverNode, AudioNode, mBuffer)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ConvolverNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(ConvolverNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(ConvolverNode, AudioNode)

class ConvolverNodeEngine : public AudioNodeEngine
{
  typedef PlayingRefChangeHandler PlayingRefChanged;
public:
  ConvolverNodeEngine(AudioNode* aNode, bool aNormalize)
    : AudioNodeEngine(aNode)
    , mBufferLength(0)
    , mLeftOverData(INT32_MIN)
    , mSampleRate(0.0f)
    , mUseBackgroundThreads(!aNode->Context()->IsOffline())
    , mNormalize(aNormalize)
    , mSeenInput(false)
  {
  }

  enum Parameters {
    BUFFER_LENGTH,
    SAMPLE_RATE,
    NORMALIZE
  };
  virtual void SetInt32Parameter(uint32_t aIndex, int32_t aParam) MOZ_OVERRIDE
  {
    switch (aIndex) {
    case BUFFER_LENGTH:
      
      
      mBuffer = nullptr;
      mSampleRate = 0.0f;
      mBufferLength = aParam;
      mLeftOverData = INT32_MIN;
      break;
    case SAMPLE_RATE:
      mSampleRate = aParam;
      break;
    case NORMALIZE:
      mNormalize = !!aParam;
      break;
    default:
      NS_ERROR("Bad ConvolverNodeEngine Int32Parameter");
    }
  }
  virtual void SetDoubleParameter(uint32_t aIndex, double aParam) MOZ_OVERRIDE
  {
    switch (aIndex) {
    case SAMPLE_RATE:
      mSampleRate = aParam;
      AdjustReverb();
      break;
    default:
      NS_ERROR("Bad ConvolverNodeEngine DoubleParameter");
    }
  }
  virtual void SetBuffer(already_AddRefed<ThreadSharedFloatArrayBufferList> aBuffer)
  {
    mBuffer = aBuffer;
    AdjustReverb();
  }

  void AdjustReverb()
  {
    
    
    
    
    
    
    const size_t MaxFFTSize = 32768;

    if (!mBuffer || !mBufferLength || !mSampleRate) {
      mReverb = nullptr;
      mSeenInput = false;
      mLeftOverData = INT32_MIN;
      return;
    }

    mReverb = new WebCore::Reverb(mBuffer, mBufferLength,
                                  WEBAUDIO_BLOCK_SIZE,
                                  MaxFFTSize, 2, mUseBackgroundThreads,
                                  mNormalize, mSampleRate);
  }

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished)
  {
    if (!mSeenInput && aInput.IsNull()) {
      aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
      return;
    }
    if (!mReverb) {
      *aOutput = aInput;
      return;
    }

    mSeenInput = true;
    AudioChunk input = aInput;
    if (aInput.IsNull()) {
      AllocateAudioBlock(1, &input);
      WriteZeroesToAudioBlock(&input, 0, WEBAUDIO_BLOCK_SIZE);

      if (mLeftOverData > 0) {
        mLeftOverData -= WEBAUDIO_BLOCK_SIZE;
      } else if (mLeftOverData != INT32_MIN) {
        mLeftOverData = INT32_MIN;
        nsRefPtr<PlayingRefChanged> refchanged =
          new PlayingRefChanged(aStream, PlayingRefChanged::RELEASE);
        NS_DispatchToMainThread(refchanged);
      }
    } else {
      if (aInput.mVolume != 1.0f) {
        
        uint32_t numChannels = aInput.mChannelData.Length();
        AllocateAudioBlock(numChannels, &input);
        for (uint32_t i = 0; i < numChannels; ++i) {
          const float* src = static_cast<const float*>(aInput.mChannelData[i]);
          float* dest = static_cast<float*>(const_cast<void*>(input.mChannelData[i]));
          AudioBlockCopyChannelWithScale(src, aInput.mVolume, dest);
        }
      }

      if (mLeftOverData <= 0) {
        nsRefPtr<PlayingRefChanged> refchanged =
          new PlayingRefChanged(aStream, PlayingRefChanged::ADDREF);
        NS_DispatchToMainThread(refchanged);
      }
      mLeftOverData = mBufferLength;
      MOZ_ASSERT(mLeftOverData > 0);
    }
    AllocateAudioBlock(2, aOutput);

    mReverb->process(&input, aOutput, WEBAUDIO_BLOCK_SIZE);
  }

private:
  nsRefPtr<ThreadSharedFloatArrayBufferList> mBuffer;
  nsAutoPtr<WebCore::Reverb> mReverb;
  int32_t mBufferLength;
  int32_t mLeftOverData;
  float mSampleRate;
  bool mUseBackgroundThreads;
  bool mNormalize;
  bool mSeenInput;
};

ConvolverNode::ConvolverNode(AudioContext* aContext)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Clamped_max,
              ChannelInterpretation::Speakers)
  , mNormalize(true)
{
  ConvolverNodeEngine* engine = new ConvolverNodeEngine(this, mNormalize);
  mStream = aContext->Graph()->CreateAudioNodeStream(engine, MediaStreamGraph::INTERNAL_STREAM);
}

JSObject*
ConvolverNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return ConvolverNodeBinding::Wrap(aCx, aScope, this);
}

void
ConvolverNode::SetBuffer(JSContext* aCx, AudioBuffer* aBuffer, ErrorResult& aRv)
{
  if (aBuffer) {
    switch (aBuffer->NumberOfChannels()) {
    case 1:
    case 2:
    case 4:
      
      break;
    default:
      aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
      return;
    }
  }

  mBuffer = aBuffer;

  
  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  MOZ_ASSERT(ns, "Why don't we have a stream here?");
  if (mBuffer) {
    uint32_t length = mBuffer->Length();
    nsRefPtr<ThreadSharedFloatArrayBufferList> data =
      mBuffer->GetThreadSharedChannelsForRate(aCx);
    if (data && length < WEBAUDIO_BLOCK_SIZE) {
      
      
      
      length = WEBAUDIO_BLOCK_SIZE;
      nsRefPtr<ThreadSharedFloatArrayBufferList> paddedBuffer =
        new ThreadSharedFloatArrayBufferList(data->GetChannels());
      float* channelData = (float*) malloc(sizeof(float) * length * data->GetChannels());
      for (uint32_t i = 0; i < data->GetChannels(); ++i) {
        PodCopy(channelData + length * i, data->GetData(i), mBuffer->Length());
        PodZero(channelData + length * i + mBuffer->Length(), WEBAUDIO_BLOCK_SIZE - mBuffer->Length());
        paddedBuffer->SetData(i, (i == 0) ? channelData : nullptr, channelData);
      }
      data = paddedBuffer;
    }
    SendInt32ParameterToStream(ConvolverNodeEngine::BUFFER_LENGTH, length);
    SendDoubleParameterToStream(ConvolverNodeEngine::SAMPLE_RATE,
                                mBuffer->SampleRate());
    ns->SetBuffer(data.forget());
  } else {
    ns->SetBuffer(nullptr);
  }
}

void
ConvolverNode::SetNormalize(bool aNormalize)
{
  mNormalize = aNormalize;
  SendInt32ParameterToStream(ConvolverNodeEngine::NORMALIZE, aNormalize);
}

}
}

