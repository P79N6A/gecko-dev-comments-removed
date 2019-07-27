

































#include "gtest/gtest.h"

#include <iostream>
#include <vector>




#define GTEST_IMPLEMENTATION_ 1
#include "src/gtest-internal-inl.h"
#undef GTEST_IMPLEMENTATION_

#if GTEST_IS_THREADSAFE

namespace testing {
namespace {

using internal::Notification;
using internal::TestPropertyKeyIs;
using internal::ThreadWithParam;
using internal::scoped_ptr;






const int kThreadCount = 50;

std::string IdToKey(int id, const char* suffix) {
  Message key;
  key << "key_" << id << "_" << suffix;
  return key.GetString();
}

std::string IdToString(int id) {
  Message id_message;
  id_message << id;
  return id_message.GetString();
}

void ExpectKeyAndValueWereRecordedForId(
    const std::vector<TestProperty>& properties,
    int id, const char* suffix) {
  TestPropertyKeyIs matches_key(IdToKey(id, suffix).c_str());
  const std::vector<TestProperty>::const_iterator property =
      std::find_if(properties.begin(), properties.end(), matches_key);
  ASSERT_TRUE(property != properties.end())
      << "expecting " << suffix << " value for id " << id;
  EXPECT_STREQ(IdToString(id).c_str(), property->value());
}



void ManyAsserts(int id) {
  GTEST_LOG_(INFO) << "Thread #" << id << " running...";

  SCOPED_TRACE(Message() << "Thread #" << id);

  for (int i = 0; i < kThreadCount; i++) {
    SCOPED_TRACE(Message() << "Iteration #" << i);

    
    EXPECT_TRUE(true);
    ASSERT_FALSE(false) << "This shouldn't fail.";
    EXPECT_STREQ("a", "a");
    ASSERT_LE(5, 6);
    EXPECT_EQ(i, i) << "This shouldn't fail.";

    
    
    Test::RecordProperty(IdToKey(id, "string").c_str(), IdToString(id).c_str());
    Test::RecordProperty(IdToKey(id, "int").c_str(), id);
    Test::RecordProperty("shared_key", IdToString(id).c_str());

    
    
    
    EXPECT_LT(i, 0) << "This should always fail.";
  }
}

void CheckTestFailureCount(int expected_failures) {
  const TestInfo* const info = UnitTest::GetInstance()->current_test_info();
  const TestResult* const result = info->result();
  GTEST_CHECK_(expected_failures == result->total_part_count())
      << "Logged " << result->total_part_count() << " failures "
      << " vs. " << expected_failures << " expected";
}



TEST(StressTest, CanUseScopedTraceAndAssertionsInManyThreads) {
  {
    scoped_ptr<ThreadWithParam<int> > threads[kThreadCount];
    Notification threads_can_start;
    for (int i = 0; i != kThreadCount; i++)
      threads[i].reset(new ThreadWithParam<int>(&ManyAsserts,
                                                i,
                                                &threads_can_start));

    threads_can_start.Notify();

    
    for (int i = 0; i != kThreadCount; i++)
      threads[i]->Join();
  }

  
  const TestInfo* const info = UnitTest::GetInstance()->current_test_info();
  const TestResult* const result = info->result();

  std::vector<TestProperty> properties;
  
  
  for (int i = 0; i < result->test_property_count(); ++i)
    properties.push_back(result->GetTestProperty(i));

  EXPECT_EQ(kThreadCount * 2 + 1, result->test_property_count())
      << "String and int values recorded on each thread, "
      << "as well as one shared_key";
  for (int i = 0; i < kThreadCount; ++i) {
    ExpectKeyAndValueWereRecordedForId(properties, i, "string");
    ExpectKeyAndValueWereRecordedForId(properties, i, "int");
  }
  CheckTestFailureCount(kThreadCount*kThreadCount);
}

void FailingThread(bool is_fatal) {
  if (is_fatal)
    FAIL() << "Fatal failure in some other thread. "
           << "(This failure is expected.)";
  else
    ADD_FAILURE() << "Non-fatal failure in some other thread. "
                  << "(This failure is expected.)";
}

void GenerateFatalFailureInAnotherThread(bool is_fatal) {
  ThreadWithParam<bool> thread(&FailingThread, is_fatal, NULL);
  thread.Join();
}

TEST(NoFatalFailureTest, ExpectNoFatalFailureIgnoresFailuresInOtherThreads) {
  EXPECT_NO_FATAL_FAILURE(GenerateFatalFailureInAnotherThread(true));
  
  
  
  CheckTestFailureCount(1);
}

void AssertNoFatalFailureIgnoresFailuresInOtherThreads() {
  ASSERT_NO_FATAL_FAILURE(GenerateFatalFailureInAnotherThread(true));
}
TEST(NoFatalFailureTest, AssertNoFatalFailureIgnoresFailuresInOtherThreads) {
  
  AssertNoFatalFailureIgnoresFailuresInOtherThreads();
  
  
  
  CheckTestFailureCount(1);
}

TEST(FatalFailureTest, ExpectFatalFailureIgnoresFailuresInOtherThreads) {
  
  
  EXPECT_FATAL_FAILURE(GenerateFatalFailureInAnotherThread(true), "expected");
  CheckTestFailureCount(2);
}

TEST(FatalFailureOnAllThreadsTest, ExpectFatalFailureOnAllThreads) {
  
  
  EXPECT_FATAL_FAILURE_ON_ALL_THREADS(
      GenerateFatalFailureInAnotherThread(true), "expected");
  CheckTestFailureCount(0);
  
  
  ADD_FAILURE() << "This is an expected non-fatal failure.";
}

TEST(NonFatalFailureTest, ExpectNonFatalFailureIgnoresFailuresInOtherThreads) {
  
  
  EXPECT_NONFATAL_FAILURE(GenerateFatalFailureInAnotherThread(false),
                          "expected");
  CheckTestFailureCount(2);
}

TEST(NonFatalFailureOnAllThreadsTest, ExpectNonFatalFailureOnAllThreads) {
  
  
  EXPECT_NONFATAL_FAILURE_ON_ALL_THREADS(
      GenerateFatalFailureInAnotherThread(false), "expected");
  CheckTestFailureCount(0);
  
  
  ADD_FAILURE() << "This is an expected non-fatal failure.";
}

}  
}  

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  const int result = RUN_ALL_TESTS();  
  GTEST_CHECK_(result == 1) << "RUN_ALL_TESTS() did not fail as expected";

  printf("\nPASS\n");
  return 0;
}

#else
TEST(StressTest,
     DISABLED_ThreadSafetyTestsAreSkippedWhenGoogleTestIsNotThreadSafe) {
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#endif  
