






































#include "mozilla/dom/AudioChild.h"

namespace mozilla {
namespace dom {

AudioChild::AudioChild()
  : mLastSampleOffset(-1),
    mLastSampleOffsetTime(0)
{
  MOZ_COUNT_CTOR(AudioChild);
}

AudioChild::~AudioChild()
{
  MOZ_COUNT_DTOR(AudioChild);
}

bool
AudioChild::RecvSampleOffsetUpdate(const PRInt64& offset,
                                   const PRInt64& time)
{
  mLastSampleOffset = offset;
  mLastSampleOffsetTime = time;
  return true;
}

PRInt64
AudioChild::GetLastKnownSampleOffset()
{
  return mLastSampleOffset;
}

PRInt64
AudioChild::GetLastKnownSampleOffsetTime()
{
  return mLastSampleOffsetTime;
}

} 
} 
