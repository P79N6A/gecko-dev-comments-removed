



#include "base/strings/stringprintf.h"
#include "sandbox/win/src/handle_policy.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/sandbox_policy.h"
#include "sandbox/win/src/win_utils.h"
#include "sandbox/win/tests/common/controller.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {


SBOX_TESTS_COMMAND int Handle_WaitProcess(int argc, wchar_t **argv) {
  if (argc != 1)
    return SBOX_TEST_FAILED_TO_EXECUTE_COMMAND;

  ::Sleep(::wcstoul(argv[0], NULL, 10));
  return SBOX_TEST_TIMED_OUT;
}


SBOX_TESTS_COMMAND int Handle_DuplicateEvent(int argc, wchar_t **argv) {
  if (argc != 1)
    return SBOX_TEST_FAILED_TO_EXECUTE_COMMAND;

  
  base::win::ScopedHandle test_event;
  test_event.Set(::CreateEvent(NULL, TRUE, TRUE, NULL));
  if (!test_event.IsValid())
    return SBOX_TEST_FIRST_ERROR;

  
  DWORD target_process_id = ::wcstoul(argv[0], NULL, 10);

  HANDLE handle = NULL;
  ResultCode result = SandboxFactory::GetTargetServices()->DuplicateHandle(
      test_event.Get(), target_process_id, &handle, 0, DUPLICATE_SAME_ACCESS);

  return (result == SBOX_ALL_OK) ? SBOX_TEST_SUCCEEDED : SBOX_TEST_DENIED;
}


TEST(HandlePolicyTest, DuplicateHandle) {
  TestRunner target;
  TestRunner runner;

  
  target.SetAsynchronous(true);
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, target.RunTest(L"Handle_WaitProcess 30000"));

  
  base::string16 cmd_line = base::StringPrintf(L"Handle_DuplicateEvent %d",
                                               target.process_id());
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(cmd_line.c_str()));

  
  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_HANDLES,
                             TargetPolicy::HANDLES_DUP_ANY,
                             L"Event"));
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(cmd_line.c_str()));
}


TEST(HandlePolicyTest, DuplicatePeerHandle) {
  TestRunner target;
  TestRunner runner;

  
  target.SetAsynchronous(true);
  target.SetUnsandboxed(true);
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, target.RunTest(L"Handle_WaitProcess 30000"));

  
  base::string16 cmd_line = base::StringPrintf(L"Handle_DuplicateEvent %d",
                                               target.process_id());
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(cmd_line.c_str()));

  
  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_HANDLES,
                             TargetPolicy::HANDLES_DUP_ANY,
                             L"Event"));
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(cmd_line.c_str()));
}


TEST(HandlePolicyTest, DuplicateBrokerHandle) {
  TestRunner runner;

  
  base::string16 cmd_line = base::StringPrintf(L"Handle_DuplicateEvent %d",
                                             ::GetCurrentProcessId());
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(cmd_line.c_str()));

  
  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_HANDLES,
                             TargetPolicy::HANDLES_DUP_ANY,
                             L"Event"));
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(cmd_line.c_str()));


  
  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_HANDLES,
                             TargetPolicy::HANDLES_DUP_BROKER,
                             L"Event"));
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(cmd_line.c_str()));
}

}  

