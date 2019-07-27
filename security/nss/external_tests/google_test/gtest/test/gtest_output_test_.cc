




































#include "gtest/gtest-spi.h"
#include "gtest/gtest.h"






#define GTEST_IMPLEMENTATION_ 1
#include "src/gtest-internal-inl.h"
#undef GTEST_IMPLEMENTATION_

#include <stdlib.h>

#if GTEST_IS_THREADSAFE
using testing::ScopedFakeTestPartResultReporter;
using testing::TestPartResultArray;

using testing::internal::Notification;
using testing::internal::ThreadWithParam;
#endif

namespace posix = ::testing::internal::posix;
using testing::internal::scoped_ptr;




void TestEq1(int x) {
  ASSERT_EQ(1, x);
}



void TryTestSubroutine() {
  
  TestEq1(2);

  
  
  
  
  if (testing::Test::HasFatalFailure()) return;

  
  FAIL() << "This should never be reached.";
}

TEST(PassingTest, PassingTest1) {
}

TEST(PassingTest, PassingTest2) {
}



class FailingParamTest : public testing::TestWithParam<int> {};

TEST_P(FailingParamTest, Fails) {
  EXPECT_EQ(1, GetParam());
}



INSTANTIATE_TEST_CASE_P(PrintingFailingParams,
                        FailingParamTest,
                        testing::Values(2));

static const char kGoldenString[] = "\"Line\0 1\"\nLine 2";

TEST(NonfatalFailureTest, EscapesStringOperands) {
  std::string actual = "actual \"string\"";
  EXPECT_EQ(kGoldenString, actual);

  const char* golden = kGoldenString;
  EXPECT_EQ(golden, actual);
}

TEST(NonfatalFailureTest, DiffForLongStrings) {
  std::string golden_str(kGoldenString, sizeof(kGoldenString) - 1);
  EXPECT_EQ(golden_str, "Line 2");
}


TEST(FatalFailureTest, FatalFailureInSubroutine) {
  printf("(expecting a failure that x should be 1)\n");

  TryTestSubroutine();
}


TEST(FatalFailureTest, FatalFailureInNestedSubroutine) {
  printf("(expecting a failure that x should be 1)\n");

  
  TryTestSubroutine();

  
  
  
  
  if (HasFatalFailure()) return;

  
  FAIL() << "This should never be reached.";
}


TEST(FatalFailureTest, NonfatalFailureInSubroutine) {
  printf("(expecting a failure on false)\n");
  EXPECT_TRUE(false);  
  ASSERT_FALSE(HasFatalFailure());  
}


TEST(LoggingTest, InterleavingLoggingAndAssertions) {
  static const int a[4] = {
    3, 9, 2, 6
  };

  printf("(expecting 2 failures on (3) >= (a[i]))\n");
  for (int i = 0; i < static_cast<int>(sizeof(a)/sizeof(*a)); i++) {
    printf("i == %d\n", i);
    EXPECT_GE(3, a[i]);
  }
}




void SubWithoutTrace(int n) {
  EXPECT_EQ(1, n);
  ASSERT_EQ(2, n);
}


void SubWithTrace(int n) {
  SCOPED_TRACE(testing::Message() << "n = " << n);

  SubWithoutTrace(n);
}


TEST(SCOPED_TRACETest, ObeysScopes) {
  printf("(expected to fail)\n");

  
  ADD_FAILURE() << "This failure is expected, and shouldn't have a trace.";

  {
    SCOPED_TRACE("Expected trace");
    
    
    ADD_FAILURE() << "This failure is expected, and should have a trace.";
  }

  
  
  ADD_FAILURE() << "This failure is expected, and shouldn't have a trace.";
}


TEST(SCOPED_TRACETest, WorksInLoop) {
  printf("(expected to fail)\n");

  for (int i = 1; i <= 2; i++) {
    SCOPED_TRACE(testing::Message() << "i = " << i);

    SubWithoutTrace(i);
  }
}


TEST(SCOPED_TRACETest, WorksInSubroutine) {
  printf("(expected to fail)\n");

  SubWithTrace(1);
  SubWithTrace(2);
}


TEST(SCOPED_TRACETest, CanBeNested) {
  printf("(expected to fail)\n");

  SCOPED_TRACE("");  

  SubWithTrace(2);
}


