





#include "ScriptProcessorNode.h"
#include "mozilla/dom/ScriptProcessorNodeBinding.h"
#include "AudioBuffer.h"
#include "AudioDestinationNode.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "AudioProcessingEvent.h"
#include "WebAudioUtils.h"
#include "nsCxPusher.h"
#include "mozilla/Mutex.h"
#include "mozilla/PodOperations.h"
#include <deque>

namespace mozilla {
namespace dom {



static const float MAX_LATENCY_S = 0.5;

NS_IMPL_ISUPPORTS_INHERITED0(ScriptProcessorNode, AudioNode)



class SharedBuffers
{
private:
  class OutputQueue
  {
  public:
    explicit OutputQueue(const char* aName)
      : mMutex(aName)
    {}

    Mutex& Lock() { return mMutex; }

    size_t ReadyToConsume() const
    {
      mMutex.AssertCurrentThreadOwns();
      MOZ_ASSERT(!NS_IsMainThread());
      return mBufferList.size();
    }

    
    AudioChunk& Produce()
    {
      mMutex.AssertCurrentThreadOwns();
      MOZ_ASSERT(NS_IsMainThread());
      mBufferList.push_back(AudioChunk());
      return mBufferList.back();
    }

    
    AudioChunk Consume()
    {
      mMutex.AssertCurrentThreadOwns();
      MOZ_ASSERT(!NS_IsMainThread());
      MOZ_ASSERT(ReadyToConsume() > 0);
      AudioChunk front = mBufferList.front();
      mBufferList.pop_front();
      return front;
    }

    
    void Clear()
    {
      mMutex.AssertCurrentThreadOwns();
      mBufferList.clear();
    }

  private:
    typedef std::deque<AudioChunk> BufferList;

    
    
    
    Mutex mMutex;
    
    BufferList mBufferList;
  };

public:
  SharedBuffers(float aSampleRate)
    : mOutputQueue("SharedBuffers::outputQueue")
    , mDelaySoFar(TRACK_TICKS_MAX)
    , mSampleRate(aSampleRate)
    , mLatency(0.0)
    , mDroppingBuffers(false)
  {
  }

  
  void FinishProducingOutputBuffer(ThreadSharedFloatArrayBufferList* aBuffer,
                                   uint32_t aBufferSize)
  {
    MOZ_ASSERT(NS_IsMainThread());

    TimeStamp now = TimeStamp::Now();

    if (mLastEventTime.IsNull()) {
      mLastEventTime = now;
    } else {
      
      
      
      
      
      
      float latency = (now - mLastEventTime).ToSeconds();
      float bufferDuration = aBufferSize / mSampleRate;
      mLatency += latency - bufferDuration;
      mLastEventTime = now;
      if (mLatency > MAX_LATENCY_S || (mDroppingBuffers && mLatency > 0.0)) {
        mDroppingBuffers = true;
        return;
      } else {
        mDroppingBuffers = false;
      }
    }

    MutexAutoLock lock(mOutputQueue.Lock());
    for (uint32_t offset = 0; offset < aBufferSize; offset += WEBAUDIO_BLOCK_SIZE) {
      AudioChunk& chunk = mOutputQueue.Produce();
      if (aBuffer) {
        chunk.mDuration = WEBAUDIO_BLOCK_SIZE;
        chunk.mBuffer = aBuffer;
        chunk.mChannelData.SetLength(aBuffer->GetChannels());
        for (uint32_t i = 0; i < aBuffer->GetChannels(); ++i) {
          chunk.mChannelData[i] = aBuffer->GetData(i) + offset;
        }
        chunk.mVolume = 1.0f;
        chunk.mBufferFormat = AUDIO_FORMAT_FLOAT32;
      } else {
        chunk.SetNull(WEBAUDIO_BLOCK_SIZE);
      }
    }
  }

  
  AudioChunk GetOutputBuffer()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    AudioChunk buffer;

    {
      MutexAutoLock lock(mOutputQueue.Lock());
      if (mOutputQueue.ReadyToConsume() > 0) {
        if (mDelaySoFar == TRACK_TICKS_MAX) {
          mDelaySoFar = 0;
        }
        buffer = mOutputQueue.Consume();
      } else {
        
        buffer.SetNull(WEBAUDIO_BLOCK_SIZE);
        if (mDelaySoFar != TRACK_TICKS_MAX) {
          
          mDelaySoFar += WEBAUDIO_BLOCK_SIZE;
        }
      }
    }

