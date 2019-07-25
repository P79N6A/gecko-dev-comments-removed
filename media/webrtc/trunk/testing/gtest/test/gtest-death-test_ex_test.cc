
































#include "gtest/gtest-death-test.h"
#include "gtest/gtest.h"

#if GTEST_HAS_DEATH_TEST

# if GTEST_HAS_SEH
#  include <windows.h>          
# endif

# include "gtest/gtest-spi.h"

# if GTEST_HAS_EXCEPTIONS

#  include <exception>  



TEST(CxxExceptionDeathTest, ExceptionIsFailure) {
  try {
    EXPECT_NONFATAL_FAILURE(EXPECT_DEATH(throw 1, ""), "threw an exception");
  } catch (...) {  
    FAIL() << "An exception escaped a death test macro invocation "
           << "with catch_exceptions "
           << (testing::GTEST_FLAG(catch_exceptions) ? "enabled" : "disabled");
  }
}

class TestException : public std::exception {
 public:
  virtual const char* what() const throw() { return "exceptional message"; }
};

TEST(CxxExceptionDeathTest, PrintsMessageForStdExceptions) {
  
  EXPECT_NONFATAL_FAILURE(EXPECT_DEATH(throw TestException(), ""),
                          "exceptional message");
  
  EXPECT_NONFATAL_FAILURE(EXPECT_DEATH(throw TestException(), ""),
                          "gtest-death-test_ex_test.cc");
}
# endif  

# if GTEST_HAS_SEH



TEST(SehExceptionDeasTest, CatchExceptionsDoesNotInterfere) {
  EXPECT_DEATH(RaiseException(42, 0x0, 0, NULL), "")
      << "with catch_exceptions "
      << (testing::GTEST_FLAG(catch_exceptions) ? "enabled" : "disabled");
}
# endif

#endif  

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::GTEST_FLAG(catch_exceptions) = GTEST_ENABLE_CATCH_EXCEPTIONS_ != 0;
  return RUN_ALL_TESTS();
}
