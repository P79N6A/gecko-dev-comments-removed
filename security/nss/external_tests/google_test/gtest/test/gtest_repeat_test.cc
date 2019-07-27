
































#include <stdlib.h>
#include <iostream>
#include "gtest/gtest.h"






#define GTEST_IMPLEMENTATION_ 1
#include "src/gtest-internal-inl.h"
#undef GTEST_IMPLEMENTATION_

namespace testing {

GTEST_DECLARE_string_(death_test_style);
GTEST_DECLARE_string_(filter);
GTEST_DECLARE_int32_(repeat);

}  

using testing::GTEST_FLAG(death_test_style);
using testing::GTEST_FLAG(filter);
using testing::GTEST_FLAG(repeat);

namespace {



#define GTEST_CHECK_INT_EQ_(expected, actual) \
  do {\
    const int expected_val = (expected);\
    const int actual_val = (actual);\
    if (::testing::internal::IsTrue(expected_val != actual_val)) {\
      ::std::cout << "Value of: " #actual "\n"\
                  << "  Actual: " << actual_val << "\n"\
                  << "Expected: " #expected "\n"\
                  << "Which is: " << expected_val << "\n";\
      ::testing::internal::posix::Abort();\
    }\
  } while (::testing::internal::AlwaysFalse())





int g_environment_set_up_count = 0;
int g_environment_tear_down_count = 0;

class MyEnvironment : public testing::Environment {
 public:
  MyEnvironment() {}
  virtual void SetUp() { g_environment_set_up_count++; }
  virtual void TearDown() { g_environment_tear_down_count++; }
};



int g_should_fail_count = 0;

TEST(FooTest, ShouldFail) {
  g_should_fail_count++;
  EXPECT_EQ(0, 1) << "Expected failure.";
}



int g_should_pass_count = 0;

TEST(FooTest, ShouldPass) {
  g_should_pass_count++;
}




int g_death_test_count = 0;

TEST(BarDeathTest, ThreadSafeAndFast) {
  g_death_test_count++;

  GTEST_FLAG(death_test_style) = "threadsafe";
  EXPECT_DEATH_IF_SUPPORTED(::testing::internal::posix::Abort(), "");

  GTEST_FLAG(death_test_style) = "fast";
  EXPECT_DEATH_IF_SUPPORTED(::testing::internal::posix::Abort(), "");
}

#if GTEST_HAS_PARAM_TEST
int g_param_test_count = 0;

const int kNumberOfParamTests = 10;

class MyParamTest : public testing::TestWithParam<int> {};

TEST_P(MyParamTest, ShouldPass) {
  
  
  GTEST_CHECK_INT_EQ_(g_param_test_count % kNumberOfParamTests, GetParam());
  g_param_test_count++;
}
INSTANTIATE_TEST_CASE_P(MyParamSequence,
                        MyParamTest,
                        testing::Range(0, kNumberOfParamTests));
#endif  


void ResetCounts() {
  g_environment_set_up_count = 0;
  g_environment_tear_down_count = 0;
  g_should_fail_count = 0;
  g_should_pass_count = 0;
  g_death_test_count = 0;
#if GTEST_HAS_PARAM_TEST
  g_param_test_count = 0;
#endif  
}


void CheckCounts(int expected) {
  GTEST_CHECK_INT_EQ_(expected, g_environment_set_up_count);
  GTEST_CHECK_INT_EQ_(expected, g_environment_tear_down_count);
  GTEST_CHECK_INT_EQ_(expected, g_should_fail_count);
  GTEST_CHECK_INT_EQ_(expected, g_should_pass_count);
  GTEST_CHECK_INT_EQ_(expected, g_death_test_count);
#if GTEST_HAS_PARAM_TEST
  GTEST_CHECK_INT_EQ_(expected * kNumberOfParamTests, g_param_test_count);
#endif  
}


void TestRepeatUnspecified() {
  ResetCounts();
  GTEST_CHECK_INT_EQ_(1, RUN_ALL_TESTS());
  CheckCounts(1);
}


void TestRepeat(int repeat) {
  GTEST_FLAG(repeat) = repeat;

  ResetCounts();
  GTEST_CHECK_INT_EQ_(repeat > 0 ? 1 : 0, RUN_ALL_TESTS());
  CheckCounts(repeat);
}



void TestRepeatWithEmptyFilter(int repeat) {
  GTEST_FLAG(repeat) = repeat;
  GTEST_FLAG(filter) = "None";

  ResetCounts();
  GTEST_CHECK_INT_EQ_(0, RUN_ALL_TESTS());
  CheckCounts(0);
}



void TestRepeatWithFilterForSuccessfulTests(int repeat) {
  GTEST_FLAG(repeat) = repeat;
  GTEST_FLAG(filter) = "*-*ShouldFail";

  ResetCounts();
  GTEST_CHECK_INT_EQ_(0, RUN_ALL_TESTS());
  GTEST_CHECK_INT_EQ_(repeat, g_environment_set_up_count);
  GTEST_CHECK_INT_EQ_(repeat, g_environment_tear_down_count);
  GTEST_CHECK_INT_EQ_(0, g_should_fail_count);
  GTEST_CHECK_INT_EQ_(repeat, g_should_pass_count);
  GTEST_CHECK_INT_EQ_(repeat, g_death_test_count);
#if GTEST_HAS_PARAM_TEST
  GTEST_CHECK_INT_EQ_(repeat * kNumberOfParamTests, g_param_test_count);
#endif  
}



void TestRepeatWithFilterForFailedTests(int repeat) {
  GTEST_FLAG(repeat) = repeat;
  GTEST_FLAG(filter) = "*ShouldFail";

  ResetCounts();
  GTEST_CHECK_INT_EQ_(1, RUN_ALL_TESTS());
  GTEST_CHECK_INT_EQ_(repeat, g_environment_set_up_count);
  GTEST_CHECK_INT_EQ_(repeat, g_environment_tear_down_count);
  GTEST_CHECK_INT_EQ_(repeat, g_should_fail_count);
  GTEST_CHECK_INT_EQ_(0, g_should_pass_count);
  GTEST_CHECK_INT_EQ_(0, g_death_test_count);
#if GTEST_HAS_PARAM_TEST
  GTEST_CHECK_INT_EQ_(0, g_param_test_count);
#endif  
}

}  

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::AddGlobalTestEnvironment(new MyEnvironment);

  TestRepeatUnspecified();
  TestRepeat(0);
  TestRepeat(1);
  TestRepeat(5);

  TestRepeatWithEmptyFilter(2);
  TestRepeatWithEmptyFilter(3);

  TestRepeatWithFilterForSuccessfulTests(3);

  TestRepeatWithFilterForFailedTests(4);

  
  
  
  
  

  printf("PASS\n");
  return 0;
}