    return buffer;
  }

  TrackTicks DelaySoFar() const
  {
    MOZ_ASSERT(!NS_IsMainThread());
    return mDelaySoFar == TRACK_TICKS_MAX ? 0 : mDelaySoFar;
  }

  void Reset()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    mDelaySoFar = TRACK_TICKS_MAX;
    mLatency = 0.0f;
    {
      MutexAutoLock lock(mOutputQueue.Lock());
      mOutputQueue.Clear();
    }
    mLastEventTime = TimeStamp();
  }

private:
  OutputQueue mOutputQueue;
  
  
  
  TrackTicks mDelaySoFar;
  
  float mSampleRate;
  
  
  float mLatency;
  
  
  TimeStamp mLastEventTime;
  
  bool mDroppingBuffers;
};

class ScriptProcessorNodeEngine : public AudioNodeEngine
{
public:
  typedef nsAutoTArray<nsAutoArrayPtr<float>, 2> InputChannels;

  ScriptProcessorNodeEngine(ScriptProcessorNode* aNode,
                            AudioDestinationNode* aDestination,
                            uint32_t aBufferSize,
                            uint32_t aNumberOfInputChannels)
    : AudioNodeEngine(aNode)
    , mSharedBuffers(aNode->GetSharedBuffers())
    , mSource(nullptr)
    , mDestination(static_cast<AudioNodeStream*> (aDestination->Stream()))
    , mBufferSize(aBufferSize)
    , mInputWriteIndex(0)
    , mSeenNonSilenceInput(false)
  {
    mInputChannels.SetLength(aNumberOfInputChannels);
    AllocateInputBlock();
  }

  void SetSourceStream(AudioNodeStream* aSource)
  {
    mSource = aSource;
  }

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished) MOZ_OVERRIDE
  {
    MutexAutoLock lock(NodeMutex());

    
    if (!Node()) {
      aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
      return;
    }

    
    
    
    if (!(aStream->ConsumerCount() ||
          aStream->AsProcessedStream()->InputPortCount())) {
      aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
      mSharedBuffers->Reset();
      mSeenNonSilenceInput = false;
      mInputWriteIndex = 0;
      return;
    }

    
    for (uint32_t i = 0; i < mInputChannels.Length(); ++i) {
      if (aInput.IsNull()) {
        PodZero(mInputChannels[i] + mInputWriteIndex,
                aInput.GetDuration());
      } else {
        mSeenNonSilenceInput = true;
        MOZ_ASSERT(aInput.GetDuration() == WEBAUDIO_BLOCK_SIZE, "sanity check");
        MOZ_ASSERT(aInput.mChannelData.Length() == mInputChannels.Length());
        AudioBlockCopyChannelWithScale(static_cast<const float*>(aInput.mChannelData[i]),
                                       aInput.mVolume,
                                       mInputChannels[i] + mInputWriteIndex);
      }
    }
    mInputWriteIndex += aInput.GetDuration();

    
    
    
    *aOutput = mSharedBuffers->GetOutputBuffer();

    if (mInputWriteIndex >= mBufferSize) {
      SendBuffersToMainThread(aStream);
      mInputWriteIndex -= mBufferSize;
      mSeenNonSilenceInput = false;
      AllocateInputBlock();
    }
  }