TEST(SCOPED_TRACETest, CanBeRepeated) {
  printf("(expected to fail)\n");

  SCOPED_TRACE("A");
  ADD_FAILURE()
      << "This failure is expected, and should contain trace point A.";

  SCOPED_TRACE("B");
  ADD_FAILURE()
      << "This failure is expected, and should contain trace point A and B.";

  {
    SCOPED_TRACE("C");
    ADD_FAILURE() << "This failure is expected, and should "
                  << "contain trace point A, B, and C.";
  }

  SCOPED_TRACE("D");
  ADD_FAILURE() << "This failure is expected, and should "
                << "contain trace point A, B, and D.";
}

#if GTEST_IS_THREADSAFE





























struct CheckPoints {
  Notification n1;
  Notification n2;
  Notification n3;
};

static void ThreadWithScopedTrace(CheckPoints* check_points) {
  {
    SCOPED_TRACE("Trace B");
    ADD_FAILURE()
        << "Expected failure #1 (in thread B, only trace B alive).";
    check_points->n1.Notify();
    check_points->n2.WaitForNotification();

    ADD_FAILURE()
        << "Expected failure #3 (in thread B, trace A & B both alive).";
  }  
  ADD_FAILURE()
      << "Expected failure #4 (in thread B, only trace A alive).";
  check_points->n3.Notify();
}

TEST(SCOPED_TRACETest, WorksConcurrently) {
  printf("(expecting 6 failures)\n");

  CheckPoints check_points;
  ThreadWithParam<CheckPoints*> thread(&ThreadWithScopedTrace,
                                       &check_points,
                                       NULL);
  check_points.n1.WaitForNotification();

  {
    SCOPED_TRACE("Trace A");
    ADD_FAILURE()
        << "Expected failure #2 (in thread A, trace A & B both alive).";
    check_points.n2.Notify();
    check_points.n3.WaitForNotification();

    ADD_FAILURE()
        << "Expected failure #5 (in thread A, only trace A alive).";
  }  
  ADD_FAILURE()
      << "Expected failure #6 (in thread A, no trace alive).";
  thread.Join();
}
#endif  

TEST(DisabledTestsWarningTest,
     DISABLED_AlsoRunDisabledTestsFlagSuppressesWarning) {
  
  
  
  
}




void AdHocTest() {
  printf("The non-test part of the code is expected to have 2 failures.\n\n");
  EXPECT_TRUE(false);
  EXPECT_EQ(2, 3);
}


int RunAllTests() {
  AdHocTest();
  return RUN_ALL_TESTS();
}


class NonFatalFailureInFixtureConstructorTest : public testing::Test {
 protected:
  NonFatalFailureInFixtureConstructorTest() {
    printf("(expecting 5 failures)\n");
    ADD_FAILURE() << "Expected failure #1, in the test fixture c'tor.";
  }

  ~NonFatalFailureInFixtureConstructorTest() {
    ADD_FAILURE() << "Expected failure #5, in the test fixture d'tor.";
  }

  virtual void SetUp() {
    ADD_FAILURE() << "Expected failure #2, in SetUp().";
  }

  virtual void TearDown() {
    ADD_FAILURE() << "Expected failure #4, in TearDown.";
  }
};

TEST_F(NonFatalFailureInFixtureConstructorTest, FailureInConstructor) {
  ADD_FAILURE() << "Expected failure #3, in the test body.";
}


class FatalFailureInFixtureConstructorTest : public testing::Test {
 protected:
  FatalFailureInFixtureConstructorTest() {
    printf("(expecting 2 failures)\n");
    Init();
  }

  ~FatalFailureInFixtureConstructorTest() {
    ADD_FAILURE() << "Expected failure #2, in the test fixture d'tor.";
  }

  virtual void SetUp() {
    ADD_FAILURE() << "UNEXPECTED failure in SetUp().  "
                  << "We should never get here, as the test fixture c'tor "
                  << "had a fatal failure.";
  }

  virtual void TearDown() {
    ADD_FAILURE() << "UNEXPECTED failure in TearDown().  "
                  << "We should never get here, as the test fixture c'tor "
                  << "had a fatal failure.";
  }

 private:
  void Init() {
    FAIL() << "Expected failure #1, in the test fixture c'tor.";
  }
};

TEST_F(FatalFailureInFixtureConstructorTest, FailureInConstructor) {
  ADD_FAILURE() << "UNEXPECTED failure in the test body.  "
                << "We should never get here, as the test fixture c'tor "
                << "had a fatal failure.";
}


