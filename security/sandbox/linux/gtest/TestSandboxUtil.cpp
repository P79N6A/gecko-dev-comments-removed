





#include "gtest/gtest.h"

#include "SandboxUtil.h"

#include <pthread.h>

namespace mozilla {





namespace {

struct EarlyTest {
  bool mWasSingleThreaded;

  EarlyTest()
  : mWasSingleThreaded(IsSingleThreaded())
  { }
};

static const EarlyTest gEarlyTest;

} 

TEST(SandboxUtil, IsSingleThreaded)
{
  EXPECT_TRUE(gEarlyTest.mWasSingleThreaded);
  EXPECT_FALSE(IsSingleThreaded());
}

} 
