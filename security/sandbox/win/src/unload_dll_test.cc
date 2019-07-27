



#include "base/win/scoped_handle.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/target_services.h"
#include "sandbox/win/tests/common/controller.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {



SBOX_TESTS_COMMAND int UseOneDLL(int argc, wchar_t **argv) {
  if (argc != 2)
    return SBOX_TEST_FAILED_TO_RUN_TEST;
  int rv = SBOX_TEST_FAILED_TO_RUN_TEST;

  wchar_t option = (argv[0])[0];
  if ((option == L'L') || (option == L'B')) {
    HMODULE module1 = ::LoadLibraryW(argv[1]);
    rv = (module1 == NULL) ? SBOX_TEST_FAILED : SBOX_TEST_SUCCEEDED;
  }

  if ((option == L'U') || (option == L'B')) {
    HMODULE module2 = ::GetModuleHandleW(argv[1]);
    rv = ::FreeLibrary(module2) ? SBOX_TEST_SUCCEEDED : SBOX_TEST_FAILED;
  }
  return rv;
}


SBOX_TESTS_COMMAND int SimpleOpenEvent(int argc, wchar_t **argv) {
  if (argc != 1)
    return SBOX_TEST_FAILED_TO_EXECUTE_COMMAND;

  base::win::ScopedHandle event_open(::OpenEvent(SYNCHRONIZE, FALSE, argv[0]));
  return event_open.Get() ? SBOX_TEST_SUCCEEDED : SBOX_TEST_FAILED;
}


TEST(UnloadDllTest, DISABLED_BaselineAvicapDll) {
  TestRunner runner;
  runner.SetTestState(BEFORE_REVERT);
  runner.SetTimeout(2000);
  
  
  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_SYNC,
                             TargetPolicy::EVENTS_ALLOW_ANY, L"t0001"));

  
  
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(L"UseOneDLL L avicap32.dll"));
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(L"UseOneDLL B avicap32.dll"));
}


TEST(UnloadDllTest, DISABLED_UnloadAviCapDllNoPatching) {
  TestRunner runner;
  runner.SetTestState(BEFORE_REVERT);
  runner.SetTimeout(2000);
  sandbox::TargetPolicy* policy = runner.GetPolicy();
  policy->AddDllToUnload(L"avicap32.dll");
  EXPECT_EQ(SBOX_TEST_FAILED, runner.RunTest(L"UseOneDLL L avicap32.dll"));
  EXPECT_EQ(SBOX_TEST_FAILED, runner.RunTest(L"UseOneDLL B avicap32.dll"));
}


TEST(UnloadDllTest, DISABLED_UnloadAviCapDllWithPatching) {
  TestRunner runner;
  runner.SetTimeout(2000);
  runner.SetTestState(BEFORE_REVERT);
  sandbox::TargetPolicy* policy = runner.GetPolicy();
  policy->AddDllToUnload(L"avicap32.dll");

  base::win::ScopedHandle handle1(::CreateEvent(
      NULL, FALSE, FALSE, L"tst0001"));

  
  
  
  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_ANY,
                             L"HKEY_LOCAL_MACHINE\\Software\\Microsoft"));
  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_SYNC,
                             TargetPolicy::EVENTS_ALLOW_ANY, L"tst0001"));

  EXPECT_EQ(SBOX_TEST_FAILED, runner.RunTest(L"UseOneDLL L avicap32.dll"));

  runner.SetTestState(AFTER_REVERT);
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(L"SimpleOpenEvent tst0001"));
}

}  