class NonFatalFailureInSetUpTest : public testing::Test {
 protected:
  virtual ~NonFatalFailureInSetUpTest() {
    Deinit();
  }

  virtual void SetUp() {
    printf("(expecting 4 failures)\n");
    ADD_FAILURE() << "Expected failure #1, in SetUp().";
  }

  virtual void TearDown() {
    FAIL() << "Expected failure #3, in TearDown().";
  }
 private:
  void Deinit() {
    FAIL() << "Expected failure #4, in the test fixture d'tor.";
  }
};

TEST_F(NonFatalFailureInSetUpTest, FailureInSetUp) {
  FAIL() << "Expected failure #2, in the test function.";
}


class FatalFailureInSetUpTest : public testing::Test {
 protected:
  virtual ~FatalFailureInSetUpTest() {
    Deinit();
  }

  virtual void SetUp() {
    printf("(expecting 3 failures)\n");
    FAIL() << "Expected failure #1, in SetUp().";
  }

  virtual void TearDown() {
    FAIL() << "Expected failure #2, in TearDown().";
  }
 private:
  void Deinit() {
    FAIL() << "Expected failure #3, in the test fixture d'tor.";
  }
};

TEST_F(FatalFailureInSetUpTest, FailureInSetUp) {
  FAIL() << "UNEXPECTED failure in the test function.  "
         << "We should never get here, as SetUp() failed.";
}

TEST(AddFailureAtTest, MessageContainsSpecifiedFileAndLineNumber) {
  ADD_FAILURE_AT("foo.cc", 42) << "Expected failure in foo.cc";
}

#if GTEST_IS_THREADSAFE


void DieIf(bool should_die) {
  GTEST_CHECK_(!should_die) << " - death inside DieIf().";
}




struct SpawnThreadNotifications {
  SpawnThreadNotifications() {}

  Notification spawn_thread_started;
  Notification spawn_thread_ok_to_terminate;

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(SpawnThreadNotifications);
};



static void ThreadRoutine(SpawnThreadNotifications* notifications) {
  
  notifications->spawn_thread_started.Notify();

  
  notifications->spawn_thread_ok_to_terminate.WaitForNotification();
}




class DeathTestAndMultiThreadsTest : public testing::Test {
 protected:
  
  virtual void SetUp() {
    thread_.reset(new ThreadWithParam<SpawnThreadNotifications*>(
        &ThreadRoutine, &notifications_, NULL));
    notifications_.spawn_thread_started.WaitForNotification();
  }
  
  
  
  
  
  virtual void TearDown() {
    notifications_.spawn_thread_ok_to_terminate.Notify();
  }

 private:
  SpawnThreadNotifications notifications_;
  scoped_ptr<ThreadWithParam<SpawnThreadNotifications*> > thread_;
};

#endif  











namespace foo {

class MixedUpTestCaseTest : public testing::Test {
};

TEST_F(MixedUpTestCaseTest, FirstTestFromNamespaceFoo) {}
TEST_F(MixedUpTestCaseTest, SecondTestFromNamespaceFoo) {}

class MixedUpTestCaseWithSameTestNameTest : public testing::Test {
};

TEST_F(MixedUpTestCaseWithSameTestNameTest,
       TheSecondTestWithThisNameShouldFail) {}

}  

namespace bar {

class MixedUpTestCaseTest : public testing::Test {
};



TEST_F(MixedUpTestCaseTest, ThisShouldFail) {}
TEST_F(MixedUpTestCaseTest, ThisShouldFailToo) {}

class MixedUpTestCaseWithSameTestNameTest : public testing::Test {
};



TEST_F(MixedUpTestCaseWithSameTestNameTest,
       TheSecondTestWithThisNameShouldFail) {}

}  






class TEST_F_before_TEST_in_same_test_case : public testing::Test {
};

TEST_F(TEST_F_before_TEST_in_same_test_case, DefinedUsingTEST_F) {}



TEST(TEST_F_before_TEST_in_same_test_case, DefinedUsingTESTAndShouldFail) {}

class TEST_before_TEST_F_in_same_test_case : public testing::Test {
};

TEST(TEST_before_TEST_F_in_same_test_case, DefinedUsingTEST) {}



TEST_F(TEST_before_TEST_F_in_same_test_case, DefinedUsingTEST_FAndShouldFail) {
}


