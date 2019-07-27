
































#include <stdlib.h>
#include <stdio.h>
#include "gtest/gtest.h"

#define GTEST_IMPLEMENTATION_ 1  // Required for the next #include.
#include "src/gtest-internal-inl.h"
#undef GTEST_IMPLEMENTATION_

namespace testing {
GTEST_DECLARE_string_(filter);
}

namespace {

enum FailureType {
  NO_FAILURE, NON_FATAL_FAILURE, FATAL_FAILURE
};


class MyEnvironment : public testing::Environment {
 public:
  MyEnvironment() { Reset(); }

  
  
  
  virtual void SetUp() {
    set_up_was_run_ = true;

    switch (failure_in_set_up_) {
      case NON_FATAL_FAILURE:
        ADD_FAILURE() << "Expected non-fatal failure in global set-up.";
        break;
      case FATAL_FAILURE:
        FAIL() << "Expected fatal failure in global set-up.";
        break;
      default:
        break;
    }
  }

  
  virtual void TearDown() {
    tear_down_was_run_ = true;
    ADD_FAILURE() << "Expected non-fatal failure in global tear-down.";
  }

  
  void Reset() {
    failure_in_set_up_ = NO_FAILURE;
    set_up_was_run_ = false;
    tear_down_was_run_ = false;
  }

  
  
  void set_failure_in_set_up(FailureType type) {
    failure_in_set_up_ = type;
  }

  
  bool set_up_was_run() const { return set_up_was_run_; }

  
  bool tear_down_was_run() const { return tear_down_was_run_; }

 private:
  FailureType failure_in_set_up_;
  bool set_up_was_run_;
  bool tear_down_was_run_;
};


bool test_was_run;



TEST(FooTest, Bar) {
  test_was_run = true;
}


void Check(bool condition, const char* msg) {
  if (!condition) {
    printf("FAILED: %s\n", msg);
    testing::internal::posix::Abort();
  }
}





int RunAllTests(MyEnvironment* env, FailureType failure) {
  env->Reset();
  env->set_failure_in_set_up(failure);
  test_was_run = false;
  testing::internal::GetUnitTestImpl()->ClearAdHocTestResult();
  return RUN_ALL_TESTS();
}

}  

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  
  
  MyEnvironment* const env = new MyEnvironment;
  Check(testing::AddGlobalTestEnvironment(env) == env,
        "AddGlobalTestEnvironment() should return its argument.");

  
  
  Check(RunAllTests(env, NO_FAILURE) != 0,
        "RUN_ALL_TESTS() should return non-zero, as the global tear-down "
        "should generate a failure.");
  Check(test_was_run,
        "The tests should run, as the global set-up should generate no "
        "failure");
  Check(env->tear_down_was_run(),
        "The global tear-down should run, as the global set-up was run.");

  
  
  Check(RunAllTests(env, NON_FATAL_FAILURE) != 0,
        "RUN_ALL_TESTS() should return non-zero, as both the global set-up "
        "and the global tear-down should generate a non-fatal failure.");
  Check(test_was_run,
        "The tests should run, as the global set-up should generate no "
        "fatal failure.");
  Check(env->tear_down_was_run(),
        "The global tear-down should run, as the global set-up was run.");

  
  
  Check(RunAllTests(env, FATAL_FAILURE) != 0,
        "RUN_ALL_TESTS() should return non-zero, as the global set-up "
        "should generate a fatal failure.");
  Check(!test_was_run,
        "The tests should not run, as the global set-up should generate "
        "a fatal failure.");
  Check(env->tear_down_was_run(),
        "The global tear-down should run, as the global set-up was run.");

  
  
  testing::GTEST_FLAG(filter) = "-*";
  Check(RunAllTests(env, NO_FAILURE) == 0,
        "RUN_ALL_TESTS() should return zero, as there is no test to run.");
  Check(!env->set_up_was_run(),
        "The global set-up should not run, as there is no test to run.");
  Check(!env->tear_down_was_run(),
        "The global tear-down should not run, "
        "as the global set-up was not run.");

  printf("PASS\n");
  return 0;
}
