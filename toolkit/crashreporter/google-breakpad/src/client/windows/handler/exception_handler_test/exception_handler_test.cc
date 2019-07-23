




























#include "breakpad_googletest_includes.h"
#include "client/windows/crash_generation/crash_generation_server.h"
#include "client/windows/handler/exception_handler.h"
#include <windows.h>
#include <dbghelp.h>
#include <strsafe.h>
#include <objbase.h>
#include <shellapi.h>

namespace {
const wchar_t kPipeName[] = L"\\\\.\\pipe\\BreakpadCrashTest\\TestCaseServer";
const char kSuccessIndicator[] = "success";
const char kFailureIndicator[] = "failure";


BOOL DoesPathExist(const TCHAR *path_name);

class ExceptionHandlerDeathTest : public ::testing::Test {
protected:
  
  
  TCHAR temp_path_[MAX_PATH];
  
  virtual void SetUp();
  
  void DoCrash();
};

void ExceptionHandlerDeathTest::SetUp() {
  const ::testing::TestInfo* const test_info =
    ::testing::UnitTest::GetInstance()->current_test_info();
  TCHAR temp_path[MAX_PATH] = { '\0' };
  TCHAR test_name_wide[MAX_PATH] = { '\0' };
  
  
  GetTempPath(MAX_PATH, temp_path);
  
  
  int dwRet = MultiByteToWideChar(CP_ACP, 0, test_info->name(), 
    (int)strlen(test_info->name()), test_name_wide, MAX_PATH);
  if (!dwRet) {
    assert(false);
  }
  StringCchPrintfW(temp_path_, MAX_PATH, L"%s%s", temp_path, test_name_wide);
  CreateDirectory(temp_path_, NULL);
}

BOOL DoesPathExist(const TCHAR *path_name) {
  DWORD flags = GetFileAttributes(path_name);
  if (flags == INVALID_FILE_ATTRIBUTES) {
    return FALSE;
  }
  return TRUE;
}

bool MinidumpWrittenCallback(const wchar_t* dump_path, 
                             const wchar_t* minidump_id,
                             void* context,
                             EXCEPTION_POINTERS* exinfo,
                             MDRawAssertionInfo* assertion,
                             bool succeeded) {
  if (succeeded && DoesPathExist(dump_path)) {
    fprintf(stderr, kSuccessIndicator);
  } else {
    fprintf(stderr, kFailureIndicator);
  }
  
  
  fflush(stderr);
  return succeeded;
}

TEST_F(ExceptionHandlerDeathTest, InProcTest) {
  
  
  
  
  
  ASSERT_TRUE(DoesPathExist(temp_path_));
  google_breakpad::ExceptionHandler *exc = 
    new google_breakpad::ExceptionHandler(
    temp_path_, NULL, &MinidumpWrittenCallback, NULL, 
    google_breakpad::ExceptionHandler::HANDLER_ALL);
  int *i = NULL;  
  ASSERT_DEATH((*i)++, kSuccessIndicator);
  delete exc;
}

static bool gDumpCallbackCalled = false;

void clientDumpCallback(void *dump_context,
                        const google_breakpad::ClientInfo *client_info,
                        const std::wstring *dump_path){

  gDumpCallbackCalled = true;
}

void ExceptionHandlerDeathTest::DoCrash() {
  google_breakpad::ExceptionHandler *exc = 
    new google_breakpad::ExceptionHandler(
    temp_path_, NULL, NULL, NULL,
    google_breakpad::ExceptionHandler::HANDLER_ALL, MiniDumpNormal, kPipeName,
    NULL);
  
  
  
  ASSERT_TRUE(exc->IsOutOfProcess());
  int *i = NULL;
  printf("%d\n", (*i)++);
}

TEST_F(ExceptionHandlerDeathTest, OutOfProcTest) {
  
  
  
  
  
  
  

  ASSERT_TRUE(DoesPathExist(temp_path_));
  std::wstring dump_path(temp_path_);
  google_breakpad::CrashGenerationServer server(
    kPipeName, NULL, NULL, NULL, &clientDumpCallback, NULL, NULL, NULL, true,
    &dump_path);
  
  
  
  
  EXPECT_TRUE(server.Start());
  EXPECT_FALSE(gDumpCallbackCalled);
  ASSERT_DEATH(this->DoCrash(), "");
  EXPECT_TRUE(gDumpCallbackCalled);
}
}