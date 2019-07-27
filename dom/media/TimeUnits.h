





#ifndef TIME_UNITS_H
#define TIME_UNITS_H

#include "VideoUtils.h"
#include "mozilla/FloatingPoint.h"

namespace mozilla {
namespace media {

struct Microseconds {
  Microseconds()
    : mValue(0)
  {}

  explicit Microseconds(int64_t aValue)
    : mValue(aValue)
  {}

  double ToSeconds() {
    return double(mValue) / USECS_PER_S;
  }

  static Microseconds FromSeconds(double aValue) {
    MOZ_ASSERT(!IsNaN(aValue));

    double val = aValue * USECS_PER_S;
    if (val >= double(INT64_MAX)) {
      return Microseconds(INT64_MAX);
    } else if (val <= double(INT64_MIN)) {
      return Microseconds(INT64_MIN);
    } else {
      return Microseconds(int64_t(val));
    }
  }

  bool operator > (const Microseconds& aOther) const {
    return mValue > aOther.mValue;
  }
  bool operator >= (const Microseconds& aOther) const {
    return mValue >= aOther.mValue;
  }
  bool operator < (const Microseconds& aOther) const {
    return mValue < aOther.mValue;
  }
  bool operator <= (const Microseconds& aOther) const {
    return mValue <= aOther.mValue;
  }

  int64_t mValue;
};


}
}

#endif 
