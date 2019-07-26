




#include "gtest/gtest.h"
#include "TestBase.h"
#include "TestPoint.h"
#include "TestScaling.h"

TEST(Moz2D, Point) {
  TestBase* test = new TestPoint();
  int failures = 0;
  int totalTests = test->RunTests(&failures);
  delete test;

  ASSERT_EQ(failures, 0);
}

TEST(Moz2D, Scaling) {
  TestBase* test = new TestScaling();
  int failures = 0;
  int totalTests = test->RunTests(&failures);
  delete test;

  ASSERT_EQ(failures, 0);
}
