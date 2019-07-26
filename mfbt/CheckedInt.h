







#ifndef mozilla_CheckedInt_h
#define mozilla_CheckedInt_h

#include <stdint.h>
#include "mozilla/Assertions.h"
#include "mozilla/IntegerTypeTraits.h"

namespace mozilla {

template<typename T> class CheckedInt;

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

template<>
struct IsSupportedPass2<long long>
{ static const bool value = true; };

template<>
struct IsSupportedPass2<unsigned long long>
{ static const bool value = true; };







template<typename IntegerType, size_t Size = sizeof(IntegerType)>
struct TwiceBiggerType
{
  typedef typename detail::StdintTypeForSizeAndSignedness<
                     sizeof(IntegerType) * 2,
                     IsSigned<IntegerType>::value
                   >::Type Type;
};

template<typename IntegerType>
struct TwiceBiggerType<IntegerType, 8>
{
  typedef UnsupportedType Type;
};

template<typename T>
inline bool
HasSignBit(T aX)
{
  
  
  
  
  return bool(typename MakeUnsigned<T>::Type(aX) >>
              PositionOfSignBit<T>::value);
}



template<typename T>
inline T
BinaryComplement(T aX)
{
  return ~aX;
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
  static bool run(U aX)
  {
    return aX <= MaxValue<T>::value && aX >= MinValue<T>::value;
  }
};

template<typename T, typename U>
struct IsInRangeImpl<T, U, false, false, false>
{
  static bool run(U aX)
  {
    return aX <= MaxValue<T>::value;
  }
};

template<typename T, typename U>
struct IsInRangeImpl<T, U, true, false, false>
{
  static bool run(U aX)
  {
    return sizeof(T) > sizeof(U) || aX <= U(MaxValue<T>::value);
  }
};

template<typename T, typename U>
struct IsInRangeImpl<T, U, false, true, false>
{
  static bool run(U aX)
  {
    return sizeof(T) >= sizeof(U)
           ? aX >= 0
           : aX >= 0 && aX <= U(MaxValue<T>::value);
  }
};

template<typename T, typename U>
inline bool
IsInRange(U aX)
{
  return IsInRangeImpl<T, U>::run(aX);
}

template<typename T>
inline bool
IsAddValid(T aX, T aY)
{
  
  
  
  
  

  typename MakeUnsigned<T>::Type ux = aX;
  typename MakeUnsigned<T>::Type uy = aY;
  typename MakeUnsigned<T>::Type result = ux + uy;
  return IsSigned<T>::value
         ? HasSignBit(BinaryComplement(T((result ^ aX) & (result ^ aY))))
         : BinaryComplement(aX) >= aY;
}

template<typename T>
inline bool
IsSubValid(T aX, T aY)
{
  
  
  
  typename MakeUnsigned<T>::Type ux = aX;
  typename MakeUnsigned<T>::Type uy = aY;
  typename MakeUnsigned<T>::Type result = ux - uy;

  return IsSigned<T>::value
         ? HasSignBit(BinaryComplement(T((result ^ aX) & (aX ^ aY))))
         : aX >= aY;
}

template<typename T,
         bool IsTSigned = IsSigned<T>::value,
         bool TwiceBiggerTypeIsSupported =
           IsSupported<typename TwiceBiggerType<T>::Type>::value>
struct IsMulValidImpl {};

template<typename T, bool IsTSigned>
struct IsMulValidImpl<T, IsTSigned, true>
{
  static bool run(T aX, T aY)
  {
    typedef typename TwiceBiggerType<T>::Type TwiceBiggerType;
    TwiceBiggerType product = TwiceBiggerType(aX) * TwiceBiggerType(aY);
    return IsInRange<T>(product);
  }
};

template<typename T>
struct IsMulValidImpl<T, true, false>
{
  static bool run(T aX, T aY)
  {
    const T max = MaxValue<T>::value;
    const T min = MinValue<T>::value;

    if (aX == 0 || aY == 0) {
      return true;
    }
    if (aX > 0) {
      return aY > 0
             ? aX <= max / aY
             : aY >= min / aX;
    }

    
    return aY > 0
           ? aX >= min / aY
           : aY >= max / aX;
  }
};

template<typename T>
struct IsMulValidImpl<T, false, false>
{
  static bool run(T aX, T aY)
  {
    return aY == 0 ||  aX <= MaxValue<T>::value / aY;
  }
};

template<typename T>
inline bool
IsMulValid(T aX, T aY)
{
  return IsMulValidImpl<T>::run(aX, aY);
}

template<typename T>
inline bool
IsDivValid(T aX, T aY)
{
  
  
  return aY != 0 &&
         !(IsSigned<T>::value && aX == MinValue<T>::value && aY == T(-1));
}

template<typename T, bool IsTSigned = IsSigned<T>::value>
struct IsModValidImpl;

template<typename T>
inline bool
IsModValid(T aX, T aY)
{
  return IsModValidImpl<T>::run(aX, aY);
}












template<typename T>
struct IsModValidImpl<T, false>
{
  static inline bool run(T aX, T aY)
  {
    return aY >= 1;
  }
};

template<typename T>
struct IsModValidImpl<T, true>
{
  static inline bool run(T aX, T aY)
  {
    if (aX < 0) {
      return false;
    }
    return aY >= 1;
  }
};

template<typename T, bool IsSigned = IsSigned<T>::value>
struct NegateImpl;

template<typename T>
struct NegateImpl<T, false>
{
  static CheckedInt<T> negate(const CheckedInt<T>& aVal)
  {
    
    
    return CheckedInt<T>(0, aVal.isValid() && aVal.mValue == 0);
  }
};

template<typename T>
struct NegateImpl<T, true>
{
  static CheckedInt<T> negate(const CheckedInt<T>& aVal)
  {
    
    
    if (!aVal.isValid() || aVal.mValue == MinValue<T>::value) {
      return CheckedInt<T>(aVal.mValue, false);
    }
    return CheckedInt<T>(-aVal.mValue, true);
  }
};

} 









































































