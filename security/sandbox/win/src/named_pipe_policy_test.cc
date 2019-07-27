



#include "base/win/windows_version.h"
#include "sandbox/win/src/handle_closer.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_policy.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/tests/common/controller.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {


SBOX_TESTS_COMMAND int NamedPipe_Create(int argc, wchar_t **argv) {
  if (argc < 1 || argc > 2) {
    return SBOX_TEST_FAILED_TO_EXECUTE_COMMAND;
  }
  if ((NULL == argv) || (NULL == argv[0])) {
    return SBOX_TEST_FAILED_TO_EXECUTE_COMMAND;
  }

  HANDLE pipe = ::CreateNamedPipeW(argv[0],
                                   PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                   PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, 1, 4096,
                                   4096, 2000, NULL);
  if (INVALID_HANDLE_VALUE == pipe)
    return SBOX_TEST_DENIED;

  
  
  if (argc == 2) {
    base::string16 handle_name;
    if (GetHandleName(pipe, &handle_name)) {
      if (handle_name.compare(0, wcslen(argv[1]), argv[1]) != 0)
        return SBOX_TEST_FAILED;
    } else {
      return SBOX_TEST_FAILED;
    }
  }

  OVERLAPPED overlapped = {0};
  overlapped.hEvent = ::CreateEvent(NULL, TRUE, TRUE, NULL);
  BOOL result = ::ConnectNamedPipe(pipe, &overlapped);

  if (!result) {
    DWORD error = ::GetLastError();
    if (ERROR_PIPE_CONNECTED != error &&
        ERROR_IO_PENDING != error) {
          return SBOX_TEST_FAILED;
    }
  }

  if (!::CloseHandle(pipe))
    return SBOX_TEST_FAILED;

  ::CloseHandle(overlapped.hEvent);
  return SBOX_TEST_SUCCEEDED;
}


TEST(NamedPipePolicyTest, CreatePipe) {
  TestRunner runner;
  
  
  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_NAMED_PIPES,
                             TargetPolicy::NAMEDPIPES_ALLOW_ANY,
                             L"\\\\.\\pipe\\test*"));

  EXPECT_EQ(SBOX_TEST_SUCCEEDED,
            runner.RunTest(L"NamedPipe_Create \\\\.\\pipe\\testbleh"));

  
  
  if (base::win::OSInfo::GetInstance()->version() >= base::win::VERSION_VISTA) {
    EXPECT_EQ(SBOX_TEST_DENIED,
              runner.RunTest(L"NamedPipe_Create \\\\.\\pipe\\bleh"));
  }
}


TEST(NamedPipePolicyTest, CreatePipeTraversal) {
  TestRunner runner;
  
  
  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_NAMED_PIPES,
                             TargetPolicy::NAMEDPIPES_ALLOW_ANY,
                              L"\\\\.\\pipe\\test*"));

  
  
  if (base::win::OSInfo::GetInstance()->version() >= base::win::VERSION_VISTA) {
    EXPECT_EQ(SBOX_TEST_DENIED,
              runner.RunTest(L"NamedPipe_Create \\\\.\\pipe\\test\\..\\bleh"));
    EXPECT_EQ(SBOX_TEST_DENIED,
              runner.RunTest(L"NamedPipe_Create \\\\.\\pipe\\test/../bleh"));
    EXPECT_EQ(SBOX_TEST_DENIED,
              runner.RunTest(L"NamedPipe_Create \\\\.\\pipe\\test\\../bleh"));
    EXPECT_EQ(SBOX_TEST_DENIED,
              runner.RunTest(L"NamedPipe_Create \\\\.\\pipe\\test/..\\bleh"));
  }
}



TEST(NamedPipePolicyTest, CreatePipeCanonicalization) {
  
  
  
  
  const wchar_t* argv[2] = { L"\\\\?\\pipe\\test\\..\\bleh",
                             L"\\Device\\NamedPipe\\test" };
  EXPECT_EQ(SBOX_TEST_SUCCEEDED,
            NamedPipe_Create(2, const_cast<wchar_t**>(argv)));
}


TEST(NamedPipePolicyTest, CreatePipeStrictInterceptions) {
  TestRunner runner;
  runner.GetPolicy()->SetStrictInterceptions();

  
  
  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_NAMED_PIPES,
                             TargetPolicy::NAMEDPIPES_ALLOW_ANY,
                              L"\\\\.\\pipe\\test*"));

  EXPECT_EQ(SBOX_TEST_SUCCEEDED,
            runner.RunTest(L"NamedPipe_Create \\\\.\\pipe\\testbleh"));

  
  
  if (base::win::OSInfo::GetInstance()->version() >= base::win::VERSION_VISTA) {
    EXPECT_EQ(SBOX_TEST_DENIED,
              runner.RunTest(L"NamedPipe_Create \\\\.\\pipe\\bleh"));
  }
}

}  
