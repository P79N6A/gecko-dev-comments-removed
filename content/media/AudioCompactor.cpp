




#include "AudioCompactor.h"
#if defined(MOZ_MEMORY)
# include "mozmemory.h"
#endif

namespace mozilla {

static size_t
MallocGoodSize(size_t aSize)
{
# if defined(MOZ_MEMORY)
  return malloc_good_size(aSize);
# else
  return aSize;
# endif
}

static size_t
TooMuchSlop(size_t aSize, size_t aAllocSize, size_t aMaxSlop)
{
  
  
  
  size_t slop = (aAllocSize > aSize) ? (aAllocSize - aSize) : 0;
  return slop > aMaxSlop;
}

uint32_t
AudioCompactor::GetChunkSamples(uint32_t aFrames, uint32_t aChannels,
                                size_t aMaxSlop)
{
  size_t size = AudioDataSize(aFrames, aChannels);
  size_t chunkSize = MallocGoodSize(size);

  
  
  while (chunkSize > 64 && TooMuchSlop(size, chunkSize, aMaxSlop)) {
    chunkSize = MallocGoodSize(chunkSize / 2);
  }

  
  
  return chunkSize / sizeof(AudioDataValue);
}

uint32_t
AudioCompactor::NativeCopy::operator()(AudioDataValue *aBuffer,
                                       uint32_t aSamples)
{
  NS_ASSERTION(aBuffer, "cannot copy to null buffer pointer");
  NS_ASSERTION(aSamples, "cannot copy zero values");

  size_t bufferBytes = aSamples * sizeof(AudioDataValue);
  size_t maxBytes = std::min(bufferBytes, mSourceBytes - mNextByte);
  uint32_t frames = maxBytes / BytesPerFrame(mChannels);
  size_t bytes = frames * BytesPerFrame(mChannels);

  NS_ASSERTION((mNextByte + bytes) <= mSourceBytes,
               "tried to copy beyond source buffer");
  NS_ASSERTION(bytes <= bufferBytes, "tried to copy beyond destination buffer");

  memcpy(aBuffer, mSource + mNextByte, bytes);

  mNextByte += bytes;
  return frames;
}

} 
