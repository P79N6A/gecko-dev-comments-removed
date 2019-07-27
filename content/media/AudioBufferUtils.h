




#ifndef MOZILLA_SCRATCHBUFFER_H_
#define MOZILLA_SCRATCHBUFFER_H_
#include <mozilla/PodOperations.h>
#include <algorithm>

namespace mozilla {






static inline uint32_t FramesToSamples(uint32_t aChannels, uint32_t aFrames) {
  return aFrames * aChannels;
}

static inline uint32_t SamplesToFrames(uint32_t aChannels, uint32_t aSamples) {
  MOZ_ASSERT(!(aSamples % aChannels), "Frame alignment is wrong.");
  return aSamples / aChannels;
}






template<typename T, uint32_t CHANNELS>
class AudioCallbackBufferWrapper
{
public:
  AudioCallbackBufferWrapper()
    : mBuffer(nullptr),
      mSamples(0),
      mSampleWriteOffset(1)
  {}
  



  void SetBuffer(T* aBuffer, uint32_t aFrames) {
    MOZ_ASSERT(!mBuffer && !mSamples,
        "SetBuffer called twice.");
    mBuffer = aBuffer;
    mSamples = FramesToSamples(CHANNELS, aFrames);
    mSampleWriteOffset = 0;
  }

  



  void WriteFrames(T* aBuffer, uint32_t aFrames) {
    MOZ_ASSERT(aFrames <= Available(),
        "Writing more that we can in the audio buffer.");

    PodCopy(mBuffer + mSampleWriteOffset, aBuffer, FramesToSamples(CHANNELS,
                                                                   aFrames));
    mSampleWriteOffset += FramesToSamples(CHANNELS, aFrames);
  }

  


  uint32_t Available() {
    return SamplesToFrames(CHANNELS, mSamples - mSampleWriteOffset);
  }

  



  void BufferFilled() {
    
    
    
    
    
    
    
    
    NS_WARN_IF_FALSE(Available() == 0 || mSampleWriteOffset == 0,
            "Audio Buffer is not full by the end of the callback.");
    
    if (Available()) {
      PodZero(mBuffer + mSampleWriteOffset, FramesToSamples(CHANNELS, Available()));
    }
    MOZ_ASSERT(mSamples, "Buffer not set.");
    mSamples = 0;
    mSampleWriteOffset = 0;
    mBuffer = nullptr;
  }

private:
  

  T* mBuffer;
  
  uint32_t mSamples;
  

  uint32_t mSampleWriteOffset;
};







template<typename T, uint32_t BLOCK_SIZE, uint32_t CHANNELS>
class SpillBuffer
{
public:
  SpillBuffer()
  : mPosition(0)
  {
    PodArrayZero(mBuffer);
  }
  

  uint32_t Empty(AudioCallbackBufferWrapper<T, CHANNELS>& aBuffer) {
    uint32_t framesToWrite = std::min(aBuffer.Available(),
                                      SamplesToFrames(CHANNELS, mPosition));

    aBuffer.WriteFrames(mBuffer, framesToWrite);

    mPosition -= FramesToSamples(CHANNELS, framesToWrite);
    
    if (mPosition > 0) {
      PodMove(mBuffer, mBuffer + FramesToSamples(CHANNELS, framesToWrite),
              mPosition);
    }

    return framesToWrite;
  }
  

  uint32_t Fill(T* aInput, uint32_t aFrames) {
    uint32_t framesToWrite = std::min(aFrames,
                                      BLOCK_SIZE - SamplesToFrames(CHANNELS,
                                                                   mPosition));

    PodCopy(mBuffer + mPosition, aInput, FramesToSamples(CHANNELS,
                                                         framesToWrite));

    mPosition += FramesToSamples(CHANNELS, framesToWrite);

    return framesToWrite;
  }
private:
  
  T mBuffer[BLOCK_SIZE * CHANNELS];
  

  uint32_t mPosition;
};

}

#endif 