int global_integer = 0;


TEST(ExpectNonfatalFailureTest, CanReferenceGlobalVariables) {
  global_integer = 0;
  EXPECT_NONFATAL_FAILURE({
    EXPECT_EQ(1, global_integer) << "Expected non-fatal failure.";
  }, "Expected non-fatal failure.");
}



TEST(ExpectNonfatalFailureTest, CanReferenceLocalVariables) {
  int m = 0;
  static int n;
  n = 1;
  EXPECT_NONFATAL_FAILURE({
    EXPECT_EQ(m, n) << "Expected non-fatal failure.";
  }, "Expected non-fatal failure.");
}



TEST(ExpectNonfatalFailureTest, SucceedsWhenThereIsOneNonfatalFailure) {
  EXPECT_NONFATAL_FAILURE({
    ADD_FAILURE() << "Expected non-fatal failure.";
  }, "Expected non-fatal failure.");
}



TEST(ExpectNonfatalFailureTest, FailsWhenThereIsNoNonfatalFailure) {
  printf("(expecting a failure)\n");
  EXPECT_NONFATAL_FAILURE({
  }, "");
}



TEST(ExpectNonfatalFailureTest, FailsWhenThereAreTwoNonfatalFailures) {
  printf("(expecting a failure)\n");
  EXPECT_NONFATAL_FAILURE({
    ADD_FAILURE() << "Expected non-fatal failure 1.";
    ADD_FAILURE() << "Expected non-fatal failure 2.";
  }, "");
}



TEST(ExpectNonfatalFailureTest, FailsWhenThereIsOneFatalFailure) {
  printf("(expecting a failure)\n");
  EXPECT_NONFATAL_FAILURE({
    FAIL() << "Expected fatal failure.";
  }, "");
}



TEST(ExpectNonfatalFailureTest, FailsWhenStatementReturns) {
  printf("(expecting a failure)\n");
  EXPECT_NONFATAL_FAILURE({
    return;
  }, "");
}

#if GTEST_HAS_EXCEPTIONS



TEST(ExpectNonfatalFailureTest, FailsWhenStatementThrows) {
  printf("(expecting a failure)\n");
  try {
    EXPECT_NONFATAL_FAILURE({
      throw 0;
    }, "");
  } catch(int) {  
  }
}

#endif  


TEST(ExpectFatalFailureTest, CanReferenceGlobalVariables) {
  global_integer = 0;
  EXPECT_FATAL_FAILURE({
    ASSERT_EQ(1, global_integer) << "Expected fatal failure.";
  }, "Expected fatal failure.");
}



TEST(ExpectFatalFailureTest, CanReferenceLocalStaticVariables) {
  static int n;
  n = 1;
  EXPECT_FATAL_FAILURE({
    ASSERT_EQ(0, n) << "Expected fatal failure.";
  }, "Expected fatal failure.");
}



TEST(ExpectFatalFailureTest, SucceedsWhenThereIsOneFatalFailure) {
  EXPECT_FATAL_FAILURE({
    FAIL() << "Expected fatal failure.";
  }, "Expected fatal failure.");
}



TEST(ExpectFatalFailureTest, FailsWhenThereIsNoFatalFailure) {
  printf("(expecting a failure)\n");
  EXPECT_FATAL_FAILURE({
  }, "");
}


void FatalFailure() {
  FAIL() << "Expected fatal failure.";
}



TEST(ExpectFatalFailureTest, FailsWhenThereAreTwoFatalFailures) {
  printf("(expecting a failure)\n");
  EXPECT_FATAL_FAILURE({
    FatalFailure();
    FatalFailure();
  }, "");
}



TEST(ExpectFatalFailureTest, FailsWhenThereIsOneNonfatalFailure) {
  printf("(expecting a failure)\n");
  EXPECT_FATAL_FAILURE({
    ADD_FAILURE() << "Expected non-fatal failure.";
  }, "");
}



TEST(ExpectFatalFailureTest, FailsWhenStatementReturns) {
  printf("(expecting a failure)\n");
  EXPECT_FATAL_FAILURE({
    return;
  }, "");
}

#if GTEST_HAS_EXCEPTIONS



TEST(ExpectFatalFailureTest, FailsWhenStatementThrows) {
  printf("(expecting a failure)\n");
  try {
    EXPECT_FATAL_FAILURE({
      throw 0;
    }, "");
  } catch(int) {  
  }
}

