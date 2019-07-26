




#include "mozilla/Assertions.h"
#include "mozilla/TypeTraits.h"

using mozilla::IsBaseOf;
using mozilla::IsConvertible;

namespace CPlusPlus11IsBaseOf {


struct B {};
struct B1 : B {};
struct B2 : B {};
struct D : private B1, private B2 {};

static void
StandardIsBaseOfTests()
{
  MOZ_ASSERT((IsBaseOf<B, D>::value) == true);
  MOZ_ASSERT((IsBaseOf<const B, D>::value) == true);
  MOZ_ASSERT((IsBaseOf<B, const D>::value) == true);
  MOZ_ASSERT((IsBaseOf<B, const B>::value) == true);
  MOZ_ASSERT((IsBaseOf<D, B>::value) == false);
  MOZ_ASSERT((IsBaseOf<B&, D&>::value) == false);
  MOZ_ASSERT((IsBaseOf<B[3], D[3]>::value) == false);
  
  
  
}

} 

class A { };
class B : public A { };
class C : private A { };
class D { };
class E : public A { };
class F : public B, public E { };

static void
TestIsBaseOf()
{
  MOZ_ASSERT((IsBaseOf<A, B>::value),
             "A is a base of B");
  MOZ_ASSERT((!IsBaseOf<B, A>::value),
             "B is not a base of A");
  MOZ_ASSERT((IsBaseOf<A, C>::value),
             "A is a base of C");
  MOZ_ASSERT((!IsBaseOf<C, A>::value),
             "C is not a base of A");
  MOZ_ASSERT((IsBaseOf<A, F>::value),
             "A is a base of F");
  MOZ_ASSERT((!IsBaseOf<F, A>::value),
             "F is not a base of A");
  MOZ_ASSERT((!IsBaseOf<A, D>::value),
             "A is not a base of D");
  MOZ_ASSERT((!IsBaseOf<D, A>::value),
             "D is not a base of A");
  MOZ_ASSERT((IsBaseOf<B, B>::value),
             "B is the same as B (and therefore, a base of B)");
}

static void
TestIsConvertible()
{
  
  MOZ_ASSERT((IsConvertible<A*, A*>::value),
             "A* should convert to A*");
  MOZ_ASSERT((IsConvertible<B*, A*>::value),
             "B* should convert to A*");
  MOZ_ASSERT((!IsConvertible<A*, B*>::value),
             "A* shouldn't convert to B*");
  MOZ_ASSERT((!IsConvertible<A*, C*>::value),
             "A* shouldn't convert to C*");
  MOZ_ASSERT((!IsConvertible<A*, D*>::value),
             "A* shouldn't convert to unrelated D*");
  MOZ_ASSERT((!IsConvertible<D*, A*>::value),
             "D* shouldn't convert to unrelated A*");

  
  MOZ_ASSERT((IsConvertible<A, A>::value),
             "A is A");
  MOZ_ASSERT((IsConvertible<B, A>::value),
             "B converts to A");
  MOZ_ASSERT((!IsConvertible<D, A>::value),
             "D and A are unrelated");
  MOZ_ASSERT((!IsConvertible<A, D>::value),
             "A and D are unrelated");

  
  
  
  
  
  
}

int
main()
{
  CPlusPlus11IsBaseOf::StandardIsBaseOfTests();
  TestIsBaseOf();
  TestIsConvertible();
}
