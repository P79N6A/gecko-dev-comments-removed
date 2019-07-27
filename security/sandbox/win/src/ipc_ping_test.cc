



#include "testing/gtest/include/gtest/gtest.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/target_services.h"
#include "sandbox/win/tests/common/controller.h"

namespace sandbox {



SBOX_TESTS_COMMAND int IPC_Ping(int argc, wchar_t **argv) {
  if (argc != 1)
    return SBOX_TEST_FAILED;

  TargetServices* ts = SandboxFactory::GetTargetServices();
  if (NULL == ts)
    return SBOX_TEST_FAILED;

  
  TargetServicesBase* ts_base = reinterpret_cast<TargetServicesBase*>(ts);

  int version = 0;
  if (L'1' == argv[0][0])
    version = 1;
  else
    version = 2;

  if (!ts_base->TestIPCPing(version))
    return SBOX_TEST_FAILED;

  ::Sleep(1);
  if (!ts_base->TestIPCPing(version))
    return SBOX_TEST_FAILED;

  return SBOX_TEST_SUCCEEDED;
}


TEST(IPCTest, IPCPingTestSimple) {
  TestRunner runner;
  runner.SetTimeout(2000);
  runner.SetTestState(EVERY_STATE);
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(L"IPC_Ping 1"));
}

TEST(IPCTest, IPCPingTestWithOutput) {
  TestRunner runner;
  runner.SetTimeout(2000);
  runner.SetTestState(EVERY_STATE);
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(L"IPC_Ping 2"));
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(L"IPC_Ping 2"));
}

}  