#endif  


#if GTEST_HAS_TYPED_TEST

template <typename T>
class TypedTest : public testing::Test {
};

TYPED_TEST_CASE(TypedTest, testing::Types<int>);

TYPED_TEST(TypedTest, Success) {
  EXPECT_EQ(0, TypeParam());
}

TYPED_TEST(TypedTest, Failure) {
  EXPECT_EQ(1, TypeParam()) << "Expected failure";
}

#endif  


#if GTEST_HAS_TYPED_TEST_P

template <typename T>
class TypedTestP : public testing::Test {
};

TYPED_TEST_CASE_P(TypedTestP);

TYPED_TEST_P(TypedTestP, Success) {
  EXPECT_EQ(0U, TypeParam());
}

TYPED_TEST_P(TypedTestP, Failure) {
  EXPECT_EQ(1U, TypeParam()) << "Expected failure";
}

REGISTER_TYPED_TEST_CASE_P(TypedTestP, Success, Failure);

typedef testing::Types<unsigned char, unsigned int> UnsignedTypes;
INSTANTIATE_TYPED_TEST_CASE_P(Unsigned, TypedTestP, UnsignedTypes);

#endif  

#if GTEST_HAS_DEATH_TEST




TEST(ADeathTest, ShouldRunFirst) {
}

# if GTEST_HAS_TYPED_TEST




template <typename T>
class ATypedDeathTest : public testing::Test {
};

typedef testing::Types<int, double> NumericTypes;
TYPED_TEST_CASE(ATypedDeathTest, NumericTypes);

TYPED_TEST(ATypedDeathTest, ShouldRunFirst) {
}

# endif  

# if GTEST_HAS_TYPED_TEST_P





template <typename T>
class ATypeParamDeathTest : public testing::Test {
};

TYPED_TEST_CASE_P(ATypeParamDeathTest);

TYPED_TEST_P(ATypeParamDeathTest, ShouldRunFirst) {
}

REGISTER_TYPED_TEST_CASE_P(ATypeParamDeathTest, ShouldRunFirst);

INSTANTIATE_TYPED_TEST_CASE_P(My, ATypeParamDeathTest, NumericTypes);

# endif  

#endif  



class ExpectFailureTest : public testing::Test {
 public:  
  enum FailureMode {
    FATAL_FAILURE,
    NONFATAL_FAILURE
  };
  static void AddFailure(FailureMode failure) {
    if (failure == FATAL_FAILURE) {
      FAIL() << "Expected fatal failure.";
    } else {
      ADD_FAILURE() << "Expected non-fatal failure.";
    }
  }
};

TEST_F(ExpectFailureTest, ExpectFatalFailure) {
  
  printf("(expecting 1 failure)\n");
  EXPECT_FATAL_FAILURE(SUCCEED(), "Expected fatal failure.");
  
  printf("(expecting 1 failure)\n");
  EXPECT_FATAL_FAILURE(AddFailure(NONFATAL_FAILURE), "Expected non-fatal "
                       "failure.");
  
  printf("(expecting 1 failure)\n");
  EXPECT_FATAL_FAILURE(AddFailure(FATAL_FAILURE), "Some other fatal failure "
                       "expected.");
}

TEST_F(ExpectFailureTest, ExpectNonFatalFailure) {
  
  printf("(expecting 1 failure)\n");
  EXPECT_NONFATAL_FAILURE(SUCCEED(), "Expected non-fatal failure.");
  
  printf("(expecting 1 failure)\n");
  EXPECT_NONFATAL_FAILURE(AddFailure(FATAL_FAILURE), "Expected fatal failure.");
  
  printf("(expecting 1 failure)\n");
  EXPECT_NONFATAL_FAILURE(AddFailure(NONFATAL_FAILURE), "Some other non-fatal "
                          "failure.");
}

#if GTEST_IS_THREADSAFE

class ExpectFailureWithThreadsTest : public ExpectFailureTest {
 protected:
  static void AddFailureInOtherThread(FailureMode failure) {
    ThreadWithParam<FailureMode> thread(&AddFailure, failure, NULL);
    thread.Join();
  }
};

TEST_F(ExpectFailureWithThreadsTest, ExpectFatalFailure) {
  
  printf("(expecting 2 failures)\n");
  EXPECT_FATAL_FAILURE(AddFailureInOtherThread(FATAL_FAILURE),
                       "Expected fatal failure.");
}

