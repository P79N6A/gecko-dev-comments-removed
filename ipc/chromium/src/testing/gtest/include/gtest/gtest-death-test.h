




































#ifndef GTEST_INCLUDE_GTEST_GTEST_DEATH_TEST_H_
#define GTEST_INCLUDE_GTEST_GTEST_DEATH_TEST_H_

#include <gtest/internal/gtest-death-test-internal.h>

namespace testing {






GTEST_DECLARE_string_(death_test_style);

#if GTEST_HAS_DEATH_TEST








































































































#define ASSERT_EXIT(statement, predicate, regex) \
  GTEST_DEATH_TEST_(statement, predicate, regex, GTEST_FATAL_FAILURE_)



#define EXPECT_EXIT(statement, predicate, regex) \
  GTEST_DEATH_TEST_(statement, predicate, regex, GTEST_NONFATAL_FAILURE_)




#define ASSERT_DEATH(statement, regex) \
  ASSERT_EXIT(statement, ::testing::internal::ExitedUnsuccessfully, regex)



#define EXPECT_DEATH(statement, regex) \
  EXPECT_EXIT(statement, ::testing::internal::ExitedUnsuccessfully, regex)




class ExitedWithCode {
 public:
  explicit ExitedWithCode(int exit_code);
  bool operator()(int exit_status) const;
 private:
  const int exit_code_;
};

#if !GTEST_OS_WINDOWS


class KilledBySignal {
 public:
  explicit KilledBySignal(int signum);
  bool operator()(int exit_status) const;
 private:
  const int signum_;
};
#endif  












































#ifdef NDEBUG

#define EXPECT_DEBUG_DEATH(statement, regex) \
  do { statement; } while (false)

#define ASSERT_DEBUG_DEATH(statement, regex) \
  do { statement; } while (false)

#else

#define EXPECT_DEBUG_DEATH(statement, regex) \
  EXPECT_DEATH(statement, regex)

#define ASSERT_DEBUG_DEATH(statement, regex) \
  ASSERT_DEATH(statement, regex)

#endif  
#endif  
}  

#endif  
