





#ifndef mozilla_TypeTraits_h_
#define mozilla_TypeTraits_h_







#include <wchar.h>

namespace mozilla {

namespace detail {








template<class Base, class Derived>
class IsBaseOfHelper
{
  public:
    operator Base*() const;
    operator Derived*();
};

} 













template<class Base, class Derived>
class IsBaseOf
{
  private:
    template<class T>
    static char test(Derived*, T);
    static int test(Base*, int);

  public:
    static const bool value =
      sizeof(test(detail::IsBaseOfHelper<Base, Derived>(), int())) == sizeof(char);
};

template<class Base, class Derived>
class IsBaseOf<Base, const Derived>
{
  private:
    template<class T>
    static char test(Derived*, T);
    static int test(Base*, int);

  public:
    static const bool value =
      sizeof(test(detail::IsBaseOfHelper<Base, Derived>(), int())) == sizeof(char);
};

template<class Base, class Derived>
class IsBaseOf<Base&, Derived&>
{
  public:
    static const bool value = false;
};

template<class Type>
class IsBaseOf<Type, Type>
{
  public:
    static const bool value = true;
};

template<class Type>
class IsBaseOf<Type, const Type>
{
  public:
    static const bool value = true;
};






















template<typename From, typename To>
struct IsConvertible
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







template<bool condition, class A, class B>
struct Conditional
{
    typedef A Type;
};

template<class A, class B>
struct Conditional<false, A, B>
{
    typedef B Type;
};



















template<bool B, typename T = void>
struct EnableIf
{};

template<typename T>
struct EnableIf<true, T>
{
    typedef T Type;
};











template<typename T, typename U>
struct IsSame
{
    static const bool value = false;
};

template<typename T>
struct IsSame<T, T>
{
    static const bool value = true;
};





template<typename T>
struct IsPod
{
    static const bool value = false;
};
template<> struct IsPod<char>               { static const bool value = true; };
template<> struct IsPod<signed char>        { static const bool value = true; };
template<> struct IsPod<unsigned char>      { static const bool value = true; };
template<> struct IsPod<short>              { static const bool value = true; };
template<> struct IsPod<unsigned short>     { static const bool value = true; };
template<> struct IsPod<int>                { static const bool value = true; };
template<> struct IsPod<unsigned int>       { static const bool value = true; };
template<> struct IsPod<long>               { static const bool value = true; };
template<> struct IsPod<unsigned long>      { static const bool value = true; };
template<> struct IsPod<long long>          { static const bool value = true; };
template<> struct IsPod<unsigned long long> { static const bool value = true; };
template<> struct IsPod<bool>               { static const bool value = true; };
template<> struct IsPod<float>              { static const bool value = true; };
template<> struct IsPod<double>             { static const bool value = true; };
template<> struct IsPod<wchar_t>            { static const bool value = true; };
template<typename T> struct IsPod<T*>       { static const bool value = true; };











template<typename T>
struct IsPointer
{
    static const bool value = false;
};
template<typename T>
struct IsPointer<T*>
{
    static const bool value = true;
};

} 

#endif  
