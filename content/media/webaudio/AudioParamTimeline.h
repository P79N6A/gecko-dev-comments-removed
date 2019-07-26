





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
  float GetValueAtTime(TimeType aTime, size_t aCounter = 0) const
  {
    MOZ_ASSERT(aCounter < WEBAUDIO_BLOCK_SIZE);
    MOZ_ASSERT(!aCounter || !HasSimpleValue());

    
    return BaseClass::GetValueAtTime(static_cast<TimeType>(aTime + aCounter)) +
           (mStream ? AudioNodeInputValue(aCounter) : 0.0f);
  }

private:
  float AudioNodeInputValue(size_t aCounter) const;

protected:
  
  nsRefPtr<MediaStream> mStream;
};

}
}

#endif

