





#ifndef mozilla_TimeStamp_windows_h
#define mozilla_TimeStamp_windows_h

#include "mozilla/Types.h"

namespace mozilla {

class TimeStamp;

class TimeStampValue
{
  friend struct IPC::ParamTraits<mozilla::TimeStampValue>;
  friend class TimeStamp;
  friend void StartupTimelineRecordExternal(int, uint64_t);

  
  uint64_t mGTC;
  uint64_t mQPC;
  bool mHasQPC;
  bool mIsNull;

  MFBT_API TimeStampValue(uint64_t aGTC, uint64_t aQPC, bool aHasQPC);

  MFBT_API uint64_t CheckQPC(const TimeStampValue& aOther) const;

  struct _SomethingVeryRandomHere;
  MOZ_CONSTEXPR TimeStampValue(_SomethingVeryRandomHere* aNullValue)
    : mGTC(0)
    , mQPC(0)
    , mHasQPC(false)
    , mIsNull(true)
  {
  }

public:
  MFBT_API uint64_t operator-(const TimeStampValue& aOther) const;

  TimeStampValue operator+(const int64_t aOther) const
  {
    return TimeStampValue(mGTC + aOther, mQPC + aOther, mHasQPC);
  }
  TimeStampValue operator-(const int64_t aOther) const
  {
    return TimeStampValue(mGTC - aOther, mQPC - aOther, mHasQPC);
  }
  MFBT_API TimeStampValue& operator+=(const int64_t aOther);
  MFBT_API TimeStampValue& operator-=(const int64_t aOther);

  bool operator<(const TimeStampValue& aOther) const
  {
    return int64_t(*this - aOther) < 0;
  }
  bool operator>(const TimeStampValue& aOther) const
  {
    return int64_t(*this - aOther) > 0;
  }
  bool operator<=(const TimeStampValue& aOther) const
  {
    return int64_t(*this - aOther) <= 0;
  }
  bool operator>=(const TimeStampValue& aOther) const
  {
    return int64_t(*this - aOther) >= 0;
  }
  bool operator==(const TimeStampValue& aOther) const
  {
    return int64_t(*this - aOther) == 0;
  }
  bool operator!=(const TimeStampValue& aOther) const
  {
    return int64_t(*this - aOther) != 0;
  }
};

}

#endif 