TEST_F(ExpectFailureWithThreadsTest, ExpectNonFatalFailure) {
  
  printf("(expecting 2 failures)\n");
  EXPECT_NONFATAL_FAILURE(AddFailureInOtherThread(NONFATAL_FAILURE),
                          "Expected non-fatal failure.");
}

typedef ExpectFailureWithThreadsTest ScopedFakeTestPartResultReporterTest;



TEST_F(ScopedFakeTestPartResultReporterTest, InterceptOnlyCurrentThread) {
  printf("(expecting 2 failures)\n");
  TestPartResultArray results;
  {
    ScopedFakeTestPartResultReporter reporter(
        ScopedFakeTestPartResultReporter::INTERCEPT_ONLY_CURRENT_THREAD,
        &results);
    AddFailureInOtherThread(FATAL_FAILURE);
    AddFailureInOtherThread(NONFATAL_FAILURE);
  }
  
  EXPECT_EQ(0, results.size()) << "This shouldn't fail.";
}

#endif  

TEST_F(ExpectFailureTest, ExpectFatalFailureOnAllThreads) {
  
  printf("(expecting 1 failure)\n");
  EXPECT_FATAL_FAILURE_ON_ALL_THREADS(SUCCEED(), "Expected fatal failure.");
  
  printf("(expecting 1 failure)\n");
  EXPECT_FATAL_FAILURE_ON_ALL_THREADS(AddFailure(NONFATAL_FAILURE),
                                      "Expected non-fatal failure.");
  
  printf("(expecting 1 failure)\n");
  EXPECT_FATAL_FAILURE_ON_ALL_THREADS(AddFailure(FATAL_FAILURE),
                                      "Some other fatal failure expected.");
}

TEST_F(ExpectFailureTest, ExpectNonFatalFailureOnAllThreads) {
  
  printf("(expecting 1 failure)\n");
  EXPECT_NONFATAL_FAILURE_ON_ALL_THREADS(SUCCEED(), "Expected non-fatal "
                                         "failure.");
  
  printf("(expecting 1 failure)\n");
  EXPECT_NONFATAL_FAILURE_ON_ALL_THREADS(AddFailure(FATAL_FAILURE),
                                         "Expected fatal failure.");
  
  printf("(expecting 1 failure)\n");
  EXPECT_NONFATAL_FAILURE_ON_ALL_THREADS(AddFailure(NONFATAL_FAILURE),
                                         "Some other non-fatal failure.");
}




class FooEnvironment : public testing::Environment {
 public:
  virtual void SetUp() {
    printf("%s", "FooEnvironment::SetUp() called.\n");
  }

  virtual void TearDown() {
    printf("%s", "FooEnvironment::TearDown() called.\n");
    FAIL() << "Expected fatal failure.";
  }
};

class BarEnvironment : public testing::Environment {
 public:
  virtual void SetUp() {
    printf("%s", "BarEnvironment::SetUp() called.\n");
  }

  virtual void TearDown() {
    printf("%s", "BarEnvironment::TearDown() called.\n");
    ADD_FAILURE() << "Expected non-fatal failure.";
  }
};

bool GTEST_FLAG(internal_skip_environment_and_ad_hoc_tests) = false;






int main(int argc, char **argv) {
  testing::GTEST_FLAG(print_time) = false;

  
  
  

  
  
  
  testing::InitGoogleTest(&argc, argv);
  if (argc >= 2 &&
      (std::string(argv[1]) ==
       "--gtest_internal_skip_environment_and_ad_hoc_tests"))
    GTEST_FLAG(internal_skip_environment_and_ad_hoc_tests) = true;

#if GTEST_HAS_DEATH_TEST
  if (testing::internal::GTEST_FLAG(internal_run_death_test) != "") {
    
    
# if GTEST_OS_WINDOWS
    posix::FReopen("nul:", "w", stdout);
# else
    posix::FReopen("/dev/null", "w", stdout);
# endif  
    return RUN_ALL_TESTS();
  }
#endif  

  if (GTEST_FLAG(internal_skip_environment_and_ad_hoc_tests))
    return RUN_ALL_TESTS();

  
  
  
  testing::AddGlobalTestEnvironment(new FooEnvironment);
  testing::AddGlobalTestEnvironment(new BarEnvironment);

  return RunAllTests();
}
