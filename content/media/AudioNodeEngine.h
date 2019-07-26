




#ifndef MOZILLA_AUDIONODEENGINE_H_
#define MOZILLA_AUDIONODEENGINE_H_

#include "AudioSegment.h"
#include "mozilla/dom/AudioParam.h"

namespace mozilla {

namespace dom {
struct ThreeDPoint;
}

class AudioNodeStream;



const uint32_t WEBAUDIO_BLOCK_SIZE_BITS = 7;
const uint32_t WEBAUDIO_BLOCK_SIZE = 1 << WEBAUDIO_BLOCK_SIZE_BITS;







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




void AudioBlockCopyChannelWithScale(const float aInput[WEBAUDIO_BLOCK_SIZE],
                                    float aScale,
                                    float aOutput[WEBAUDIO_BLOCK_SIZE]);




void AudioBlockCopyChannelWithScale(const float aInput[WEBAUDIO_BLOCK_SIZE],
                                    const float aScale[WEBAUDIO_BLOCK_SIZE],
                                    float aOutput[WEBAUDIO_BLOCK_SIZE]);





class AudioNodeEngine {
public:
  AudioNodeEngine() {}
  virtual ~AudioNodeEngine() {}

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
                                    const dom::AudioParamTimeline& aValue)
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

  









  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished)
  {
    *aOutput = aInput;
  }
};

}

#endif 
