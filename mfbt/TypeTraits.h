





#ifndef mozilla_TypeTraits_h_
#define mozilla_TypeTraits_h_

namespace mozilla {













template<class Base, class Derived>
class IsBaseOf
{
  private:
    static char test(Base* b);
    static int test(...);
  public:
    static const bool value = (sizeof(test(static_cast<Derived*>(0))) == sizeof(char));
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

} 

#endif  
