






#ifndef mozilla_CheckedInt_h_
#define mozilla_CheckedInt_h_









#define MOZ_CHECKEDINT_ENABLE_MOZ_ASSERTS






#ifdef MOZ_CHECKEDINT_ENABLE_MOZ_ASSERTS
#  include "mozilla/Assertions.h"
#else
#  ifndef MOZ_STATIC_ASSERT
#    include <cassert>
#    define MOZ_STATIC_ASSERT(cond, reason) assert((cond) && reason)
#    define MOZ_ASSERT(cond, reason) assert((cond) && reason)
#  endif
#endif

#include "mozilla/StandardInteger.h"

#include <climits>
#include <cstddef>

namespace mozilla {

namespace detail {












struct UnsupportedType {};

template<typename IntegerType>
struct IsSupportedPass2
{
    static const bool value = false;
};

template<typename IntegerType>
struct IsSupported
{
    static const bool value = IsSupportedPass2<IntegerType>::value;
};

template<>
struct IsSupported<int8_t>
{ static const bool value = true; };

template<>
struct IsSupported<uint8_t>
{ static const bool value = true; };

template<>
struct IsSupported<int16_t>
{ static const bool value = true; };

template<>
struct IsSupported<uint16_t>
{ static const bool value = true; };

template<>
struct IsSupported<int32_t>
{ static const bool value = true; };

template<>
struct IsSupported<uint32_t>
{ static const bool value = true; };

template<>
struct IsSupported<int64_t>
{ static const bool value = true; };

template<>
struct IsSupported<uint64_t>
{ static const bool value = true; };


template<>
struct IsSupportedPass2<char>
{ static const bool value = true; };

template<>
struct IsSupportedPass2<signed char>
{ static const bool value = true; };

template<>
struct IsSupportedPass2<unsigned char>
{ static const bool value = true; };

template<>
struct IsSupportedPass2<short>
{ static const bool value = true; };

template<>
struct IsSupportedPass2<unsigned short>
{ static const bool value = true; };

template<>
struct IsSupportedPass2<int>
{ static const bool value = true; };

template<>
struct IsSupportedPass2<unsigned int>
{ static const bool value = true; };

template<>
struct IsSupportedPass2<long>
{ static const bool value = true; };

template<>
struct IsSupportedPass2<unsigned long>
{ static const bool value = true; };






template<size_t Size, bool Signedness>
struct StdintTypeForSizeAndSignedness
{};

template<>
struct StdintTypeForSizeAndSignedness<1, true>
{ typedef int8_t   Type; };

template<>
struct StdintTypeForSizeAndSignedness<1, false>
{ typedef uint8_t  Type; };

template<>
struct StdintTypeForSizeAndSignedness<2, true>
{ typedef int16_t  Type; };

template<>
struct StdintTypeForSizeAndSignedness<2, false>
{ typedef uint16_t Type; };

template<>
struct StdintTypeForSizeAndSignedness<4, true>
{ typedef int32_t  Type; };

template<>
struct StdintTypeForSizeAndSignedness<4, false>
{ typedef uint32_t Type; };

template<>
struct StdintTypeForSizeAndSignedness<8, true>
{ typedef int64_t  Type; };

template<>
struct StdintTypeForSizeAndSignedness<8, false>
{ typedef uint64_t Type; };

template<typename IntegerType>
struct UnsignedType
{
    typedef typename StdintTypeForSizeAndSignedness<sizeof(IntegerType),
                                                    false>::Type Type;
};

template<typename IntegerType>
struct IsSigned
{
    static const bool value = IntegerType(-1) <= IntegerType(0);
};

template<typename IntegerType, size_t Size = sizeof(IntegerType)>
struct TwiceBiggerType
{
    typedef typename StdintTypeForSizeAndSignedness<
                       sizeof(IntegerType) * 2,
                       IsSigned<IntegerType>::value
                     >::Type Type;
};

template<typename IntegerType>
struct TwiceBiggerType<IntegerType, 8>
{
    typedef UnsupportedType Type;
};

template<typename IntegerType>
struct PositionOfSignBit
{
    static const size_t value = CHAR_BIT * sizeof(IntegerType) - 1;
};

template<typename IntegerType>
struct MinValue
{
  private:
    typedef typename UnsignedType<IntegerType>::Type UnsignedIntegerType;
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
    
    
    
