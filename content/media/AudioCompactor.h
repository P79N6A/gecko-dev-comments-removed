




#if !defined(AudioCompactor_h)
#define AudioCompactor_h

#include "MediaQueue.h"
#include "MediaData.h"
#include "VideoUtils.h"

namespace mozilla {

class AudioCompactor
{
public:
  explicit AudioCompactor(MediaQueue<AudioData>& aQueue)
    : mQueue(aQueue)
  { }

  
  
  
  
  
  
  
  
  
  
  
  
  template<typename CopyFunc>
  bool Push(int64_t aOffset, int64_t aTime, int32_t aSampleRate,
            uint32_t aFrames, uint32_t aChannels, CopyFunc aCopyFunc)
  {
    
    
    size_t maxSlop = AudioDataSize(aFrames, aChannels) / MAX_SLOP_DIVISOR;

    while (aFrames > 0) {
      uint32_t samples = GetChunkSamples(aFrames, aChannels, maxSlop);
      nsAutoArrayPtr<AudioDataValue> buffer(new AudioDataValue[samples]);

      
      uint32_t framesCopied = aCopyFunc(buffer, samples);

      NS_ASSERTION(framesCopied <= aFrames, "functor copied too many frames");

      CheckedInt64 duration = FramesToUsecs(framesCopied, aSampleRate);
      if (!duration.isValid()) {
        return false;
      }

      mQueue.Push(new AudioData(aOffset,
                                aTime,
                                duration.value(),
                                framesCopied,
                                buffer.forget(),
                                aChannels,
                                aSampleRate));

      
      
      aTime += duration.value();
      aFrames -= framesCopied;

      
    }

    return true;
  }

  
  
  class NativeCopy
  {
  public:
    NativeCopy(const uint8_t* aSource, size_t aSourceBytes,
               uint32_t aChannels)
      : mSource(aSource)
      , mSourceBytes(aSourceBytes)
      , mChannels(aChannels)
      , mNextByte(0)
    { }

    uint32_t operator()(AudioDataValue *aBuffer, uint32_t aSamples);

  private:
    const uint8_t* const mSource;
    const size_t mSourceBytes;
    const uint32_t mChannels;
    size_t mNextByte;
  };

  
  
  static const size_t MAX_SLOP_DIVISOR = 8;

private:
  
  
  static uint32_t
  GetChunkSamples(uint32_t aFrames, uint32_t aChannels, size_t aMaxSlop);

  static size_t BytesPerFrame(uint32_t aChannels)
  {
    return sizeof(AudioDataValue) * aChannels;
  }

  static size_t AudioDataSize(uint32_t aFrames, uint32_t aChannels)
  {
    return aFrames * BytesPerFrame(aChannels);
  }

  MediaQueue<AudioData> &mQueue;
};

} 

#endif 