template<typename T>
class CheckedInt
{
protected:
  T mValue;
  bool mIsValid;

  template<typename U>
  CheckedInt(U aValue, bool aIsValid) : mValue(aValue), mIsValid(aIsValid)
  {
    static_assert(detail::IsSupported<T>::value &&
                  detail::IsSupported<U>::value,
                  "This type is not supported by CheckedInt");
  }

  friend struct detail::NegateImpl<T>;

public:
  










  template<typename U>
  CheckedInt(U aValue)
    : mValue(T(aValue)),
      mIsValid(detail::IsInRange<T>(aValue))
  {
    static_assert(detail::IsSupported<T>::value &&
                  detail::IsSupported<U>::value,
                  "This type is not supported by CheckedInt");
  }

  template<typename U>
  friend class CheckedInt;

  template<typename U>
  CheckedInt<U> toChecked() const
  {
    CheckedInt<U> ret(mValue);
    ret.mIsValid = ret.mIsValid && mIsValid;
    return ret;
  }

  
  CheckedInt() : mValue(0), mIsValid(true)
  {
    static_assert(detail::IsSupported<T>::value,
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
  friend CheckedInt<U> operator +(const CheckedInt<U>& aLhs,
                                  const CheckedInt<U>& aRhs);
  template<typename U>
  CheckedInt& operator +=(U aRhs);

  template<typename U>
  friend CheckedInt<U> operator -(const CheckedInt<U>& aLhs,
                                  const CheckedInt<U>& aRhs);
  template<typename U>
  CheckedInt& operator -=(U aRhs);

  template<typename U>
  friend CheckedInt<U> operator *(const CheckedInt<U>& aLhs,
                                  const CheckedInt<U>& aRhs);
  template<typename U>
  CheckedInt& operator *=(U aRhs);

  template<typename U>
  friend CheckedInt<U> operator /(const CheckedInt<U>& aLhs,
                                  const CheckedInt<U>& aRhs);
  template<typename U>
  CheckedInt& operator /=(U aRhs);

  template<typename U>
  friend CheckedInt<U> operator %(const CheckedInt<U>& aLhs,
                                  const CheckedInt<U>& aRhs);
  template<typename U>
  CheckedInt& operator %=(U aRhs);

  CheckedInt operator -() const
  {
    return detail::NegateImpl<T>::negate(*this);
  }

  

















  bool operator ==(const CheckedInt& aOther) const
  {
    return mIsValid && aOther.mIsValid && mValue == aOther.mValue;
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
  



  template<typename U> bool operator !=(U aOther) const MOZ_DELETE;
  template<typename U> bool operator < (U aOther) const MOZ_DELETE;
  template<typename U> bool operator <=(U aOther) const MOZ_DELETE;
  template<typename U> bool operator > (U aOther) const MOZ_DELETE;
  template<typename U> bool operator >=(U aOther) const MOZ_DELETE;
};

#define MOZ_CHECKEDINT_BASIC_BINARY_OPERATOR(NAME, OP)                        \
  template<typename T>                                                        \
  inline CheckedInt<T>                                                        \
  operator OP(const CheckedInt<T> &aLhs, const CheckedInt<T> &aRhs)           \
  {                                                                           \
    if (!detail::Is##NAME##Valid(aLhs.mValue, aRhs.mValue)) {                 \
      return CheckedInt<T>(0, false);                                         \
    }                                                                         \
    return CheckedInt<T>(aLhs.mValue OP aRhs.mValue,                          \
                         aLhs.mIsValid && aRhs.mIsValid);                     \
  }

MOZ_CHECKEDINT_BASIC_BINARY_OPERATOR(Add, +)
MOZ_CHECKEDINT_BASIC_BINARY_OPERATOR(Sub, -)
MOZ_CHECKEDINT_BASIC_BINARY_OPERATOR(Mul, *)
MOZ_CHECKEDINT_BASIC_BINARY_OPERATOR(Div, /)
MOZ_CHECKEDINT_BASIC_BINARY_OPERATOR(Mod, %)

#undef MOZ_CHECKEDINT_BASIC_BINARY_OPERATOR







namespace detail {

template<typename T, typename U>
struct CastToCheckedIntImpl
{
  typedef CheckedInt<T> ReturnType;
  static CheckedInt<T> run(U aU) { return aU; }
};

template<typename T>
struct CastToCheckedIntImpl<T, CheckedInt<T> >
{
  typedef const CheckedInt<T>& ReturnType;
  static const CheckedInt<T>& run(const CheckedInt<T>& aU) { return aU; }
};

} 

template<typename T, typename U>
inline typename detail::CastToCheckedIntImpl<T, U>::ReturnType
castToCheckedInt(U aU)
{
  static_assert(detail::IsSupported<T>::value &&
                detail::IsSupported<U>::value,
                "This type is not supported by CheckedInt");
  return detail::CastToCheckedIntImpl<T, U>::run(aU);
}

#define MOZ_CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(OP, COMPOUND_OP)          \
  template<typename T>                                                        \
  template<typename U>                                                        \
  CheckedInt<T>& CheckedInt<T>::operator COMPOUND_OP(U aRhs)                  \
  {                                                                           \
    *this = *this OP castToCheckedInt<T>(aRhs);                               \
    return *this;                                                             \
  }                                                                           \
  template<typename T, typename U>                                            \
  inline CheckedInt<T> operator OP(const CheckedInt<T> &aLhs, U aRhs)         \
  {                                                                           \
    return aLhs OP castToCheckedInt<T>(aRhs);                                 \
  }                                                                           \
  template<typename T, typename U>                                            \
  inline CheckedInt<T> operator OP(U aLhs, const CheckedInt<T> &aRhs)         \
  {                                                                           \
    return castToCheckedInt<T>(aLhs) OP aRhs;                                 \
  }

MOZ_CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(+, +=)
MOZ_CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(*, *=)
MOZ_CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(-, -=)
MOZ_CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(/, /=)
MOZ_CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(%, %=)

#undef MOZ_CHECKEDINT_CONVENIENCE_BINARY_OPERATORS

template<typename T, typename U>
inline bool
operator ==(const CheckedInt<T> &aLhs, U aRhs)
{
  return aLhs == castToCheckedInt<T>(aRhs);
}

template<typename T, typename U>
inline bool
operator ==(U aLhs, const CheckedInt<T> &aRhs)
{
  return castToCheckedInt<T>(aLhs) == aRhs;
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
