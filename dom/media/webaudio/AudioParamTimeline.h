





#ifndef AudioParamTimeline_h_
#define AudioParamTimeline_h_

#include "AudioEventTimeline.h"
#include "mozilla/ErrorResult.h"
#include "nsAutoPtr.h"
#include "MediaStreamGraph.h"
#include "AudioSegment.h"

namespace mozilla {

namespace dom {








class AudioParamTimeline : public AudioEventTimeline<ErrorResult>
{
  typedef AudioEventTimeline<ErrorResult> BaseClass;

public:
  explicit AudioParamTimeline(float aDefaultValue)
    : BaseClass(aDefaultValue)
  {
  }

  MediaStream* Stream() const
  {
    return mStream;
  }

  bool HasSimpleValue() const
  {
    return BaseClass::HasSimpleValue() && !mStream;
  }

  template<class TimeType>
  float GetValueAtTime(TimeType aTime)
  {
    return GetValueAtTime(aTime, 0);
  }

  
  
  
  template<class TimeType>
  float GetValueAtTime(TimeType aTime, size_t aCounter);

  
  
  
  
  
  
  template<class TimeType>
  void GetValuesAtTime(TimeType aTime, float* aBuffer, const size_t aSize);

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    return mStream ? mStream->SizeOfIncludingThis(aMallocSizeOf) : 0;
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }


private:
  float AudioNodeInputValue(size_t aCounter) const;

protected:
  
  nsRefPtr<MediaStream> mStream;
};

template<> inline float
AudioParamTimeline::GetValueAtTime(double aTime, size_t aCounter)
{
  MOZ_ASSERT(!aCounter);

  
  
  return BaseClass::GetValueAtTime(aTime);
}

template<> inline float
AudioParamTimeline::GetValueAtTime(int64_t aTime, size_t aCounter)
{
  MOZ_ASSERT(aCounter < WEBAUDIO_BLOCK_SIZE);
  MOZ_ASSERT(!aCounter || !HasSimpleValue());

  
  return BaseClass::GetValueAtTime(static_cast<int64_t>(aTime + aCounter)) +
    (mStream ? AudioNodeInputValue(aCounter) : 0.0f);
}

template<> inline void
AudioParamTimeline::GetValuesAtTime(double aTime, float* aBuffer,
                                    const size_t aSize)
{
  MOZ_ASSERT(aBuffer);
  MOZ_ASSERT(aSize == 1);

  
  
  *aBuffer = BaseClass::GetValueAtTime(aTime);
}

template<> inline void
AudioParamTimeline::GetValuesAtTime(int64_t aTime, float* aBuffer,
                                    const size_t aSize)
{
  MOZ_ASSERT(aBuffer);
  MOZ_ASSERT(aSize <= WEBAUDIO_BLOCK_SIZE);
  MOZ_ASSERT(aSize == 1 || !HasSimpleValue());

  
  BaseClass::GetValuesAtTime(aTime, aBuffer, aSize);
  if (mStream) {
    for (size_t i = 0; i < aSize; ++i) {
      aBuffer[i] += AudioNodeInputValue(i);
    }
  }
}

}
}

#endif
