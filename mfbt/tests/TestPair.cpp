





#include "mozilla/Pair.h"
#include "mozilla/TypeTraits.h"

using mozilla::IsSame;
using mozilla::MakePair;
using mozilla::Pair;



#define INSTANTIATE(T1, T2, name, size) \
  Pair<T1, T2> name##_1(T1(0), T2(0)); \
  static_assert(sizeof(name##_1.first()) > 0, \
                "first method should work on Pair<" #T1 ", " #T2 ">"); \
  static_assert(sizeof(name##_1.second()) > 0, \
                "second method should work on Pair<" #T1 ", " #T2 ">"); \
  static_assert(sizeof(name##_1) == (size), \
                "Pair<" #T1 ", " #T2 "> has an unexpected size"); \
  Pair<T2, T1> name##_2(T2(0), T1(0)); \
  static_assert(sizeof(name##_2.first()) > 0, \
                "first method should work on Pair<" #T2 ", " #T1 ">"); \
  static_assert(sizeof(name##_2.second()) > 0, \
                "second method should work on Pair<" #T2 ", " #T1 ">"); \
  static_assert(sizeof(name##_2) == (size), \
                "Pair<" #T2 ", " #T1 "> has an unexpected size");

INSTANTIATE(int, int, prim1, 2 * sizeof(int));
INSTANTIATE(int, long, prim2, 2 * sizeof(long));

struct EmptyClass { explicit EmptyClass(int) {} };
struct NonEmpty { char mC; explicit NonEmpty(int) {} };

INSTANTIATE(int, EmptyClass, both1, sizeof(int));
INSTANTIATE(int, NonEmpty, both2, 2 * sizeof(int));
INSTANTIATE(EmptyClass, NonEmpty, both3, 1);

struct A { char dummy; explicit A(int) {} };
struct B : A { explicit B(int aI) : A(aI) {} };

INSTANTIATE(A, A, class1, 2);
INSTANTIATE(A, B, class2, 2);
INSTANTIATE(A, EmptyClass, class3, 1);

struct OtherEmpty : EmptyClass { explicit OtherEmpty(int aI) : EmptyClass(aI) {} };












int
main()
{
  A a(0);
  B b(0);
  const A constA(0);
  const B constB(0);

  
  static_assert(IsSame<decltype(MakePair(A(0), B(0))), Pair<A, B>>::value,
                "MakePair should strip rvalue references");
  static_assert(IsSame<decltype(MakePair(a, b)), Pair<A, B>>::value,
                "MakePair should strip lvalue references");
  static_assert(IsSame<decltype(MakePair(constA, constB)), Pair<A, B>>::value,
                "MakePair should strip CV-qualifiers");

  
  a = constA;
  a = A(0);

  return 0;
}
