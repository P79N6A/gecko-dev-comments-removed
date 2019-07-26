





#include "mozilla/dom/AnalyserNode.h"
#include "mozilla/dom/AnalyserNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "mozilla/Mutex.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS_INHERITED0(AnalyserNode, AudioNode)

class AnalyserNodeEngine : public AudioNodeEngine
{
public:
  explicit AnalyserNodeEngine(AnalyserNode& aNode)
    : mMutex("AnalyserNodeEngine")
    , mNode(&aNode)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  void DisconnectFromNode()
  {
    MutexAutoLock lock(mMutex);
    mNode = nullptr;
  }

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished)
  {
    *aOutput = aInput;

    class TransferBuffer : public nsRunnable
    {
    public:
      TransferBuffer(AnalyserNode* aNode,
                     const AudioChunk& aChunk)
        : mNode(aNode)
        , mChunk(aChunk)
      {
      }

      NS_IMETHOD Run()
      {
        mNode->AppendChunk(mChunk);
        return NS_OK;
      }

    private:
      AnalyserNode* mNode;
      AudioChunk mChunk;
    };

    MutexAutoLock lock(mMutex);
    if (mNode &&
        aInput.mChannelData.Length() > 0) {
      nsRefPtr<TransferBuffer> transfer = new TransferBuffer(mNode, aInput);
      NS_DispatchToMainThread(transfer);
    }
  }

private:
  Mutex mMutex;
  AnalyserNode* mNode; 
};

AnalyserNode::AnalyserNode(AudioContext* aContext)
  : AudioNode(aContext)
  , mFFTSize(2048)
  , mMinDecibels(-100.)
  , mMaxDecibels(-30.)
  , mSmoothingTimeConstant(.8)
  , mWriteIndex(0)
{
  mStream = aContext->Graph()->CreateAudioNodeStream(new AnalyserNodeEngine(*this),
                                                     MediaStreamGraph::INTERNAL_STREAM);
  AllocateBuffer();
}

AnalyserNode::~AnalyserNode()
{
  DestroyMediaStream();
}

JSObject*
AnalyserNode::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return AnalyserNodeBinding::Wrap(aCx, aScope, this);
}

void
AnalyserNode::SetFftSize(uint32_t aValue, ErrorResult& aRv)
{
  
  if (aValue < 2 ||
      (aValue & (aValue - 1)) != 0) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }
  if (mFFTSize != aValue) {
    mFFTSize = aValue;
    AllocateBuffer();
  }
}

void
AnalyserNode::SetMinDecibels(double aValue, ErrorResult& aRv)
{
  if (aValue >= mMaxDecibels) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }
  mMinDecibels = aValue;
}

void
AnalyserNode::SetMaxDecibels(double aValue, ErrorResult& aRv)
{
  if (aValue <= mMinDecibels) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }
  mMaxDecibels = aValue;
}

void
AnalyserNode::SetSmoothingTimeConstant(double aValue, ErrorResult& aRv)
{
  if (aValue < 0 || aValue > 1) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }
  mSmoothingTimeConstant = aValue;
}

void
AnalyserNode::GetFloatFrequencyData(Float32Array& aArray)
{
}

void
AnalyserNode::GetByteFrequencyData(Uint8Array& aArray)
{
}

void
AnalyserNode::GetByteTimeDomainData(Uint8Array& aArray)
{
}

void
AnalyserNode::DestroyMediaStream()
{
  if (mStream) {
    AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
    AnalyserNodeEngine* engine = static_cast<AnalyserNodeEngine*>(ns->Engine());
    engine->DisconnectFromNode();
  }
  AudioNode::DestroyMediaStream();
}

bool
AnalyserNode::AllocateBuffer()
{
  bool result = true;
  if (mBuffer.Length() != mFFTSize) {
    result = mBuffer.SetLength(mFFTSize);
    if (result) {
      memset(mBuffer.Elements(), 0, sizeof(float) * mFFTSize);
      mWriteIndex = 0;
    }
  }
  return result;
}

void
AnalyserNode::AppendChunk(const AudioChunk& aChunk)
{
  const uint32_t bufferSize = mBuffer.Length();
  const uint32_t channelCount = aChunk.mChannelData.Length();
  const uint32_t chunkCount = aChunk.mDuration;
  MOZ_ASSERT((bufferSize & (bufferSize - 1)) == 0); 
  MOZ_ASSERT(channelCount > 0);
  MOZ_ASSERT(chunkCount == WEBAUDIO_BLOCK_SIZE);

  memcpy(mBuffer.Elements() + mWriteIndex, aChunk.mChannelData[0], sizeof(float) * chunkCount);
  for (uint32_t i = 1; i < channelCount; ++i) {
    AudioBlockAddChannelWithScale(static_cast<const float*>(aChunk.mChannelData[i]), 1.0f,
                                  mBuffer.Elements() + mWriteIndex);
  }
  if (channelCount > 1) {
    AudioBlockInPlaceScale(mBuffer.Elements() + mWriteIndex, 1,
                           1.0f / aChunk.mChannelData.Length());
  }
  mWriteIndex += chunkCount;
  MOZ_ASSERT(mWriteIndex <= bufferSize);
  if (mWriteIndex >= bufferSize) {
    mWriteIndex = 0;
  }
}

}
}

