




#ifndef MOZILLA_AUDIONODEENGINE_H_
#define MOZILLA_AUDIONODEENGINE_H_

#include "AudioSegment.h"
#include "mozilla/dom/AudioNode.h"
#include "mozilla/dom/AudioParam.h"
#include "mozilla/Mutex.h"

namespace mozilla {

namespace dom {
struct ThreeDPoint;
}

class AudioNodeStream;







class ThreadSharedFloatArrayBufferList : public ThreadSharedObject {
public:
  


  ThreadSharedFloatArrayBufferList(uint32_t aCount)
  {
    mContents.SetLength(aCount);
  }

  struct Storage {
    Storage()
    {
      mDataToFree = nullptr;
      mSampleData = nullptr;
    }
    ~Storage() { free(mDataToFree); }
    void* mDataToFree;
    const float* mSampleData;
  };

  


  uint32_t GetChannels() const { return mContents.Length(); }
  


  const float* GetData(uint32_t aIndex) const { return mContents[aIndex].mSampleData; }

  



  void SetData(uint32_t aIndex, void* aDataToFree, const float* aData)
  {
    Storage* s = &mContents[aIndex];
    free(s->mDataToFree);
    s->mDataToFree = aDataToFree;
    s->mSampleData = aData;
  }

  


  void Clear() { mContents.Clear(); }

private:
  AutoFallibleTArray<Storage,2> mContents;
};





void AllocateAudioBlock(uint32_t aChannelCount, AudioChunk* aChunk);




void WriteZeroesToAudioBlock(AudioChunk* aChunk, uint32_t aStart, uint32_t aLength);




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




void AudioBlockInPlaceScale(float aBlock[WEBAUDIO_BLOCK_SIZE],
                            uint32_t aChannelCount,
                            float aScale);






void
AudioBlockPanMonoToStereo(const float aInput[WEBAUDIO_BLOCK_SIZE],
                          float aGainL, float aGainR,
                          float aOutputL[WEBAUDIO_BLOCK_SIZE],
                          float aOutputR[WEBAUDIO_BLOCK_SIZE]);





void
AudioBlockPanStereoToStereo(const float aInputL[WEBAUDIO_BLOCK_SIZE],
                            const float aInputR[WEBAUDIO_BLOCK_SIZE],
                            float aGainL, float aGainR, bool aIsOnTheLeft,
                            float aOutputL[WEBAUDIO_BLOCK_SIZE],
                            float aOutputR[WEBAUDIO_BLOCK_SIZE]);





class AudioNodeEngine {
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

  virtual void SetStreamTimeParameter(uint32_t aIndex, TrackTicks aParam)
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

  








  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished)
  {
    MOZ_ASSERT(mInputCount <= 1 && mOutputCount <= 1);
    *aOutput = aInput;
  }

  














  virtual void ProduceAudioBlocksOnPorts(AudioNodeStream* aStream,
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

private:
  dom::AudioNode* mNode;
  Mutex mNodeMutex;
  const uint16_t mInputCount;
  const uint16_t mOutputCount;
};

}

#endif 