    static const IntegerType value = ~MinValue<IntegerType>::value;
};







template<typename T>
inline bool
HasSignBit(T x)
{
  
  
  
  
  return bool(typename UnsignedType<T>::Type(x)
                >> PositionOfSignBit<T>::value);
}



template<typename T>
inline T
BinaryComplement(T x)
{
  return ~x;
}

template<typename T,
         typename U,
         bool IsTSigned = IsSigned<T>::value,
         bool IsUSigned = IsSigned<U>::value>
struct DoesRangeContainRange
{
};

template<typename T, typename U, bool Signedness>
struct DoesRangeContainRange<T, U, Signedness, Signedness>
{
    static const bool value = sizeof(T) >= sizeof(U);
};

template<typename T, typename U>
struct DoesRangeContainRange<T, U, true, false>
{
    static const bool value = sizeof(T) > sizeof(U);
};

template<typename T, typename U>
struct DoesRangeContainRange<T, U, false, true>
{
    static const bool value = false;
};

template<typename T,
         typename U,
         bool IsTSigned = IsSigned<T>::value,
         bool IsUSigned = IsSigned<U>::value,
         bool DoesTRangeContainURange = DoesRangeContainRange<T, U>::value>
struct IsInRangeImpl {};

template<typename T, typename U, bool IsTSigned, bool IsUSigned>
struct IsInRangeImpl<T, U, IsTSigned, IsUSigned, true>
{
    static bool run(U)
    {
       return true;
    }
};

template<typename T, typename U>
struct IsInRangeImpl<T, U, true, true, false>
{
    static bool run(U x)
    {
      return x <= MaxValue<T>::value && x >= MinValue<T>::value;
    }
};

template<typename T, typename U>
struct IsInRangeImpl<T, U, false, false, false>
{
    static bool run(U x)
    {
      return x <= MaxValue<T>::value;
    }
};

template<typename T, typename U>
struct IsInRangeImpl<T, U, true, false, false>
{
    static bool run(U x)
    {
      return sizeof(T) > sizeof(U) || x <= U(MaxValue<T>::value);
    }
};

template<typename T, typename U>
struct IsInRangeImpl<T, U, false, true, false>
{
    static bool run(U x)
    {
      return sizeof(T) >= sizeof(U)
             ? x >= 0
             : x >= 0 && x <= U(MaxValue<T>::value);
    }
};

template<typename T, typename U>
inline bool
IsInRange(U x)
{
  return IsInRangeImpl<T, U>::run(x);
}

template<typename T>
inline bool
IsAddValid(T x, T y)
{
  
  
  
  
  

  typename UnsignedType<T>::Type ux = x;
  typename UnsignedType<T>::Type uy = y;
  typename UnsignedType<T>::Type result = ux + uy;
  return IsSigned<T>::value
         ? HasSignBit(BinaryComplement(T((result ^ x) & (result ^ y))))
         : BinaryComplement(x) >= y;
}

template<typename T>
inline bool
IsSubValid(T x, T y)
{
  
  
  
  typename UnsignedType<T>::Type ux = x;
  typename UnsignedType<T>::Type uy = y;
  typename UnsignedType<T>::Type result = ux - uy;

  return IsSigned<T>::value
         ? HasSignBit(BinaryComplement(T((result ^ x) & (x ^ y))))
         : x >= y;
}

template<typename T,
         bool IsTSigned = IsSigned<T>::value,
         bool TwiceBiggerTypeIsSupported =
           IsSupported<typename TwiceBiggerType<T>::Type>::value>
struct IsMulValidImpl {};

template<typename T, bool IsTSigned>
struct IsMulValidImpl<T, IsTSigned, true>
{
    static bool run(T x, T y)
    {
      typedef typename TwiceBiggerType<T>::Type TwiceBiggerType;
      TwiceBiggerType product = TwiceBiggerType(x) * TwiceBiggerType(y);
      return IsInRange<T>(product);
    }
};

template<typename T>
struct IsMulValidImpl<T, true, false>
{
    static bool run(T x, T y)
    {
      const T max = MaxValue<T>::value;
      const T min = MinValue<T>::value;

      if (x == 0 || y == 0)
        return true;

      if (x > 0) {
        return y > 0
               ? x <= max / y
               : y >= min / x;
      }

      
      return y > 0
             ? x >= min / y
             : y >= max / x;
    }
};

template<typename T>
struct IsMulValidImpl<T, false, false>
{
    static bool run(T x, T y)
    {
      return y == 0 ||  x <= MaxValue<T>::value / y;
    }
};

template<typename T>
inline bool
IsMulValid(T x, T y)
{
  return IsMulValidImpl<T>::run(x, y);
}

template<typename T>
inline bool
IsDivValid(T x, T y)
{
  
  return y != 0 &&
         !(IsSigned<T>::value && x == MinValue<T>::value && y == T(-1));
}


template<typename T, bool IsTSigned = IsSigned<T>::value>
struct OppositeIfSignedImpl
{
    static T run(T x) { return -x; }
};
template<typename T>
struct OppositeIfSignedImpl<T, false>
{
    static T run(T x) { return x; }
};
template<typename T>
inline T
OppositeIfSigned(T x)
{
  return OppositeIfSignedImpl<T>::run(x);
}

} 









































































template<typename T>
class CheckedInt
{
  protected:
    T mValue;
    bool mIsValid;

