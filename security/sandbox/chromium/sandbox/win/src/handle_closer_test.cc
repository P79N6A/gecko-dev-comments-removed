



#include "base/strings/stringprintf.h"
#include "base/win/scoped_handle.h"
#include "sandbox/win/src/handle_closer_agent.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/target_services.h"
#include "sandbox/win/tests/common/controller.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const wchar_t *kFileExtensions[] = { L".1", L".2", L".3", L".4" };


HANDLE GetMarkerFile(const wchar_t *extension) {
  wchar_t path_buffer[MAX_PATH + 1];
  CHECK(::GetTempPath(MAX_PATH, path_buffer));
  base::string16 marker_path = path_buffer;
  marker_path += L"\\sbox_marker_";

  
  CHECK(::GetModuleFileName(NULL, path_buffer, MAX_PATH));
  base::win::ScopedHandle module(::CreateFile(path_buffer,
                                 FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
  CHECK(module.IsValid());
  FILETIME timestamp;
  CHECK(::GetFileTime(module.Get(), &timestamp, NULL, NULL));
  marker_path += base::StringPrintf(L"%08x%08x%08x",
                                    ::GetFileSize(module.Get(), NULL),
                                    timestamp.dwLowDateTime,
                                    timestamp.dwHighDateTime);
  marker_path += extension;

  
  return CreateFile(marker_path.c_str(), FILE_ALL_ACCESS,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      NULL, OPEN_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
}


HANDLE finish_event;
const int kWaitCount = 20;

}  

namespace sandbox {




SBOX_TESTS_COMMAND int CheckForFileHandles(int argc, wchar_t **argv) {
  if (argc < 2)
    return SBOX_TEST_FAILED_TO_RUN_TEST;
  bool should_find = argv[0][0] == L'Y';
  if (argv[0][1] != L'\0' || !should_find && argv[0][0] != L'N')
    return SBOX_TEST_FAILED_TO_RUN_TEST;

  static int state = BEFORE_INIT;
  switch (state++) {
    case BEFORE_INIT:
      
      
      for (int i = 0; i < arraysize(kFileExtensions); ++i)
        EXPECT_NE(GetMarkerFile(kFileExtensions[i]), INVALID_HANDLE_VALUE);
      return SBOX_TEST_SUCCEEDED;

    case AFTER_REVERT: {
      
      DWORD handle_count = UINT_MAX;
      const int kInvalidHandleThreshold = 100;
      const size_t kHandleOffset = 4;  
      HANDLE handle = NULL;
      int invalid_count = 0;
      base::string16 handle_name;

      if (!::GetProcessHandleCount(::GetCurrentProcess(), &handle_count))
        return SBOX_TEST_FAILED_TO_RUN_TEST;

      while (handle_count && invalid_count < kInvalidHandleThreshold) {
        reinterpret_cast<size_t&>(handle) += kHandleOffset;
        if (GetHandleName(handle, &handle_name)) {
          for (int i = 1; i < argc; ++i) {
            if (handle_name == argv[i])
              return should_find ? SBOX_TEST_SUCCEEDED : SBOX_TEST_FAILED;
          }
          --handle_count;
        } else {
          ++invalid_count;
        }
      }

      return should_find ? SBOX_TEST_FAILED : SBOX_TEST_SUCCEEDED;
    }

    default:  
      break;
  }

  return SBOX_TEST_SUCCEEDED;
}

TEST(HandleCloserTest, CheckForMarkerFiles) {
  TestRunner runner;
  runner.SetTimeout(2000);
  runner.SetTestState(EVERY_STATE);

  base::string16 command = base::string16(L"CheckForFileHandles Y");
  for (int i = 0; i < arraysize(kFileExtensions); ++i) {
    base::string16 handle_name;
    base::win::ScopedHandle marker(GetMarkerFile(kFileExtensions[i]));
    CHECK(marker.IsValid());
    CHECK(sandbox::GetHandleName(marker.Get(), &handle_name));
    command += (L" ");
    command += handle_name;
  }

  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(command.c_str())) <<
    "Failed: " << command;
}

TEST(HandleCloserTest, CloseMarkerFiles) {
  TestRunner runner;
  runner.SetTimeout(2000);
  runner.SetTestState(EVERY_STATE);
  sandbox::TargetPolicy* policy = runner.GetPolicy();

  base::string16 command = base::string16(L"CheckForFileHandles N");
  for (int i = 0; i < arraysize(kFileExtensions); ++i) {
    base::string16 handle_name;
    base::win::ScopedHandle marker(GetMarkerFile(kFileExtensions[i]));
    CHECK(marker.IsValid());
    CHECK(sandbox::GetHandleName(marker.Get(), &handle_name));
    CHECK_EQ(policy->AddKernelObjectToClose(L"File", handle_name.c_str()),
              SBOX_ALL_OK);
    command += (L" ");
    command += handle_name;
  }

  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(command.c_str())) <<
    "Failed: " << command;
}

void WINAPI ThreadPoolTask(void* event, BOOLEAN timeout) {
  static volatile LONG waiters_remaining = kWaitCount;
  CHECK(!timeout);
  CHECK(::CloseHandle(event));
  if (::InterlockedDecrement(&waiters_remaining) == 0)
    CHECK(::SetEvent(finish_event));
}


SBOX_TESTS_COMMAND int RunThreadPool(int argc, wchar_t **argv) {
  HANDLE wait_list[20];
  finish_event = ::CreateEvent(NULL, TRUE, FALSE, NULL);
  CHECK(finish_event);

  
  HANDLE pool = NULL;
  for (int i = 0; i < kWaitCount; ++i) {
    HANDLE event = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    CHECK(event);
    CHECK(::RegisterWaitForSingleObject(&pool, event, ThreadPoolTask, event,
                                        INFINITE, WT_EXECUTEONLYONCE));
    wait_list[i] = event;
  }

  
  for (int i = 0; i < kWaitCount; ++i)
    CHECK(::SetEvent(wait_list[i]));

  CHECK_EQ(::WaitForSingleObject(finish_event, INFINITE), WAIT_OBJECT_0);
  CHECK(::CloseHandle(finish_event));

  return SBOX_TEST_SUCCEEDED;
}

TEST(HandleCloserTest, RunThreadPool) {
  TestRunner runner;
  runner.SetTimeout(2000);
  runner.SetTestState(AFTER_REVERT);
  sandbox::TargetPolicy* policy = runner.GetPolicy();

  
  CHECK_EQ(policy->AddKernelObjectToClose(L"ALPC Port", NULL), SBOX_ALL_OK);

  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(L"RunThreadPool"));
}

}  
