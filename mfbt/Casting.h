






#ifndef mozilla_Casting_h_
#define mozilla_Casting_h_

#include "mozilla/Assertions.h"
#include "mozilla/TypeTraits.h"

#include <limits.h>

namespace mozilla {

namespace detail {

enum ToSignedness { ToIsSigned, ToIsUnsigned };
enum FromSignedness { FromIsSigned, FromIsUnsigned };

template<typename From,
         typename To,
         FromSignedness = IsSigned<From>::value ? FromIsSigned : FromIsUnsigned,
         ToSignedness = IsSigned<To>::value ? ToIsSigned : ToIsUnsigned>
struct BoundsCheckImpl;






template<typename From, typename To>
struct BoundsCheckImpl<From, To, FromIsUnsigned, ToIsUnsigned>
{
  public:
    static bool check(const From from) {
      typedef typename Conditional<sizeof(From) >= sizeof(To), From, To>::Type
              LargerType;
      return LargerType(from) <= LargerType(To(-1));
    }
};

template<typename From, typename To>
struct BoundsCheckImpl<From, To, FromIsSigned, ToIsUnsigned>
{
  public:
    static bool check(const From from) {
      if (from < 0)
        return false;
      if (sizeof(To) >= sizeof(From))
        return To(from) <= To(-1);
      return from <= From(To(-1));
    }
};

template<typename From, typename To>
struct BoundsCheckImpl<From, To, FromIsUnsigned, ToIsSigned>
{
  public:
    static bool check(const From from) {
      if (sizeof(From) < sizeof(To))
        return true;
      const To MaxValue = To((1ULL << (CHAR_BIT * sizeof(To) - 1)) - 1);
      return from <= From(MaxValue);
    }
};

template<typename From, typename To>
struct BoundsCheckImpl<From, To, FromIsSigned, ToIsSigned>
{
  public:
    static bool check(const From from) {
      typedef typename Conditional<sizeof(To) >= sizeof(From), To, From>::Type
              LargerType;
      const To MaxValue = To((1ULL << (CHAR_BIT * sizeof(To) - 1)) - 1);
      const To MinValue = -MaxValue - To(1);
      return LargerType(MinValue) <= LargerType(from) &&
             LargerType(from) <= LargerType(MaxValue);
    }
};

template<typename From, typename To,
         bool TypesAreIntegral = IsIntegral<From>::value && IsIntegral<To>::value>
class BoundsChecker;

template<typename From>
class BoundsChecker<From, From, true>
{
  public:
    static bool check(const From from) { return true; }
};

template<typename From, typename To>
class BoundsChecker<From, To, true>
{
  public:
    static bool check(const From from) {
      return BoundsCheckImpl<From, To>::check(from);
    }
};

template<typename From, typename To>
inline bool
IsInBounds(const From from)
{
  return BoundsChecker<From, To>::check(from);
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
