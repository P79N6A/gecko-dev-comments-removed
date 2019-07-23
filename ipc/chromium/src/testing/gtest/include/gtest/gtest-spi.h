

































#ifndef GTEST_INCLUDE_GTEST_GTEST_SPI_H_
#define GTEST_INCLUDE_GTEST_GTEST_SPI_H_

#include <gtest/gtest.h>

namespace testing {










class ScopedFakeTestPartResultReporter
    : public TestPartResultReporterInterface {
 public:
  
  enum InterceptMode {
    INTERCEPT_ONLY_CURRENT_THREAD,  
    INTERCEPT_ALL_THREADS           
  };

  
  
  
  
  explicit ScopedFakeTestPartResultReporter(TestPartResultArray* result);

  
  ScopedFakeTestPartResultReporter(InterceptMode intercept_mode,
                                   TestPartResultArray* result);

  
  virtual ~ScopedFakeTestPartResultReporter();

  
  
  
  
  
  virtual void ReportTestPartResult(const TestPartResult& result);
 private:
  void Init();

  const InterceptMode intercept_mode_;
  TestPartResultReporterInterface* old_reporter_;
  TestPartResultArray* const result_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(ScopedFakeTestPartResultReporter);
};

namespace internal {






class SingleFailureChecker {
 public:
  
  SingleFailureChecker(const TestPartResultArray* results,
                       TestPartResultType type,
                       const char* substr);
  ~SingleFailureChecker();
 private:
  const TestPartResultArray* const results_;
  const TestPartResultType type_;
  const String substr_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(SingleFailureChecker);
};

}  

}  
























#define EXPECT_FATAL_FAILURE(statement, substr) \
  do { \
    class GTestExpectFatalFailureHelper {\
     public:\
      static void Execute() { statement; }\
    };\
    ::testing::TestPartResultArray gtest_failures;\
    ::testing::internal::SingleFailureChecker gtest_checker(\
        &gtest_failures, ::testing::TPRT_FATAL_FAILURE, (substr));\
    {\
      ::testing::ScopedFakeTestPartResultReporter gtest_reporter(\
          ::testing::ScopedFakeTestPartResultReporter:: \
          INTERCEPT_ONLY_CURRENT_THREAD, &gtest_failures);\
      GTestExpectFatalFailureHelper::Execute();\
    }\
  } while (false)

#define EXPECT_FATAL_FAILURE_ON_ALL_THREADS(statement, substr) \
  do { \
    class GTestExpectFatalFailureHelper {\
     public:\
      static void Execute() { statement; }\
    };\
    ::testing::TestPartResultArray gtest_failures;\
    ::testing::internal::SingleFailureChecker gtest_checker(\
        &gtest_failures, ::testing::TPRT_FATAL_FAILURE, (substr));\
    {\
      ::testing::ScopedFakeTestPartResultReporter gtest_reporter(\
          ::testing::ScopedFakeTestPartResultReporter:: \
          INTERCEPT_ALL_THREADS, &gtest_failures);\
      GTestExpectFatalFailureHelper::Execute();\
    }\
  } while (false)
























#define EXPECT_NONFATAL_FAILURE(statement, substr) \
  do {\
    ::testing::TestPartResultArray gtest_failures;\
    ::testing::internal::SingleFailureChecker gtest_checker(\
        &gtest_failures, ::testing::TPRT_NONFATAL_FAILURE, (substr));\
    {\
      ::testing::ScopedFakeTestPartResultReporter gtest_reporter(\
          ::testing::ScopedFakeTestPartResultReporter:: \
          INTERCEPT_ONLY_CURRENT_THREAD, &gtest_failures);\
      statement;\
    }\
  } while (false)

#define EXPECT_NONFATAL_FAILURE_ON_ALL_THREADS(statement, substr) \
  do {\
    ::testing::TestPartResultArray gtest_failures;\
    ::testing::internal::SingleFailureChecker gtest_checker(\
        &gtest_failures, ::testing::TPRT_NONFATAL_FAILURE, (substr));\
    {\
      ::testing::ScopedFakeTestPartResultReporter gtest_reporter(\
          ::testing::ScopedFakeTestPartResultReporter::INTERCEPT_ALL_THREADS,\
          &gtest_failures);\
      statement;\
    }\
  } while (false)

#endif
