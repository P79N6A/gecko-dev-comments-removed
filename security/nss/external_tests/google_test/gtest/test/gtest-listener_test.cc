


































#include "gtest/gtest.h"
#include <vector>

using ::testing::AddGlobalTestEnvironment;
using ::testing::Environment;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestCase;
using ::testing::TestEventListener;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;


std::vector<std::string>* g_events = NULL;

namespace testing {
namespace internal {

class EventRecordingListener : public TestEventListener {
 public:
  explicit EventRecordingListener(const char* name) : name_(name) {}

 protected:
  virtual void OnTestProgramStart(const UnitTest& ) {
    g_events->push_back(GetFullMethodName("OnTestProgramStart"));
  }

  virtual void OnTestIterationStart(const UnitTest& ,
                                    int iteration) {
    Message message;
    message << GetFullMethodName("OnTestIterationStart")
            << "(" << iteration << ")";
    g_events->push_back(message.GetString());
  }

  virtual void OnEnvironmentsSetUpStart(const UnitTest& ) {
    g_events->push_back(GetFullMethodName("OnEnvironmentsSetUpStart"));
  }

  virtual void OnEnvironmentsSetUpEnd(const UnitTest& ) {
    g_events->push_back(GetFullMethodName("OnEnvironmentsSetUpEnd"));
  }

  virtual void OnTestCaseStart(const TestCase& ) {
    g_events->push_back(GetFullMethodName("OnTestCaseStart"));
  }

  virtual void OnTestStart(const TestInfo& ) {
    g_events->push_back(GetFullMethodName("OnTestStart"));
  }

  virtual void OnTestPartResult(const TestPartResult& ) {
    g_events->push_back(GetFullMethodName("OnTestPartResult"));
  }

  virtual void OnTestEnd(const TestInfo& ) {
    g_events->push_back(GetFullMethodName("OnTestEnd"));
  }

  virtual void OnTestCaseEnd(const TestCase& ) {
    g_events->push_back(GetFullMethodName("OnTestCaseEnd"));
  }

  virtual void OnEnvironmentsTearDownStart(const UnitTest& ) {
    g_events->push_back(GetFullMethodName("OnEnvironmentsTearDownStart"));
  }

  virtual void OnEnvironmentsTearDownEnd(const UnitTest& ) {
    g_events->push_back(GetFullMethodName("OnEnvironmentsTearDownEnd"));
  }

  virtual void OnTestIterationEnd(const UnitTest& ,
                                  int iteration) {
    Message message;
    message << GetFullMethodName("OnTestIterationEnd")
            << "("  << iteration << ")";
    g_events->push_back(message.GetString());
  }

  virtual void OnTestProgramEnd(const UnitTest& ) {
    g_events->push_back(GetFullMethodName("OnTestProgramEnd"));
  }

 private:
  std::string GetFullMethodName(const char* name) {
    return name_ + "." + name;
  }

  std::string name_;
};

class EnvironmentInvocationCatcher : public Environment {
 protected:
  virtual void SetUp() {
    g_events->push_back("Environment::SetUp");
  }

  virtual void TearDown() {
    g_events->push_back("Environment::TearDown");
  }
};

class ListenerTest : public Test {
 protected:
  static void SetUpTestCase() {
    g_events->push_back("ListenerTest::SetUpTestCase");
  }

  static void TearDownTestCase() {
    g_events->push_back("ListenerTest::TearDownTestCase");
  }

  virtual void SetUp() {
    g_events->push_back("ListenerTest::SetUp");
  }

  virtual void TearDown() {
    g_events->push_back("ListenerTest::TearDown");
  }
};

TEST_F(ListenerTest, DoesFoo) {
  
  
  g_events->push_back("ListenerTest::* Test Body");
  SUCCEED();  
}

TEST_F(ListenerTest, DoesBar) {
  g_events->push_back("ListenerTest::* Test Body");
  SUCCEED();  
}

}  

}  

using ::testing::internal::EnvironmentInvocationCatcher;
using ::testing::internal::EventRecordingListener;

void VerifyResults(const std::vector<std::string>& data,
                   const char* const* expected_data,
                   int expected_data_size) {
  const int actual_size = data.size();
  
  
  EXPECT_EQ(expected_data_size, actual_size);

  
  const int shorter_size = expected_data_size <= actual_size ?
      expected_data_size : actual_size;
  int i = 0;
  for (; i < shorter_size; ++i) {
    ASSERT_STREQ(expected_data[i], data[i].c_str())
        << "at position " << i;
  }

  
  for (; i < actual_size; ++i) {
    printf("  Actual event #%d: %s\n", i, data[i].c_str());
  }
}

