
   




#include "mozilla/Assertions.h"
#include "mozilla/Move.h"
#include "mozilla/Tuple.h"
#include "mozilla/TypeTraits.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/unused.h"

#include <stddef.h>

using mozilla::Get;
using mozilla::IsSame;
using mozilla::MakeTuple;
using mozilla::MakeUnique;
using mozilla::Move;
using mozilla::Tie;
using mozilla::Tuple;
using mozilla::UniquePtr;
using mozilla::unused;

#define CHECK(c) \
  do { \
    bool cond = !!(c); \
    MOZ_RELEASE_ASSERT(cond, "Failed assertion: " #c); \
  } while (false)



#define CHECK_TYPE(expression, ...)  \
  static_assert(IsSame<decltype(expression), __VA_ARGS__>::value, \
      "Type mismatch!")

struct ConvertibleToInt
{
  operator int() const { return 42; }
};

static void
TestConstruction()
{
  
  Tuple<> a;
  unused << a;
  Tuple<int> b;
  unused << b;

  
  int x = 1, y = 1;
  Tuple<int, int> c{x, y};
  Tuple<int&, const int&> d{x, y};
  x = 42;
  y = 42;
  CHECK(Get<0>(c) == 1);
  CHECK(Get<1>(c) == 1);
  CHECK(Get<0>(d) == 42);
  CHECK(Get<1>(d) == 42);

  
  Tuple<int, int> e{1.0, ConvertibleToInt{}};

  
  Tuple<int> x1;
  Tuple<int> x2{x1};

  Tuple<int, int> f(c);
  CHECK(Get<0>(f) == 1);
  CHECK(Get<0>(f) == 1);

  
  Tuple<UniquePtr<int>> g{MakeUnique<int>(42)};
  Tuple<UniquePtr<int>> h{Move(g)};
  CHECK(Get<0>(g) == nullptr);
  CHECK(*Get<0>(h) == 42);
}

static void
TestAssignment()
{
  
  Tuple<int> a{0};
  Tuple<int> b{42};
  a = b;
  CHECK(Get<0>(a) == 42);

  
  int i = 0;
  int j = 42;
  Tuple<int&> c{i};
  Tuple<int&> d{j};
  c = d;
  CHECK(i == 42);

  
  Tuple<UniquePtr<int>> e{MakeUnique<int>(0)};
  Tuple<UniquePtr<int>> f{MakeUnique<int>(42)};
  e = Move(f);
  CHECK(*Get<0>(e) == 42);
  CHECK(Get<0>(f) == nullptr);
}

static void
TestGet()
{
  int x = 1;
  int y = 2;
  int z = 3;
  Tuple<int, int&, const int&> tuple(x, y, z);

  
  CHECK(Get<0>(tuple) == 1);
  CHECK(Get<1>(tuple) == 2);
  CHECK(Get<2>(tuple) == 3);

  
  Get<0>(tuple) = 41;
  CHECK(Get<0>(tuple) == 41);

  
  Get<1>(tuple) = 42;
  CHECK(Get<1>(tuple) == 42);
  CHECK(y == 42);
}

static bool
TestMakeTuple()
{
  auto tuple = MakeTuple(42, 0.5f, 'c');
  CHECK_TYPE(tuple, Tuple<int, float, char>);
  CHECK(Get<0>(tuple) == 42);
  CHECK(Get<1>(tuple) == 1.0f);
  CHECK(Get<2>(tuple) == 'c');
  return true;
}

static bool
TestTie()
{
  int i;
  float f;
  char c;
  Tuple<int, float, char> rhs1(42, 0.5f, 'c');
  Tie(i, f, c) = rhs1;
  CHECK(i == Get<0>(rhs1));
  CHECK(f == Get<1>(rhs1));
  CHECK(c == Get<2>(rhs1));
  
  Tuple<ConvertibleToInt, double, unsigned char> rhs2(ConvertibleToInt(),
      0.7f, 'd');
  Tie(i, f, c) = rhs2;
  CHECK(i == Get<0>(rhs2));
  CHECK(f == Get<1>(rhs2));
  CHECK(c == Get<2>(rhs2));
  return true;
}

int
main()
{
  TestConstruction();
  TestAssignment();
  TestGet();
  TestMakeTuple();
  TestTie();
  return 0;
}