    template<typename U>
    CheckedInt(U value, bool isValid) : mValue(value), mIsValid(isValid)
    {
      MOZ_STATIC_ASSERT(detail::IsSupported<T>::value,
                        "This type is not supported by CheckedInt");
    }

  public:
    










    template<typename U>
    CheckedInt(U value)
      : mValue(T(value)),
        mIsValid(detail::IsInRange<T>(value))
    {
      MOZ_STATIC_ASSERT(detail::IsSupported<T>::value,
                        "This type is not supported by CheckedInt");
    }

    
    CheckedInt() : mValue(0), mIsValid(true)
    {
      MOZ_STATIC_ASSERT(detail::IsSupported<T>::value,
                        "This type is not supported by CheckedInt");
    }

    
    T value() const
    {
      MOZ_ASSERT(mIsValid, "Invalid checked integer (division by zero or integer overflow)");
      return mValue;
    }

    




    bool isValid() const
    {
      return mIsValid;
    }

    template<typename U>
    friend CheckedInt<U> operator +(const CheckedInt<U>& lhs,
                                    const CheckedInt<U>& rhs);
    template<typename U>
    CheckedInt& operator +=(U rhs);
    template<typename U>
    friend CheckedInt<U> operator -(const CheckedInt<U>& lhs,
                                    const CheckedInt<U> &rhs);
    template<typename U>
    CheckedInt& operator -=(U rhs);
    template<typename U>
    friend CheckedInt<U> operator *(const CheckedInt<U>& lhs,
                                    const CheckedInt<U> &rhs);
    template<typename U>
    CheckedInt& operator *=(U rhs);
    template<typename U>
    friend CheckedInt<U> operator /(const CheckedInt<U>& lhs,
                                    const CheckedInt<U> &rhs);
    template<typename U>
    CheckedInt& operator /=(U rhs);

    CheckedInt operator -() const
    {
      
      
      
      T result = detail::OppositeIfSigned(mValue);
      
      return CheckedInt(result,
                        mIsValid && detail::IsSubValid(T(0),
                                                       mValue));
    }

    

















    bool operator ==(const CheckedInt& other) const
    {
      return mIsValid && other.mIsValid && mValue == other.mValue;
    }

    
    CheckedInt& operator++()
    {
      *this += 1;
      return *this;
    }

    
    CheckedInt operator++(int)
    {
      CheckedInt tmp = *this;
      *this += 1;
      return tmp;
    }

    
    CheckedInt& operator--()
    {
      *this -= 1;
      return *this;
    }

    
    CheckedInt operator--(int)
    {
      CheckedInt tmp = *this;
      *this -= 1;
      return tmp;
    }