int main(int argc, char **argv) {
  std::vector<std::string> events;
  g_events = &events;
  InitGoogleTest(&argc, argv);

  UnitTest::GetInstance()->listeners().Append(
      new EventRecordingListener("1st"));
  UnitTest::GetInstance()->listeners().Append(
      new EventRecordingListener("2nd"));

  AddGlobalTestEnvironment(new EnvironmentInvocationCatcher);

  GTEST_CHECK_(events.size() == 0)
      << "AddGlobalTestEnvironment should not generate any events itself.";

  ::testing::GTEST_FLAG(repeat) = 2;
  int ret_val = RUN_ALL_TESTS();

  const char* const expected_events[] = {
    "1st.OnTestProgramStart",
    "2nd.OnTestProgramStart",
    "1st.OnTestIterationStart(0)",
    "2nd.OnTestIterationStart(0)",
    "1st.OnEnvironmentsSetUpStart",
    "2nd.OnEnvironmentsSetUpStart",
    "Environment::SetUp",
    "2nd.OnEnvironmentsSetUpEnd",
    "1st.OnEnvironmentsSetUpEnd",
    "1st.OnTestCaseStart",
    "2nd.OnTestCaseStart",
    "ListenerTest::SetUpTestCase",
    "1st.OnTestStart",
    "2nd.OnTestStart",
    "ListenerTest::SetUp",
    "ListenerTest::* Test Body",
    "1st.OnTestPartResult",
    "2nd.OnTestPartResult",
    "ListenerTest::TearDown",
    "2nd.OnTestEnd",
    "1st.OnTestEnd",
    "1st.OnTestStart",
    "2nd.OnTestStart",
    "ListenerTest::SetUp",
    "ListenerTest::* Test Body",
    "1st.OnTestPartResult",
    "2nd.OnTestPartResult",
    "ListenerTest::TearDown",
    "2nd.OnTestEnd",
    "1st.OnTestEnd",
    "ListenerTest::TearDownTestCase",
    "2nd.OnTestCaseEnd",
    "1st.OnTestCaseEnd",
    "1st.OnEnvironmentsTearDownStart",
    "2nd.OnEnvironmentsTearDownStart",
    "Environment::TearDown",
    "2nd.OnEnvironmentsTearDownEnd",
    "1st.OnEnvironmentsTearDownEnd",
    "2nd.OnTestIterationEnd(0)",
    "1st.OnTestIterationEnd(0)",
    "1st.OnTestIterationStart(1)",
    "2nd.OnTestIterationStart(1)",
    "1st.OnEnvironmentsSetUpStart",
    "2nd.OnEnvironmentsSetUpStart",
    "Environment::SetUp",
    "2nd.OnEnvironmentsSetUpEnd",
    "1st.OnEnvironmentsSetUpEnd",
    "1st.OnTestCaseStart",
    "2nd.OnTestCaseStart",
    "ListenerTest::SetUpTestCase",
    "1st.OnTestStart",
    "2nd.OnTestStart",
    "ListenerTest::SetUp",
    "ListenerTest::* Test Body",
    "1st.OnTestPartResult",
    "2nd.OnTestPartResult",
    "ListenerTest::TearDown",
    "2nd.OnTestEnd",
    "1st.OnTestEnd",
    "1st.OnTestStart",
    "2nd.OnTestStart",
    "ListenerTest::SetUp",
    "ListenerTest::* Test Body",
    "1st.OnTestPartResult",
    "2nd.OnTestPartResult",
    "ListenerTest::TearDown",
    "2nd.OnTestEnd",
    "1st.OnTestEnd",
    "ListenerTest::TearDownTestCase",
    "2nd.OnTestCaseEnd",
    "1st.OnTestCaseEnd",
    "1st.OnEnvironmentsTearDownStart",
    "2nd.OnEnvironmentsTearDownStart",
    "Environment::TearDown",
    "2nd.OnEnvironmentsTearDownEnd",
    "1st.OnEnvironmentsTearDownEnd",
    "2nd.OnTestIterationEnd(1)",
    "1st.OnTestIterationEnd(1)",
    "2nd.OnTestProgramEnd",
    "1st.OnTestProgramEnd"
  };
  VerifyResults(events,
                expected_events,
                sizeof(expected_events)/sizeof(expected_events[0]));

  
  
  if (UnitTest::GetInstance()->Failed())
    ret_val = 1;

  return ret_val;
}
