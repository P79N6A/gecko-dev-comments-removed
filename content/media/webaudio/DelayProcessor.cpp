





#include "DelayProcessor.h"

#include "mozilla/PodOperations.h"
#include "AudioSegment.h"

namespace mozilla {

void
DelayProcessor::Process(const double *aPerFrameDelays,
                        const float* const* aInputChannels,
                        float* const* aOutputChannels,
                        int aChannelCount, int aFramesToProcess)
{
  if (!EnsureBuffer(aChannelCount)) {
    for (int channel = 0; channel < aChannelCount; ++channel) {
      PodZero(aOutputChannels[channel], aFramesToProcess);
    }
    return;
  }

  for (int channel = 0; channel < aChannelCount; ++channel) {
    double currentDelayFrames = mCurrentDelay;
    int writeIndex = mWriteIndex;

    float* buffer = mBuffer[channel].Elements();
    const uint32_t bufferLength = mBuffer[channel].Length();
    const float* input = aInputChannels ? aInputChannels[channel] : nullptr;
    float* output = aOutputChannels[channel];

    for (int i = 0; i < aFramesToProcess; ++i) {
      currentDelayFrames = clamped(aPerFrameDelays[i],
                                   0.0, static_cast<double>(mMaxDelayFrames));

      
      if (input) {
        buffer[writeIndex] = input[i];
      }

      
      
      
      double readPosition = writeIndex + bufferLength - currentDelayFrames;
      if (readPosition >= bufferLength) {
        readPosition -= bufferLength;
      }
      MOZ_ASSERT(readPosition >= 0.0, "Why are we reading before the beginning of the buffer?");

      
      
      
      
      
      
      
      
      
      int readIndex1 = int(readPosition);
      int readIndex2 = (readIndex1 + 1) % bufferLength;
      double interpolationFactor = readPosition - readIndex1;

      output[i] = (1.0 - interpolationFactor) * buffer[readIndex1] +
                         interpolationFactor  * buffer[readIndex2];
      writeIndex = (writeIndex + 1) % bufferLength;
    }

    
    
    if (channel == aChannelCount - 1) {
      mCurrentDelay = currentDelayFrames;
      mWriteIndex = writeIndex;
    }
  }
}

void
DelayProcessor::Process(double aDelayFrames, const float* const* aInputChannels,
                        float* const* aOutputChannels, int aChannelCount,
                        int aFramesToProcess)
{
  const bool firstTime = !mBuffer.Length();
  double currentDelay = firstTime ? aDelayFrames : mCurrentDelay;

  nsAutoTArray<double, WEBAUDIO_BLOCK_SIZE> computedDelay;
  computedDelay.SetLength(aFramesToProcess);

  for (int i = 0; i < aFramesToProcess; ++i) {
    
    currentDelay += (aDelayFrames - currentDelay) * mSmoothingRate;
    computedDelay[i] = currentDelay;
  }

  Process(computedDelay.Elements(), aInputChannels, aOutputChannels,
          aChannelCount, aFramesToProcess);
}

bool
DelayProcessor::EnsureBuffer(uint32_t aNumberOfChannels)
{
  if (aNumberOfChannels == 0) {
    return false;
  }
  if (mBuffer.Length() == 0) {
    if (!mBuffer.SetLength(aNumberOfChannels)) {
      return false;
    }
    const int numFrames = mMaxDelayFrames;
    for (uint32_t channel = 0; channel < aNumberOfChannels; ++channel) {
      if (!mBuffer[channel].SetLength(numFrames)) {
        return false;
      }
      PodZero(mBuffer[channel].Elements(), numFrames);
    }
  } else if (mBuffer.Length() != aNumberOfChannels) {
    
    return false;
  }
  return true;
}

} 
