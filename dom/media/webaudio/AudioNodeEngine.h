




#ifndef MOZILLA_AUDIONODEENGINE_H_
#define MOZILLA_AUDIONODEENGINE_H_

#include "AudioSegment.h"
#include "mozilla/dom/AudioNode.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Mutex.h"

namespace mozilla {

namespace dom {
struct ThreeDPoint;
class AudioParamTimeline;
class DelayNodeEngine;
}

class AudioNodeStream;







class ThreadSharedFloatArrayBufferList final : public ThreadSharedObject
{
public:
  


  explicit ThreadSharedFloatArrayBufferList(uint32_t aCount)
  {
    mContents.SetLength(aCount);
  }

  struct Storage final
  {
    Storage() :
      mDataToFree(nullptr),
      mFree(nullptr),
      mSampleData(nullptr)
    {}
    ~Storage() {
      if (mFree) {
        mFree(mDataToFree);
      } else { MOZ_ASSERT(!mDataToFree); }
    }
    size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
    {
      
      
      return aMallocSizeOf(mDataToFree);
    }
    void* mDataToFree;
    void (*mFree)(void*);
    const float* mSampleData;
  };

  


  uint32_t GetChannels() const { return mContents.Length(); }
  


  const float* GetData(uint32_t aIndex) const { return mContents[aIndex].mSampleData; }

  



  void SetData(uint32_t aIndex, void* aDataToFree, void (*aFreeFunc)(void*), const float* aData)
  {
    Storage* s = &mContents[aIndex];
    if (s->mFree) {
      s->mFree(s->mDataToFree);
    } else {
      MOZ_ASSERT(!s->mDataToFree);
    }

    s->mDataToFree = aDataToFree;
    s->mFree = aFreeFunc;
    s->mSampleData = aData;
  }

  


  void Clear() { mContents.Clear(); }

  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const override
  {
    size_t amount = ThreadSharedObject::SizeOfExcludingThis(aMallocSizeOf);
    amount += mContents.SizeOfExcludingThis(aMallocSizeOf);
    for (size_t i = 0; i < mContents.Length(); i++) {
      amount += mContents[i].SizeOfExcludingThis(aMallocSizeOf);
    }

    return amount;
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

private:
  AutoFallibleTArray<Storage,2> mContents;
};





void AllocateAudioBlock(uint32_t aChannelCount, AudioChunk* aChunk);




void WriteZeroesToAudioBlock(AudioChunk* aChunk, uint32_t aStart, uint32_t aLength);




void AudioBufferCopyWithScale(const float* aInput,
                              float aScale,
                              float* aOutput,
                              uint32_t aSize);




void AudioBufferAddWithScale(const float* aInput,
                             float aScale,
                             float* aOutput,
                             uint32_t aSize);




void AudioBlockAddChannelWithScale(const float aInput[WEBAUDIO_BLOCK_SIZE],
                                   float aScale,
                                   float aOutput[WEBAUDIO_BLOCK_SIZE]);






void AudioBlockCopyChannelWithScale(const float* aInput,
                                    float aScale,
                                    float* aOutput);




void AudioBlockCopyChannelWithScale(const float aInput[WEBAUDIO_BLOCK_SIZE],
                                    const float aScale[WEBAUDIO_BLOCK_SIZE],
                                    float aOutput[WEBAUDIO_BLOCK_SIZE]);




void BufferComplexMultiply(const float* aInput,
                           const float* aScale,
                           float* aOutput,
                           uint32_t aSize);




float AudioBufferPeakValue(const float* aInput, uint32_t aSize);




void AudioBlockInPlaceScale(float aBlock[WEBAUDIO_BLOCK_SIZE],
                            float aScale);




void AudioBufferInPlaceScale(float* aBlock,
                             float aScale,
                             uint32_t aSize);






void
AudioBlockPanMonoToStereo(const float aInput[WEBAUDIO_BLOCK_SIZE],
                          float aGainL, float aGainR,
                          float aOutputL[WEBAUDIO_BLOCK_SIZE],
                          float aOutputR[WEBAUDIO_BLOCK_SIZE]);

void
AudioBlockPanMonoToStereo(const float aInput[WEBAUDIO_BLOCK_SIZE],
                          float aGainL[WEBAUDIO_BLOCK_SIZE],
                          float aGainR[WEBAUDIO_BLOCK_SIZE],
                          float aOutputL[WEBAUDIO_BLOCK_SIZE],
                          float aOutputR[WEBAUDIO_BLOCK_SIZE]);





void
AudioBlockPanStereoToStereo(const float aInputL[WEBAUDIO_BLOCK_SIZE],
                            const float aInputR[WEBAUDIO_BLOCK_SIZE],
                            float aGainL, float aGainR, bool aIsOnTheLeft,
                            float aOutputL[WEBAUDIO_BLOCK_SIZE],
                            float aOutputR[WEBAUDIO_BLOCK_SIZE]);
void
AudioBlockPanStereoToStereo(const float aInputL[WEBAUDIO_BLOCK_SIZE],
                            const float aInputR[WEBAUDIO_BLOCK_SIZE],
                            float aGainL[WEBAUDIO_BLOCK_SIZE],
                            float aGainR[WEBAUDIO_BLOCK_SIZE],
                            bool  aIsOnTheLeft[WEBAUDIO_BLOCK_SIZE],
                            float aOutputL[WEBAUDIO_BLOCK_SIZE],
                            float aOutputR[WEBAUDIO_BLOCK_SIZE]);




float
AudioBufferSumOfSquares(const float* aInput, uint32_t aLength);





class AudioNodeEngine
{
public:
  
