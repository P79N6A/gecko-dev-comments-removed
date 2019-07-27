





#ifndef mozilla_StickyTimeDuration_h
#define mozilla_StickyTimeDuration_h

#include "mozilla/TimeStamp.h"
#include "mozilla/FloatingPoint.h"

namespace mozilla {















class StickyTimeDurationValueCalculator
{
public:
  static int64_t
  Add(int64_t aA, int64_t aB)
  {
    MOZ_ASSERT((aA != INT64_MAX || aB != INT64_MIN) &&
               (aA != INT64_MIN || aB != INT64_MAX),
               "'Infinity + -Infinity' and '-Infinity + Infinity'"
               " are undefined");

    
    
    if (aA == INT64_MAX || aB == INT64_MAX) {
      return INT64_MAX;
    }
    
    
    if (aA == INT64_MIN || aB == INT64_MIN) {
      return INT64_MIN;
    }

    return aA + aB;
  }

  
  
  
  static int64_t
  Subtract(int64_t aA, int64_t aB)
  {
    MOZ_ASSERT((aA != INT64_MAX && aA != INT64_MIN) || aA != aB,
               "'Infinity - Infinity' and '-Infinity - -Infinity'"
               " are undefined");

    
    
    if (aA == INT64_MAX || aB == INT64_MIN) {
      return INT64_MAX;
    }
    
    
    if (aA == INT64_MIN || aB == INT64_MAX) {
      return INT64_MIN;
    }

    return aA - aB;
  }

  template <typename T>
  static int64_t
  Multiply(int64_t aA, T aB) {
    
    return Multiply(aA, static_cast<int64_t>(aB));
  }

  static int64_t
  Divide(int64_t aA, int64_t aB) {
    MOZ_ASSERT(aB != 0, "Division by zero");
    MOZ_ASSERT((aA != INT64_MAX && aA != INT64_MIN) ||
               (aB != INT64_MAX && aB != INT64_MIN),
               "Dividing +/-Infinity by +/-Infinity is undefined");

    
    
    
    
    if (aA == INT64_MAX || aA == INT64_MIN) {
      return (aA >= 0) ^ (aB >= 0) ? INT64_MIN : INT64_MAX;
    }
    
    
    if (aB == INT64_MAX || aB == INT64_MIN) {
      return 0;
    }

    return aA / aB;
  }

  static double
  DivideDouble(int64_t aA, int64_t aB)
  {
    MOZ_ASSERT(aB != 0, "Division by zero");
    MOZ_ASSERT((aA != INT64_MAX && aA != INT64_MIN) ||
               (aB != INT64_MAX && aB != INT64_MIN),
               "Dividing +/-Infinity by +/-Infinity is undefined");

    
    
    
    
    if (aA == INT64_MAX || aA == INT64_MIN) {
      return (aA >= 0) ^ (aB >= 0)
             ? NegativeInfinity<double>()
             : PositiveInfinity<double>();
    }
    
    
    if (aB == INT64_MAX || aB == INT64_MIN) {
      return 0.0;
    }

    return static_cast<double>(aA) / aB;
  }

  static int64_t
  Modulo(int64_t aA, int64_t aB)
  {
    MOZ_ASSERT(aA != INT64_MAX && aA != INT64_MIN,
               "Infinity modulo x is undefined");

    return aA % aB;
  }
};

template <>
inline int64_t
StickyTimeDurationValueCalculator::Multiply<int64_t>(int64_t aA,
                                                          int64_t aB)
{
  MOZ_ASSERT((aA != 0 || (aB != INT64_MIN && aB != INT64_MAX)) &&
             ((aA != INT64_MIN && aA != INT64_MAX) || aB != 0),
             "Multiplication of infinity by zero");

  
  
  
  
  
  
  
  if (aA == INT64_MAX || aA == INT64_MIN ||
      aB == INT64_MAX || aB == INT64_MIN) {
    return (aA >= 0) ^ (aB >= 0) ? INT64_MAX : INT64_MIN;
  }

  return aA * aB;
}

template <>
inline int64_t
StickyTimeDurationValueCalculator::Multiply<double>(int64_t aA, double aB)
{
  MOZ_ASSERT((aA != 0 || (!IsInfinite(aB))) &&
             ((aA != INT64_MIN && aA != INT64_MAX) || aB != 0.0),
             "Multiplication of infinity by zero");

  
  
  
  if (aA == INT64_MAX || aA == INT64_MIN || IsInfinite(aB)) {
    return (aA >= 0) ^ (aB >= 0.0) ? INT64_MAX : INT64_MIN;
  }

  return aA * aB;
}

template <>
inline int64_t
StickyTimeDurationValueCalculator::Multiply<float>(int64_t aA, float aB)
{
  MOZ_ASSERT(IsInfinite(aB) == IsInfinite(static_cast<double>(aB)),
             "Casting to float loses infinite-ness");

  return Multiply(aA, static_cast<double>(aB));
}










typedef BaseTimeDuration<StickyTimeDurationValueCalculator>
  StickyTimeDuration;



inline StickyTimeDuration
operator+(const TimeDuration& aA, const StickyTimeDuration& aB)
{
  return StickyTimeDuration(aA) + aB;
}
inline StickyTimeDuration
operator+(const StickyTimeDuration& aA, const TimeDuration& aB)
{
  return aA + StickyTimeDuration(aB);
}

inline StickyTimeDuration
operator-(const TimeDuration& aA, const StickyTimeDuration& aB)
{
  return StickyTimeDuration(aA) - aB;
}
inline StickyTimeDuration
operator-(const StickyTimeDuration& aA, const TimeDuration& aB)
{
  return aA - StickyTimeDuration(aB);
}

inline StickyTimeDuration&
operator+=(StickyTimeDuration &aA, const TimeDuration& aB)
{
  return aA += StickyTimeDuration(aB);
}
inline StickyTimeDuration&
operator-=(StickyTimeDuration &aA, const TimeDuration& aB)
{
  return aA -= StickyTimeDuration(aB);
}

inline double
operator/(const TimeDuration& aA, const StickyTimeDuration& aB)
{
  return StickyTimeDuration(aA) / aB;
}
inline double
operator/(const StickyTimeDuration& aA, const TimeDuration& aB)
{
  return aA / StickyTimeDuration(aB);
}

inline StickyTimeDuration
operator%(const TimeDuration& aA, const StickyTimeDuration& aB)
{
  return StickyTimeDuration(aA) % aB;
}
inline StickyTimeDuration
operator%(const StickyTimeDuration& aA, const TimeDuration& aB)
{
  return aA % StickyTimeDuration(aB);
}

} 

#endif 
