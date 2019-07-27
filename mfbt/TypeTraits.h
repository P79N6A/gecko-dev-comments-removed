







#ifndef mozilla_TypeTraits_h
#define mozilla_TypeTraits_h

#include "mozilla/Types.h"







#include <wchar.h>

namespace mozilla {



template<typename> struct RemoveCV;







template<typename T, T Value>
struct IntegralConstant
{
  static const T value = Value;
  typedef T ValueType;
  typedef IntegralConstant<T, Value> Type;
};


typedef IntegralConstant<bool, true> TrueType;
typedef IntegralConstant<bool, false> FalseType;





namespace detail {

template<typename T>
struct IsVoidHelper : FalseType {};

template<>
struct IsVoidHelper<void> : TrueType {};

} 









template<typename T>
struct IsVoid : detail::IsVoidHelper<typename RemoveCV<T>::Type> {};

namespace detail {

template <typename T>
struct IsIntegralHelper : FalseType {};

template<> struct IsIntegralHelper<char>               : TrueType {};
template<> struct IsIntegralHelper<signed char>        : TrueType {};
template<> struct IsIntegralHelper<unsigned char>      : TrueType {};
template<> struct IsIntegralHelper<short>              : TrueType {};
template<> struct IsIntegralHelper<unsigned short>     : TrueType {};
template<> struct IsIntegralHelper<int>                : TrueType {};
template<> struct IsIntegralHelper<unsigned int>       : TrueType {};
template<> struct IsIntegralHelper<long>               : TrueType {};
template<> struct IsIntegralHelper<unsigned long>      : TrueType {};
template<> struct IsIntegralHelper<long long>          : TrueType {};
template<> struct IsIntegralHelper<unsigned long long> : TrueType {};
template<> struct IsIntegralHelper<bool>               : TrueType {};
template<> struct IsIntegralHelper<wchar_t>            : TrueType {};
#ifdef MOZ_CHAR16_IS_NOT_WCHAR
template<> struct IsIntegralHelper<char16_t>           : TrueType {};
#endif

} 













template<typename T>
struct IsIntegral : detail::IsIntegralHelper<typename RemoveCV<T>::Type>
{};

template<typename T, typename U>
struct IsSame;

namespace detail {

template<typename T>
struct IsFloatingPointHelper
  : IntegralConstant<bool,
                     IsSame<T, float>::value ||
                     IsSame<T, double>::value ||
                     IsSame<T, long double>::value>
{};

} 










template<typename T>
struct IsFloatingPoint
  : detail::IsFloatingPointHelper<typename RemoveCV<T>::Type>
{};

namespace detail {

template<typename T>
struct IsArrayHelper : FalseType {};

template<typename T, decltype(sizeof(1)) N>
struct IsArrayHelper<T[N]> : TrueType {};

template<typename T>
struct IsArrayHelper<T[]> : TrueType {};

} 









template<typename T>
struct IsArray : detail::IsArrayHelper<typename RemoveCV<T>::Type>
{};

namespace detail {

template<typename T>
struct IsPointerHelper : FalseType {};

template<typename T>
struct IsPointerHelper<T*> : TrueType {};

} 
















template<typename T>
struct IsPointer : detail::IsPointerHelper<typename RemoveCV<T>::Type>
{};












template<typename T>
struct IsLvalueReference : FalseType {};

template<typename T>
struct IsLvalueReference<T&> : TrueType {};












template<typename T>
struct IsRvalueReference : FalseType {};

template<typename T>
struct IsRvalueReference<T&&> : TrueType {};

namespace detail {


template<typename T>
struct IsEnumHelper
  : IntegralConstant<bool, __is_enum(T)>
{};

} 








template<typename T>
struct IsEnum
  : detail::IsEnumHelper<typename RemoveCV<T>::Type>
{};

namespace detail {





template<typename T>
struct IsClassHelper
  : IntegralConstant<bool, __is_class(T)>
{};

} 










template<typename T>
struct IsClass
  : detail::IsClassHelper<typename RemoveCV<T>::Type>
{};

















template<typename T>
struct IsReference
  : IntegralConstant<bool,
                     IsLvalueReference<T>::value || IsRvalueReference<T>::value>
{};









template<typename T>
struct IsArithmetic
  : IntegralConstant<bool, IsIntegral<T>::value || IsFloatingPoint<T>::value>
{};










template<typename T>
struct IsConst : FalseType {};

template<typename T>
struct IsConst<const T> : TrueType {};








template<typename T>
struct IsVolatile : FalseType {};

template<typename T>
struct IsVolatile<volatile T> : TrueType {};









template<typename T>
struct IsPod : public FalseType {};

template<> struct IsPod<char>               : TrueType {};
template<> struct IsPod<signed char>        : TrueType {};
template<> struct IsPod<unsigned char>      : TrueType {};
template<> struct IsPod<short>              : TrueType {};
template<> struct IsPod<unsigned short>     : TrueType {};
template<> struct IsPod<int>                : TrueType {};
template<> struct IsPod<unsigned int>       : TrueType {};
template<> struct IsPod<long>               : TrueType {};
template<> struct IsPod<unsigned long>      : TrueType {};
template<> struct IsPod<long long>          : TrueType {};
template<> struct IsPod<unsigned long long> : TrueType {};
template<> struct IsPod<bool>               : TrueType {};
template<> struct IsPod<float>              : TrueType {};
template<> struct IsPod<double>             : TrueType {};
template<> struct IsPod<wchar_t>            : TrueType {};
#ifdef MOZ_CHAR16_IS_NOT_WCHAR
template<> struct IsPod<char16_t>           : TrueType {};
#endif
template<typename T> struct IsPod<T*>       : TrueType {};

namespace detail {





template<typename T>
struct IsEmptyHelper
  : IntegralConstant<bool, IsClass<T>::value && __is_empty(T)>
{};

} 









































template<typename T>
struct IsEmpty : detail::IsEmptyHelper<typename RemoveCV<T>::Type>
{};


namespace detail {

template<typename T,
         bool = IsFloatingPoint<T>::value,
         bool = IsIntegral<T>::value,
         typename NoCV = typename RemoveCV<T>::Type>
struct IsSignedHelper;


template<typename T, typename NoCV>
struct IsSignedHelper<T, true, false, NoCV> : TrueType {};


template<typename T, typename NoCV>
struct IsSignedHelper<T, false, true, NoCV>
  : IntegralConstant<bool, bool(NoCV(-1) < NoCV(1))>
{};


template<typename T, typename NoCV>
struct IsSignedHelper<T, false, false, NoCV> : FalseType {};

} 










template<typename T>
struct IsSigned : detail::IsSignedHelper<T> {};

namespace detail {

template<typename T,
         bool = IsFloatingPoint<T>::value,
         bool = IsIntegral<T>::value,
         typename NoCV = typename RemoveCV<T>::Type>
struct IsUnsignedHelper;


template<typename T, typename NoCV>
struct IsUnsignedHelper<T, true, false, NoCV> : FalseType {};


template<typename T, typename NoCV>
struct IsUnsignedHelper<T, false, true, NoCV>
  : IntegralConstant<bool,
                     (IsSame<NoCV, bool>::value || bool(NoCV(1) < NoCV(-1)))>
{};


template<typename T, typename NoCV>
struct IsUnsignedHelper<T, false, false, NoCV> : FalseType {};

} 









template<typename T>
struct IsUnsigned : detail::IsUnsignedHelper<T> {};















template<typename T, typename U>
struct IsSame : FalseType {};

template<typename T>
struct IsSame<T, T> : TrueType {};

namespace detail {

#if defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)

template<class Base, class Derived>
struct BaseOfTester : IntegralConstant<bool, __is_base_of(Base, Derived)> {};

#else






template<class Base, class Derived>
struct BaseOfHelper
{
public:
  operator Base*() const;
  operator Derived*();
};

template<class Base, class Derived>
struct BaseOfTester
{
private:
  template<class T>
  static char test(Derived*, T);
  static int test(Base*, int);

public:
  static const bool value =
    sizeof(test(BaseOfHelper<Base, Derived>(), int())) == sizeof(char);
};

template<class Base, class Derived>
struct BaseOfTester<Base, const Derived>
{
private:
  template<class T>
  static char test(Derived*, T);
  static int test(Base*, int);

public:
  static const bool value =
    sizeof(test(BaseOfHelper<Base, Derived>(), int())) == sizeof(char);
};

template<class Base, class Derived>
struct BaseOfTester<Base&, Derived&> : FalseType {};

template<class Type>
struct BaseOfTester<Type, Type> : TrueType {};

template<class Type>
struct BaseOfTester<Type, const Type> : TrueType {};

#endif

} 













template<class Base, class Derived>
struct IsBaseOf
  : IntegralConstant<bool, detail::BaseOfTester<Base, Derived>::value>
{};

namespace detail {

template<typename From, typename To>
struct ConvertibleTester
{
private:
  static From create();

