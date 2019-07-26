







#ifndef mozilla_Casting_h
#define mozilla_Casting_h

#include "mozilla/Assertions.h"
#include "mozilla/TypeTraits.h"

#include <limits.h>

namespace mozilla {








template<typename To, typename From>
inline To
BitwiseCast(const From from)
{
  static_assert(sizeof(From) == sizeof(To),
                "To and From must have the same size");
  union {
    From from;
    To to;
  } u;
  u.from = from;
  return u.to;
}

namespace detail {

enum ToSignedness { ToIsSigned, ToIsUnsigned };
enum FromSignedness { FromIsSigned, FromIsUnsigned };

template<typename From,
         typename To,
         FromSignedness = IsSigned<From>::value ? FromIsSigned : FromIsUnsigned,
         ToSignedness = IsSigned<To>::value ? ToIsSigned : ToIsUnsigned>
struct BoundsCheckImpl;






enum UUComparison { FromIsBigger, FromIsNotBigger };



template<typename From, typename To,
         UUComparison = (sizeof(From) > sizeof(To)) ? FromIsBigger : FromIsNotBigger>
struct UnsignedUnsignedCheck;

template<typename From, typename To>
struct UnsignedUnsignedCheck<From, To, FromIsBigger>
{
  public:
    static bool checkBounds(const From from) {
      return from <= From(To(-1));
    }
};

template<typename From, typename To>
struct UnsignedUnsignedCheck<From, To, FromIsNotBigger>
{
  public:
    static bool checkBounds(const From from) {
      return true;
    }
};

template<typename From, typename To>
struct BoundsCheckImpl<From, To, FromIsUnsigned, ToIsUnsigned>
{
  public:
    static bool checkBounds(const From from) {
      return UnsignedUnsignedCheck<From, To>::checkBounds(from);
    }
};



template<typename From, typename To>
struct BoundsCheckImpl<From, To, FromIsSigned, ToIsUnsigned>
{
  public:
    static bool checkBounds(const From from) {
      if (from < 0)
        return false;
      if (sizeof(To) >= sizeof(From))
        return true;
      return from <= From(To(-1));
    }
};



enum USComparison { FromIsSmaller, FromIsNotSmaller };

template<typename From, typename To,
         USComparison = (sizeof(From) < sizeof(To)) ? FromIsSmaller : FromIsNotSmaller>
struct UnsignedSignedCheck;

template<typename From, typename To>
struct UnsignedSignedCheck<From, To, FromIsSmaller>
{
  public:
    static bool checkBounds(const From from) {
      return true;
    }
};

template<typename From, typename To>
struct UnsignedSignedCheck<From, To, FromIsNotSmaller>
{
  public:
    static bool checkBounds(const From from) {
      const To MaxValue = To((1ULL << (CHAR_BIT * sizeof(To) - 1)) - 1);
      return from <= From(MaxValue);
    }
};

template<typename From, typename To>
struct BoundsCheckImpl<From, To, FromIsUnsigned, ToIsSigned>
{
  public:
    static bool checkBounds(const From from) {
      return UnsignedSignedCheck<From, To>::checkBounds(from);
    }
};



template<typename From, typename To>
struct BoundsCheckImpl<From, To, FromIsSigned, ToIsSigned>
{
  public:
    static bool checkBounds(const From from) {
      if (sizeof(From) <= sizeof(To))
        return true;
      const To MaxValue = To((1ULL << (CHAR_BIT * sizeof(To) - 1)) - 1);
      const To MinValue = -MaxValue - To(1);
      return From(MinValue) <= from &&
             From(from) <= From(MaxValue);
    }
};

template<typename From, typename To,
         bool TypesAreIntegral = IsIntegral<From>::value && IsIntegral<To>::value>
class BoundsChecker;

template<typename From>
class BoundsChecker<From, From, true>
{
  public:
    static bool checkBounds(const From from) { return true; }
};

template<typename From, typename To>
class BoundsChecker<From, To, true>
{
  public:
    static bool checkBounds(const From from) {
      return BoundsCheckImpl<From, To>::checkBounds(from);
    }
};

template<typename From, typename To>
inline bool
IsInBounds(const From from)
{
  return BoundsChecker<From, To>::checkBounds(from);
}

} 






template<typename To, typename From>
inline To
SafeCast(const From from)
{
  MOZ_ASSERT((detail::IsInBounds<From, To>(from)));
  return static_cast<To>(from);
}

} 

#endif 
