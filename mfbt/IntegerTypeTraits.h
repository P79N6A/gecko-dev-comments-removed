







#ifndef mozilla_IntegerTypeTraits_h
#define mozilla_IntegerTypeTraits_h

#include "mozilla/TypeTraits.h"
#include <stdint.h>

namespace mozilla {

namespace detail {






template<size_t Size, bool Signedness>
struct StdintTypeForSizeAndSignedness;

template<>
struct StdintTypeForSizeAndSignedness<1, true>
{
  typedef int8_t Type;
};

template<>
struct StdintTypeForSizeAndSignedness<1, false>
{
  typedef uint8_t Type;
};

template<>
struct StdintTypeForSizeAndSignedness<2, true>
{
  typedef int16_t Type;
};

template<>
struct StdintTypeForSizeAndSignedness<2, false>
{
  typedef uint16_t Type;
};

template<>
struct StdintTypeForSizeAndSignedness<4, true>
{
  typedef int32_t Type;
};

template<>
struct StdintTypeForSizeAndSignedness<4, false>
{
  typedef uint32_t Type;
};

template<>
struct StdintTypeForSizeAndSignedness<8, true>
{
  typedef int64_t Type;
};

template<>
struct StdintTypeForSizeAndSignedness<8, false>
{
  typedef uint64_t Type;
};

} 

template<size_t Size>
struct UnsignedStdintTypeForSize
  : detail::StdintTypeForSizeAndSignedness<Size, false>
{};

template<typename IntegerType>
struct PositionOfSignBit
{
  static_assert(IsIntegral<IntegerType>::value,
                "PositionOfSignBit is only for integral types");
  
  static const size_t value = 8 * sizeof(IntegerType) - 1;
};






template<typename IntegerType>
struct MinValue
{
private:
  static_assert(IsIntegral<IntegerType>::value,
                "MinValue is only for integral types");

  typedef typename MakeUnsigned<IntegerType>::Type UnsignedIntegerType;
  static const size_t PosOfSignBit = PositionOfSignBit<IntegerType>::value;

public:
  
  
  
  
  
  
  static const IntegerType value =
      IsSigned<IntegerType>::value
      ? IntegerType(UnsignedIntegerType(1) << PosOfSignBit)
      : IntegerType(0);
};






template<typename IntegerType>
struct MaxValue
{
  static_assert(IsIntegral<IntegerType>::value,
                "MaxValue is only for integral types");

  
  
  
  static const IntegerType value = ~MinValue<IntegerType>::value;
};

} 

#endif 
