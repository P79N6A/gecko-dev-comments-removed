





#ifndef mozilla_TypeTraits_h_
#define mozilla_TypeTraits_h_







#include <wchar.h>

namespace mozilla {







template<typename T, T Value>
struct IntegralConstant
{
    static const T value = Value;
    typedef T ValueType;
    typedef IntegralConstant<T, Value> Type;
};


typedef IntegralConstant<bool, true> TrueType;
typedef IntegralConstant<bool, false> FalseType;















template<typename T>
struct IsPointer : FalseType {};

template<typename T>
struct IsPointer<T*> : TrueType {};













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