  private:
    



    template<typename U>
    bool operator !=(U other) const MOZ_DELETE;
    template<typename U>
    bool operator <(U other) const MOZ_DELETE;
    template<typename U>
    bool operator <=(U other) const MOZ_DELETE;
    template<typename U>
    bool operator >(U other) const MOZ_DELETE;
    template<typename U>
    bool operator >=(U other) const MOZ_DELETE;
};

#define MOZ_CHECKEDINT_BASIC_BINARY_OPERATOR(NAME, OP)                \
template<typename T>                                                  \
inline CheckedInt<T> operator OP(const CheckedInt<T> &lhs,            \
                                 const CheckedInt<T> &rhs)            \
{                                                                     \
  if (!detail::Is##NAME##Valid(lhs.mValue, rhs.mValue))               \
    return CheckedInt<T>(0, false);                                   \
                                                                      \
  return CheckedInt<T>(lhs.mValue OP rhs.mValue,                      \
                       lhs.mIsValid && rhs.mIsValid);                 \
}

MOZ_CHECKEDINT_BASIC_BINARY_OPERATOR(Add, +)
MOZ_CHECKEDINT_BASIC_BINARY_OPERATOR(Sub, -)
MOZ_CHECKEDINT_BASIC_BINARY_OPERATOR(Mul, *)
MOZ_CHECKEDINT_BASIC_BINARY_OPERATOR(Div, /)

#undef MOZ_CHECKEDINT_BASIC_BINARY_OPERATOR







namespace detail {

template<typename T, typename U>
struct CastToCheckedIntImpl
{
    typedef CheckedInt<T> ReturnType;
    static CheckedInt<T> run(U u) { return u; }
};

template<typename T>
struct CastToCheckedIntImpl<T, CheckedInt<T> >
{
    typedef const CheckedInt<T>& ReturnType;
    static const CheckedInt<T>& run(const CheckedInt<T>& u) { return u; }
};

} 

template<typename T, typename U>
inline typename detail::CastToCheckedIntImpl<T, U>::ReturnType
castToCheckedInt(U u)
{
  return detail::CastToCheckedIntImpl<T, U>::run(u);
}

#define MOZ_CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(OP, COMPOUND_OP)  \
template<typename T>                                              \
template<typename U>                                              \
CheckedInt<T>& CheckedInt<T>::operator COMPOUND_OP(U rhs)         \
{                                                                 \
  *this = *this OP castToCheckedInt<T>(rhs);                      \
  return *this;                                                   \
}                                                                 \
template<typename T, typename U>                                  \
inline CheckedInt<T> operator OP(const CheckedInt<T> &lhs, U rhs) \
{                                                                 \
  return lhs OP castToCheckedInt<T>(rhs);                         \
}                                                                 \
template<typename T, typename U>                                  \
inline CheckedInt<T> operator OP(U lhs, const CheckedInt<T> &rhs) \
{                                                                 \
  return castToCheckedInt<T>(lhs) OP rhs;                         \
}

MOZ_CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(+, +=)
MOZ_CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(*, *=)
MOZ_CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(-, -=)
MOZ_CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(/, /=)

#undef MOZ_CHECKEDINT_CONVENIENCE_BINARY_OPERATORS

template<typename T, typename U>
inline bool
operator ==(const CheckedInt<T> &lhs, U rhs)
{
  return lhs == castToCheckedInt<T>(rhs);
}

template<typename T, typename U>
inline bool
operator ==(U  lhs, const CheckedInt<T> &rhs)
{
  return castToCheckedInt<T>(lhs) == rhs;
}


typedef CheckedInt<int8_t>   CheckedInt8;
typedef CheckedInt<uint8_t>  CheckedUint8;
typedef CheckedInt<int16_t>  CheckedInt16;
typedef CheckedInt<uint16_t> CheckedUint16;
typedef CheckedInt<int32_t>  CheckedInt32;
typedef CheckedInt<uint32_t> CheckedUint32;
typedef CheckedInt<int64_t>  CheckedInt64;
typedef CheckedInt<uint64_t> CheckedUint64;

} 

#endif 
