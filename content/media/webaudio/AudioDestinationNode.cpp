





#include "AudioDestinationNode.h"
#include "mozilla/dom/AudioDestinationNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "MediaStreamGraph.h"
#include "OfflineAudioCompletionEvent.h"

namespace mozilla {
namespace dom {

class OfflineDestinationNodeEngine : public AudioNodeEngine
{
public:
  typedef AutoFallibleTArray<nsAutoArrayPtr<float>, 2> InputChannels;

  OfflineDestinationNodeEngine(AudioDestinationNode* aNode,
                               uint32_t aNumberOfChannels,
                               uint32_t aLength,
                               float aSampleRate)
    : AudioNodeEngine(aNode)
    , mWriteIndex(0)
    , mLength(aLength)
    , mSampleRate(aSampleRate)
  {
    
    
    
    if (mInputChannels.SetLength(aNumberOfChannels)) {
      static const fallible_t fallible = fallible_t();
      for (uint32_t i = 0; i < aNumberOfChannels; ++i) {
        mInputChannels[i] = new(fallible) float[aLength];
        if (!mInputChannels[i]) {
          mInputChannels.Clear();
          break;
        }
      }
    }
  }

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished) MOZ_OVERRIDE
  {
    
    
    *aOutput = aInput;

    
    if (mInputChannels.IsEmpty()) {
      return;
    }

    
    MOZ_ASSERT(mWriteIndex < mLength, "How did this happen?");
    const uint32_t duration = std::min(WEBAUDIO_BLOCK_SIZE, mLength - mWriteIndex);
    for (uint32_t i = 0; i < mInputChannels.Length(); ++i) {
      if (aInput.IsNull()) {
        PodZero(mInputChannels[i] + mWriteIndex, duration);
      } else {
        const float* inputBuffer = static_cast<const float*>(aInput.mChannelData[i]);
        if (duration == WEBAUDIO_BLOCK_SIZE) {
          
          AudioBlockCopyChannelWithScale(inputBuffer, aInput.mVolume,
                                         mInputChannels[i] + mWriteIndex);
        } else {
          if (aInput.mVolume == 1.0f) {
            PodCopy(mInputChannels[i] + mWriteIndex, inputBuffer, duration);
          } else {
            for (uint32_t j = 0; j < duration; ++j) {
              mInputChannels[i][mWriteIndex + j] = aInput.mVolume * inputBuffer[j];
            }
          }
        }
      }
    }
    mWriteIndex += duration;

    if (mWriteIndex == mLength) {
      SendBufferToMainThread(aStream);
      *aFinished = true;
    }
  }

  void SendBufferToMainThread(AudioNodeStream* aStream)
  {
    class Command : public nsRunnable
    {
    public:
      Command(AudioNodeStream* aStream,
              InputChannels& aInputChannels,
              uint32_t aLength,
              float aSampleRate)
        : mStream(aStream)
        , mLength(aLength)
        , mSampleRate(aSampleRate)
      {
        mInputChannels.SwapElements(aInputChannels);
      }

      NS_IMETHODIMP Run()
      {
        
        if (!nsContentUtils::IsSafeToRunScript()) {
          nsContentUtils::AddScriptRunner(this);
          return NS_OK;
        }

        nsRefPtr<AudioContext> context;
        {
          MutexAutoLock lock(mStream->Engine()->NodeMutex());
          AudioNode* node = mStream->Engine()->Node();
          if (node) {
            context = node->Context();
          }
        }
        if (!context) {
          return NS_OK;
        }

        AutoPushJSContext cx(context->GetJSContext());
        if (cx) {
          JSAutoRequest ar(cx);

          
          nsRefPtr<AudioBuffer> renderedBuffer = new AudioBuffer(context,
                                                                 mLength,
                                                                 mSampleRate);
          if (!renderedBuffer->InitializeBuffers(mInputChannels.Length(), cx)) {
            return NS_OK;
          }
          for (uint32_t i = 0; i < mInputChannels.Length(); ++i) {
            renderedBuffer->SetRawChannelContents(cx, i, mInputChannels[i]);
          }

          nsRefPtr<OfflineAudioCompletionEvent> event =
              new OfflineAudioCompletionEvent(context, nullptr, nullptr);
          event->InitEvent(renderedBuffer);
          context->DispatchTrustedEvent(event);
        }

        return NS_OK;
      }
    private:
      nsRefPtr<AudioNodeStream> mStream;
      InputChannels mInputChannels;
      uint32_t mLength;
      float mSampleRate;
    };

    
    
    NS_DispatchToMainThread(new Command(aStream, mInputChannels, mLength, mSampleRate));
  }

private:
  
  
  
  InputChannels mInputChannels;
  
  uint32_t mWriteIndex;
  
  uint32_t mLength;
  float mSampleRate;
};

NS_IMPL_ISUPPORTS_INHERITED0(AudioDestinationNode, AudioNode)

AudioDestinationNode::AudioDestinationNode(AudioContext* aContext,
                                           bool aIsOffline,
                                           uint32_t aNumberOfChannels,
                                           uint32_t aLength,
                                           float aSampleRate)
  : AudioNode(aContext,
              aIsOffline ? aNumberOfChannels : 2,
              ChannelCountMode::Explicit,
              ChannelInterpretation::Speakers)
  , mFramesToProduce(aLength)
{
  MediaStreamGraph* graph = aIsOffline ?
                            MediaStreamGraph::CreateNonRealtimeInstance() :
                            MediaStreamGraph::GetInstance();
  AudioNodeEngine* engine = aIsOffline ?
                            new OfflineDestinationNodeEngine(this, aNumberOfChannels,
                                                             aLength, aSampleRate) :
                            new AudioNodeEngine(this);
  mStream = graph->CreateAudioNodeStream(engine, MediaStreamGraph::EXTERNAL_STREAM);
}

JSObject*
AudioDestinationNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return AudioDestinationNodeBinding::Wrap(aCx, aScope, this);
}

void
AudioDestinationNode::StartRendering()
{
  mStream->Graph()->StartNonRealtimeProcessing(mFramesToProduce);
}

}
}