  typedef nsAutoTArray<AudioChunk, 1> OutputChunks;

  explicit AudioNodeEngine(dom::AudioNode* aNode)
    : mNode(aNode)
    , mNodeMutex("AudioNodeEngine::mNodeMutex")
    , mInputCount(aNode ? aNode->NumberOfInputs() : 1)
    , mOutputCount(aNode ? aNode->NumberOfOutputs() : 0)
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_COUNT_CTOR(AudioNodeEngine);
  }
  virtual ~AudioNodeEngine()
  {
    MOZ_ASSERT(!mNode, "The node reference must be already cleared");
    MOZ_COUNT_DTOR(AudioNodeEngine);
  }

  virtual dom::DelayNodeEngine* AsDelayNodeEngine() { return nullptr; }

  virtual void SetStreamTimeParameter(uint32_t aIndex, StreamTime aParam)
  {
    NS_ERROR("Invalid SetStreamTimeParameter index");
  }
  virtual void SetDoubleParameter(uint32_t aIndex, double aParam)
  {
    NS_ERROR("Invalid SetDoubleParameter index");
  }
  virtual void SetInt32Parameter(uint32_t aIndex, int32_t aParam)
  {
    NS_ERROR("Invalid SetInt32Parameter index");
  }
  virtual void SetTimelineParameter(uint32_t aIndex,
                                    const dom::AudioParamTimeline& aValue,
                                    TrackRate aSampleRate)
  {
    NS_ERROR("Invalid SetTimelineParameter index");
  }
  virtual void SetThreeDPointParameter(uint32_t aIndex,
                                       const dom::ThreeDPoint& aValue)
  {
    NS_ERROR("Invalid SetThreeDPointParameter index");
  }
  virtual void SetBuffer(already_AddRefed<ThreadSharedFloatArrayBufferList> aBuffer)
  {
    NS_ERROR("SetBuffer called on engine that doesn't support it");
  }
  
  virtual void SetRawArrayData(nsTArray<float>& aData)
  {
    NS_ERROR("SetRawArrayData called on an engine that doesn't support it");
  }

  








  virtual void ProcessBlock(AudioNodeStream* aStream,
                            const AudioChunk& aInput,
                            AudioChunk* aOutput,
                            bool* aFinished)
  {
    MOZ_ASSERT(mInputCount <= 1 && mOutputCount <= 1);
    *aOutput = aInput;
  }
  




  virtual void ProduceBlockBeforeInput(AudioChunk* aOutput)
  {
    NS_NOTREACHED("ProduceBlockBeforeInput called on wrong engine\n");
  }

  














  virtual void ProcessBlocksOnPorts(AudioNodeStream* aStream,
                                    const OutputChunks& aInput,
                                    OutputChunks& aOutput,
                                    bool* aFinished)
  {
    MOZ_ASSERT(mInputCount > 1 || mOutputCount > 1);
    
    aOutput[0] = aInput[0];
  }

  Mutex& NodeMutex() { return mNodeMutex;}

  bool HasNode() const
  {
    return !!mNode;
  }

  dom::AudioNode* Node() const
  {
    mNodeMutex.AssertCurrentThreadOwns();
    return mNode;
  }

  dom::AudioNode* NodeMainThread() const
  {
    MOZ_ASSERT(NS_IsMainThread());
    return mNode;
  }

  void ClearNode()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mNode != nullptr);
    mNodeMutex.AssertCurrentThreadOwns();
    mNode = nullptr;
  }

  uint16_t InputCount() const { return mInputCount; }
  uint16_t OutputCount() const { return mOutputCount; }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    
    return 0;
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  void SizeOfIncludingThis(MallocSizeOf aMallocSizeOf,
                           AudioNodeSizes& aUsage) const
  {
    aUsage.mEngine = SizeOfIncludingThis(aMallocSizeOf);
    if (HasNode()) {
      aUsage.mDomNode = mNode->SizeOfIncludingThis(aMallocSizeOf);
      aUsage.mNodeType = mNode->NodeType();
    }
  }

private:
  dom::AudioNode* mNode;
  Mutex mNodeMutex;
  const uint16_t mInputCount;
  const uint16_t mOutputCount;
};

}

#endif 