  template<typename From1, typename To1>
  static char test(To to);

  template<typename From1, typename To1>
  static int test(...);

public:
  static const bool value =
    sizeof(test<From, To>(create())) == sizeof(char);
};

} 






















template<typename From, typename To>
struct IsConvertible
  : IntegralConstant<bool, detail::ConvertibleTester<From, To>::value>
{};













template<typename T>
struct RemoveConst
{
  typedef T Type;
};

template<typename T>
struct RemoveConst<const T>
{
  typedef T Type;
};









template<typename T>
struct RemoveVolatile
{
  typedef T Type;
};

template<typename T>
struct RemoveVolatile<volatile T>
{
  typedef T Type;
};









template<typename T>
struct RemoveCV
{
  typedef typename RemoveConst<typename RemoveVolatile<T>::Type>::Type Type;
};











template<typename T>
struct RemoveReference
{
  typedef T Type;
};

template<typename T>
struct RemoveReference<T&>
{
  typedef T Type;
};

template<typename T>
struct RemoveReference<T&&>
{
  typedef T Type;
};

template<bool Condition, typename A, typename B>
struct Conditional;

namespace detail {

enum Voidness { TIsVoid, TIsNotVoid };

template<typename T, Voidness V = IsVoid<T>::value ? TIsVoid : TIsNotVoid>
struct AddLvalueReferenceHelper;

template<typename T>
struct AddLvalueReferenceHelper<T, TIsVoid>
{
  typedef void Type;
};

template<typename T>
struct AddLvalueReferenceHelper<T, TIsNotVoid>
{
  typedef T& Type;
};

} 















template<typename T>
struct AddLvalueReference
  : detail::AddLvalueReferenceHelper<T>
{};



template<bool B, typename T = void>
struct EnableIf;

namespace detail {

template<bool MakeConst, typename T>
struct WithC : Conditional<MakeConst, const T, T>
{};

template<bool MakeVolatile, typename T>
struct WithV : Conditional<MakeVolatile, volatile T, T>
{};


template<bool MakeConst, bool MakeVolatile, typename T>
struct WithCV : WithC<MakeConst, typename WithV<MakeVolatile, T>::Type>
{};

template<typename T>
struct CorrespondingSigned;

template<>
struct CorrespondingSigned<char> { typedef signed char Type; };
template<>
struct CorrespondingSigned<unsigned char> { typedef signed char Type; };
template<>
struct CorrespondingSigned<unsigned short> { typedef short Type; };
template<>
struct CorrespondingSigned<unsigned int> { typedef int Type; };
template<>
struct CorrespondingSigned<unsigned long> { typedef long Type; };
template<>
struct CorrespondingSigned<unsigned long long> { typedef long long Type; };

template<typename T,
         typename CVRemoved = typename RemoveCV<T>::Type,
         bool IsSignedIntegerType = IsSigned<CVRemoved>::value &&
                                    !IsSame<char, CVRemoved>::value>
struct MakeSigned;

template<typename T, typename CVRemoved>
struct MakeSigned<T, CVRemoved, true>
{
  typedef T Type;
};

template<typename T, typename CVRemoved>
struct MakeSigned<T, CVRemoved, false>
  : WithCV<IsConst<T>::value, IsVolatile<T>::value,
           typename CorrespondingSigned<CVRemoved>::Type>
{};

} 























template<typename T>
struct MakeSigned
  : EnableIf<IsIntegral<T>::value &&
             !IsSame<bool, typename RemoveCV<T>::Type>::value,
             typename detail::MakeSigned<T>
            >::Type
{};

namespace detail {

template<typename T>
struct CorrespondingUnsigned;

template<>
struct CorrespondingUnsigned<char> { typedef unsigned char Type; };
template<>
struct CorrespondingUnsigned<signed char> { typedef unsigned char Type; };
template<>
struct CorrespondingUnsigned<short> { typedef unsigned short Type; };
template<>
struct CorrespondingUnsigned<int> { typedef unsigned int Type; };
template<>
struct CorrespondingUnsigned<long> { typedef unsigned long Type; };
template<>
struct CorrespondingUnsigned<long long> { typedef unsigned long long Type; };


template<typename T,
         typename CVRemoved = typename RemoveCV<T>::Type,
         bool IsUnsignedIntegerType = IsUnsigned<CVRemoved>::value &&
                                      !IsSame<char, CVRemoved>::value>
struct MakeUnsigned;

template<typename T, typename CVRemoved>
struct MakeUnsigned<T, CVRemoved, true>
{
  typedef T Type;
};

template<typename T, typename CVRemoved>
struct MakeUnsigned<T, CVRemoved, false>
  : WithCV<IsConst<T>::value, IsVolatile<T>::value,
           typename CorrespondingUnsigned<CVRemoved>::Type>
{};

} 























template<typename T>
struct MakeUnsigned
  : EnableIf<IsIntegral<T>::value &&
             !IsSame<bool, typename RemoveCV<T>::Type>::value,
             typename detail::MakeUnsigned<T>
            >::Type
{};












template<typename T>
struct RemoveExtent
{
  typedef T Type;
};

template<typename T>
struct RemoveExtent<T[]>
{
  typedef T Type;
};

template<typename T, decltype(sizeof(1)) N>
struct RemoveExtent<T[N]>
{
  typedef T Type;
};























template<bool B, typename T>
struct EnableIf
{};

template<typename T>
struct EnableIf<true, T>
{
  typedef T Type;
};







template<bool Condition, typename A, typename B>
struct Conditional
{
  typedef A Type;
};

template<class A, class B>
struct Conditional<false, A, B>
{
  typedef B Type;
};

} 

#endif 
