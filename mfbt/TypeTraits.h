






#ifndef mozilla_TypeTraits_h_
#define mozilla_TypeTraits_h_







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











template<typename T>
struct IsPointer : FalseType {};

template<typename T>
struct IsPointer<T*> : TrueType {};











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
template<typename T> struct IsPod<T*>       : TrueType {};












template<typename T>
struct IsSigned
  : IntegralConstant<bool, IsArithmetic<T>::value && T(-1) < T(0)>
{};












template<typename T>
struct IsUnsigned
  : IntegralConstant<bool, IsArithmetic<T>::value && T(0) < T(-1)>
{};















template<typename T, typename U>
struct IsSame : FalseType {};

template<typename T>
struct IsSame<T, T> : TrueType {};

namespace detail {






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





























template<bool B, typename T = void>
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
