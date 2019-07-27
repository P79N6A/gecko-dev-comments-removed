

































#include "gtest/gtest.h"

namespace {

void Subroutine() {
  EXPECT_EQ(42, 42);
}

TEST(NoFatalFailureTest, ExpectNoFatalFailure) {
  EXPECT_NO_FATAL_FAILURE(;);
  EXPECT_NO_FATAL_FAILURE(SUCCEED());
  EXPECT_NO_FATAL_FAILURE(Subroutine());
  EXPECT_NO_FATAL_FAILURE({ SUCCEED(); });
}

TEST(NoFatalFailureTest, AssertNoFatalFailure) {
  ASSERT_NO_FATAL_FAILURE(;);
  ASSERT_NO_FATAL_FAILURE(SUCCEED());
  ASSERT_NO_FATAL_FAILURE(Subroutine());
  ASSERT_NO_FATAL_FAILURE({ SUCCEED(); });
}

}  
