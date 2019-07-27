





#include "mozilla/Assertions.h"
#include "mozilla/ScopeExit.h"

using mozilla::MakeScopeExit;

#define CHECK(c) \
  do { \
    bool cond = !!(c); \
    MOZ_RELEASE_ASSERT(cond, "Failed assertion: " #c); \
    if (!cond) { \
      return false; \
    } \
  } while (false)

static bool
Test()
{
  int a = 1;
  int b = 1;

  {
    a++;
    auto guardA = MakeScopeExit([&] {
      a--;
    });

    b++;
    auto guardB = MakeScopeExit([&] {
      b--;
    });

    guardB.release();
  }

  CHECK(a == 1);
  CHECK(b == 2);

  return true;
}

int
main()
{
  if (!Test()) {
    return 1;
  }
  return 0;
}