private:
  void AllocateInputBlock()
  {
    for (unsigned i = 0; i < mInputChannels.Length(); ++i) {
      if (!mInputChannels[i]) {
        mInputChannels[i] = new float[mBufferSize];
      }
    }
  }

  void SendBuffersToMainThread(AudioNodeStream* aStream)
  {
    MOZ_ASSERT(!NS_IsMainThread());

    
    TrackTicks playbackTick = mSource->GetCurrentPosition();
    
    playbackTick += WEBAUDIO_BLOCK_SIZE;
    
    playbackTick += mSharedBuffers->DelaySoFar();
    
    double playbackTime =
      WebAudioUtils::StreamPositionToDestinationTime(playbackTick,
                                                     mSource,
                                                     mDestination);

    class Command : public nsRunnable
    {
    public:
      Command(AudioNodeStream* aStream,
              InputChannels& aInputChannels,
              double aPlaybackTime,
              bool aNullInput)
        : mStream(aStream)
        , mPlaybackTime(aPlaybackTime)
        , mNullInput(aNullInput)
      {
        mInputChannels.SetLength(aInputChannels.Length());
        if (!aNullInput) {
          for (uint32_t i = 0; i < mInputChannels.Length(); ++i) {
            mInputChannels[i] = aInputChannels[i].forget();
          }
        }
      }

      NS_IMETHODIMP Run()
      {
        
        if (!nsContentUtils::IsSafeToRunScript()) {
          nsContentUtils::AddScriptRunner(this);
          return NS_OK;
        }

        nsRefPtr<ScriptProcessorNode> node;
        {
          
          
          
          
          MutexAutoLock lock(mStream->Engine()->NodeMutex());
          node = static_cast<ScriptProcessorNode*>(mStream->Engine()->Node());
        }
        if (!node || !node->Context()) {
          return NS_OK;
        }

        AutoPushJSContext cx(node->Context()->GetJSContext());
        if (cx) {

          
          nsRefPtr<AudioBuffer> inputBuffer;
          if (!mNullInput) {
            inputBuffer = new AudioBuffer(node->Context(),
                                          node->BufferSize(),
                                          node->Context()->SampleRate());
            if (!inputBuffer->InitializeBuffers(mInputChannels.Length(), cx)) {
              return NS_OK;
            }
            
            for (uint32_t i = 0; i < mInputChannels.Length(); ++i) {
              inputBuffer->SetRawChannelContents(cx, i, mInputChannels[i]);
            }
          }

          
          
          
          
          
          nsRefPtr<AudioProcessingEvent> event = new AudioProcessingEvent(node, nullptr, nullptr);
          event->InitEvent(inputBuffer,
                           mInputChannels.Length(),
                           mPlaybackTime);
          node->DispatchTrustedEvent(event);

          
          nsRefPtr<ThreadSharedFloatArrayBufferList> output;
          if (event->HasOutputBuffer()) {
            output = event->OutputBuffer()->GetThreadSharedChannelsForRate(cx);
          }

          
          node->GetSharedBuffers()->FinishProducingOutputBuffer(output, node->BufferSize());
        }
        return NS_OK;
      }
    private:
      nsRefPtr<AudioNodeStream> mStream;
      InputChannels mInputChannels;
      double mPlaybackTime;
      bool mNullInput;
    };

    NS_DispatchToMainThread(new Command(aStream, mInputChannels,
                                        playbackTime,
                                        !mSeenNonSilenceInput));
  }

  friend class ScriptProcessorNode;

  SharedBuffers* mSharedBuffers;
  AudioNodeStream* mSource;
  AudioNodeStream* mDestination;
  InputChannels mInputChannels;
  const uint32_t mBufferSize;
  
  uint32_t mInputWriteIndex;
  bool mSeenNonSilenceInput;
};

ScriptProcessorNode::ScriptProcessorNode(AudioContext* aContext,
                                         uint32_t aBufferSize,
                                         uint32_t aNumberOfInputChannels,
                                         uint32_t aNumberOfOutputChannels)
  : AudioNode(aContext,
              aNumberOfInputChannels,
              mozilla::dom::ChannelCountMode::Explicit,
              mozilla::dom::ChannelInterpretation::Speakers)
  , mSharedBuffers(new SharedBuffers(aContext->SampleRate()))
  , mBufferSize(aBufferSize ?
                  aBufferSize : 
                  4096)         
  , mNumberOfOutputChannels(aNumberOfOutputChannels)
{
  MOZ_ASSERT(BufferSize() % WEBAUDIO_BLOCK_SIZE == 0, "Invalid buffer size");
  ScriptProcessorNodeEngine* engine =
    new ScriptProcessorNodeEngine(this,
                                  aContext->Destination(),
                                  BufferSize(),
                                  aNumberOfInputChannels);
  mStream = aContext->Graph()->CreateAudioNodeStream(engine, MediaStreamGraph::INTERNAL_STREAM);
  engine->SetSourceStream(static_cast<AudioNodeStream*> (mStream.get()));
}

ScriptProcessorNode::~ScriptProcessorNode()
{
}

JSObject*
ScriptProcessorNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return ScriptProcessorNodeBinding::Wrap(aCx, aScope, this);
}

}
}

