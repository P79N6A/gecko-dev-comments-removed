























#ifndef mozilla_pkix_Time_h
#define mozilla_pkix_Time_h

#include <ctime>
#include <limits>
#include <stdint.h>

#include "pkix/Result.h"

namespace mozilla { namespace pkix {






class Time final
{
public:
  
  
  
  
  
  
  
  enum Uninitialized { uninitialized };
  explicit Time(Uninitialized) { }

  bool operator==(const Time& other) const
  {
    return elapsedSecondsAD == other.elapsedSecondsAD;
  }
  bool operator>(const Time& other) const
  {
    return elapsedSecondsAD > other.elapsedSecondsAD;
  }
  bool operator>=(const Time& other) const
  {
    return elapsedSecondsAD >= other.elapsedSecondsAD;
  }
  bool operator<(const Time& other) const
  {
    return elapsedSecondsAD < other.elapsedSecondsAD;
  }
  bool operator<=(const Time& other) const
  {
    return elapsedSecondsAD <= other.elapsedSecondsAD;
  }

  Result AddSeconds(uint64_t seconds)
  {
    if (std::numeric_limits<uint64_t>::max() - elapsedSecondsAD
          < seconds) {
      return Result::FATAL_ERROR_INVALID_ARGS; 
    }
    elapsedSecondsAD += seconds;
    return Success;
  }

  Result SubtractSeconds(uint64_t seconds)
  {
    if (seconds > elapsedSecondsAD) {
      return Result::FATAL_ERROR_INVALID_ARGS; 
    }
    elapsedSecondsAD -= seconds;
    return Success;
  }

  static const uint64_t ONE_DAY_IN_SECONDS
    = UINT64_C(24) * UINT64_C(60) * UINT64_C(60);

private:
  
  
  
  
  
  
  
  explicit Time(uint64_t elapsedSecondsAD)
    : elapsedSecondsAD(elapsedSecondsAD)
  {
  }
  friend Time TimeFromElapsedSecondsAD(uint64_t);

  uint64_t elapsedSecondsAD;
};

inline Time TimeFromElapsedSecondsAD(uint64_t elapsedSecondsAD)
{
  return Time(elapsedSecondsAD);
}

Time Now();


Time TimeFromEpochInSeconds(uint64_t secondsSinceEpoch);

} } 

#endif
