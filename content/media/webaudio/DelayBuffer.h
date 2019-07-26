





#ifndef DelayBuffer_h_
#define DelayBuffer_h_

#include "nsTArray.h"

namespace mozilla {

class DelayBuffer {
public:
  
  
  DelayBuffer(int aMaxDelayFrames, double aSmoothingRate)
    : mSmoothingRate(aSmoothingRate)
    , mCurrentDelay(0.)
    , mMaxDelayFrames(aMaxDelayFrames)
    , mWriteIndex(0)
  {
  }

  
  
  void Process(const double *aPerFrameDelays,
               const float* const* aInputChannels,
               float* const* aOutputChannels,
               int aChannelCount, int aFramesToProcess);

  
  
  void Process(double aDelayFrames, const float* const* aInputChannels,
               float* const* aOutputChannels, int aChannelCount,
               int aFramesToProcess);

  void Reset() { mBuffer.Clear(); };

  int MaxDelayFrames() const { return mMaxDelayFrames; }
  int ChannelCount() const { return mBuffer.Length(); }

private:
  bool EnsureBuffer(uint32_t aNumberOfChannels);

  
  AutoFallibleTArray<FallibleTArray<float>, 2> mBuffer;
  double mSmoothingRate;
  
  double mCurrentDelay;
  
  int mMaxDelayFrames;
  
  
  int mWriteIndex;
};

} 

#endif 
