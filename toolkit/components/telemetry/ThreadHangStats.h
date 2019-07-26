




#ifndef mozilla_BackgroundHangTelemetry_h
#define mozilla_BackgroundHangTelemetry_h

#include "mozilla/Array.h"
#include "mozilla/Move.h"
#include "mozilla/Mutex.h"
#include "mozilla/PodOperations.h"
#include "mozilla/Vector.h"

#include "nsString.h"
#include "prinrval.h"

namespace mozilla {
namespace Telemetry {

static const size_t kTimeHistogramBuckets = 8 * sizeof(PRIntervalTime);




class TimeHistogram : public mozilla::Array<uint32_t, kTimeHistogramBuckets>
{
public:
  TimeHistogram()
  {
    mozilla::PodArrayZero(*this);
  }
  void Add(PRIntervalTime aTime);
};



class HangHistogram : public TimeHistogram
{
public:
  typedef mozilla::Vector<const char*, 8> Stack;

private:
  static uint32_t GetHash(const Stack& aStack);

  Stack mStack;
  
  const uint32_t mHash;

public:
  explicit HangHistogram(Stack&& aStack)
    : mStack(mozilla::Move(aStack))
    , mHash(GetHash(mStack))
  {
  }
  HangHistogram(HangHistogram&& aOther)
    : TimeHistogram(mozilla::Move(aOther))
    , mStack(mozilla::Move(aOther.mStack))
    , mHash(mozilla::Move(aOther.mHash))
  {
  }
  bool operator==(const HangHistogram& aOther) const;
  bool operator!=(const HangHistogram& aOther) const
  {
    return !operator==(aOther);
  }
  const Stack& GetStack() const {
    return mStack;
  }
};





class ThreadHangStats
{
private:
  nsAutoCString mName;

public:
  TimeHistogram mActivity;
  mozilla::Vector<HangHistogram, 4> mHangs;

  explicit ThreadHangStats(const char* aName)
    : mName(aName)
  {
  }
  ThreadHangStats(ThreadHangStats&& aOther)
    : mName(mozilla::Move(aOther.mName))
    , mActivity(mozilla::Move(aOther.mActivity))
    , mHangs(mozilla::Move(aOther.mHangs))
  {
  }
  const char* GetName() const {
    return mName.get();
  }
};

} 
} 
#endif 
