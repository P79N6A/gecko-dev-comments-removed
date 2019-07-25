




























#include <windows.h>
#include <dbghelp.h>
#include <strsafe.h>
#include <objbase.h>
#include <shellapi.h>

#include "../../../breakpad_googletest_includes.h"
#include "../crash_generation/crash_generation_server.h"
#include "../handler/exception_handler.h"
#include "dump_analysis.h"  

namespace {
const wchar_t kPipeName[] = L"\\\\.\\pipe\\BreakpadCrashTest\\TestCaseServer";
const char kSuccessIndicator[] = "success";
const char kFailureIndicator[] = "failure";

const MINIDUMP_TYPE kFullDumpType = static_cast<MINIDUMP_TYPE>(
    MiniDumpWithFullMemory |  
    MiniDumpWithProcessThreadData |  
    MiniDumpWithHandleData);  

class ExceptionHandlerTest : public ::testing::Test {
 protected:
  
  
  TCHAR temp_path_[MAX_PATH];

  
  virtual void SetUp();

  
  virtual void TearDown();

  void DoCrashInvalidParameter();
  void DoCrashPureVirtualCall();

  
  static BOOL DoesPathExist(const TCHAR *path_name);

  
  static void ClientDumpCallback(
      void *dump_context,
      const google_breakpad::ClientInfo *client_info,
      const std::wstring *dump_path);

  static std::wstring dump_file;
  static std::wstring full_dump_file;
};

std::wstring ExceptionHandlerTest::dump_file;
std::wstring ExceptionHandlerTest::full_dump_file;

void ExceptionHandlerTest::SetUp() {
  const ::testing::TestInfo* const test_info =
    ::testing::UnitTest::GetInstance()->current_test_info();
  TCHAR temp_path[MAX_PATH] = { '\0' };
  TCHAR test_name_wide[MAX_PATH] = { '\0' };
  
  
  GetTempPath(MAX_PATH, temp_path);
  
  
  int dwRet = MultiByteToWideChar(CP_ACP, 0, test_info->name(),
                                  strlen(test_info->name()),
                                  test_name_wide,
                                  MAX_PATH);
  if (!dwRet) {
    assert(false);
  }
  StringCchPrintfW(temp_path_, MAX_PATH, L"%s%s", temp_path, test_name_wide);
  CreateDirectory(temp_path_, NULL);
}

void ExceptionHandlerTest::TearDown() {
  if (!dump_file.empty()) {
    ::DeleteFile(dump_file.c_str());
    dump_file = L"";
  }
  if (!full_dump_file.empty()) {
    ::DeleteFile(full_dump_file.c_str());
    full_dump_file = L"";
  }
}

BOOL ExceptionHandlerTest::DoesPathExist(const TCHAR *path_name) {
  DWORD flags = GetFileAttributes(path_name);
  if (flags == INVALID_FILE_ATTRIBUTES) {
    return FALSE;
  }
  return TRUE;
}

void ExceptionHandlerTest::ClientDumpCallback(
    void *dump_context,
    const google_breakpad::ClientInfo *client_info,
    const std::wstring *dump_path) {
  dump_file = *dump_path;
  
  full_dump_file = dump_file.substr(0, dump_file.length() - 4) + L"-full.dmp";
}

void ExceptionHandlerTest::DoCrashInvalidParameter() {
  google_breakpad::ExceptionHandler *exc =
      new google_breakpad::ExceptionHandler(
          temp_path_, NULL, NULL, NULL,
          google_breakpad::ExceptionHandler::HANDLER_INVALID_PARAMETER,
          kFullDumpType, kPipeName, NULL);

  
  _CrtSetReportMode(_CRT_ASSERT, 0);

  
  
  
  ASSERT_TRUE(exc->IsOutOfProcess());
  printf(NULL);
}


struct PureVirtualCallBase {
  PureVirtualCallBase() {
    
    
    reinterpret_cast<PureVirtualCallBase*>(this)->PureFunction();
  }
  virtual ~PureVirtualCallBase() {}
  virtual void PureFunction() const = 0;
};
struct PureVirtualCall : public PureVirtualCallBase {
  PureVirtualCall() { PureFunction(); }
  virtual void PureFunction() const {}
};

void ExceptionHandlerTest::DoCrashPureVirtualCall() {
  google_breakpad::ExceptionHandler *exc =
      new google_breakpad::ExceptionHandler(
          temp_path_, NULL, NULL, NULL,
          google_breakpad::ExceptionHandler::HANDLER_PURECALL,
          kFullDumpType, kPipeName, NULL);

  
  _CrtSetReportMode(_CRT_ASSERT, 0);

  
  
  
  ASSERT_TRUE(exc->IsOutOfProcess());

  
  
  {
    PureVirtualCall instance;
  }
}


TEST_F(ExceptionHandlerTest, InvalidParameterMiniDumpTest) {
  ASSERT_TRUE(DoesPathExist(temp_path_));

  
  ASSERT_TRUE(DoesPathExist(temp_path_));
  std::wstring dump_path(temp_path_);
  google_breakpad::CrashGenerationServer server(
      kPipeName, NULL, NULL, NULL, ClientDumpCallback, NULL, NULL, NULL, true,
      &dump_path);

  ASSERT_TRUE(dump_file.empty() && full_dump_file.empty());

  
  
  
  EXPECT_TRUE(server.Start());
  EXPECT_EXIT(DoCrashInvalidParameter(), ::testing::ExitedWithCode(0), "");
  ASSERT_TRUE(!dump_file.empty() && !full_dump_file.empty());
  ASSERT_TRUE(DoesPathExist(dump_file.c_str()));

  
  DumpAnalysis mini(dump_file);
  DumpAnalysis full(full_dump_file);

  
  EXPECT_TRUE(mini.HasStream(ThreadListStream));
  EXPECT_TRUE(full.HasStream(ThreadListStream));
  EXPECT_TRUE(mini.HasStream(ModuleListStream));
  EXPECT_TRUE(full.HasStream(ModuleListStream));
  EXPECT_TRUE(mini.HasStream(ExceptionStream));
  EXPECT_TRUE(full.HasStream(ExceptionStream));
  EXPECT_TRUE(mini.HasStream(SystemInfoStream));
  EXPECT_TRUE(full.HasStream(SystemInfoStream));
  EXPECT_TRUE(mini.HasStream(MiscInfoStream));
  EXPECT_TRUE(full.HasStream(MiscInfoStream));
  EXPECT_TRUE(mini.HasStream(HandleDataStream));
  EXPECT_TRUE(full.HasStream(HandleDataStream));

  
  EXPECT_TRUE(mini.HasTebs() || full.HasTebs());
  EXPECT_TRUE(mini.HasPeb() || full.HasPeb());

  
  EXPECT_TRUE(mini.HasStream(MemoryListStream));
  EXPECT_FALSE(mini.HasStream(Memory64ListStream));

  EXPECT_FALSE(full.HasStream(MemoryListStream));
  EXPECT_TRUE(full.HasStream(Memory64ListStream));

  
  
  EXPECT_FALSE(mini.HasStream(ThreadExListStream));
  EXPECT_FALSE(full.HasStream(ThreadExListStream));
  EXPECT_FALSE(mini.HasStream(CommentStreamA));
  EXPECT_FALSE(full.HasStream(CommentStreamA));
  EXPECT_FALSE(mini.HasStream(CommentStreamW));
  EXPECT_FALSE(full.HasStream(CommentStreamW));
  EXPECT_FALSE(mini.HasStream(FunctionTableStream));
  EXPECT_FALSE(full.HasStream(FunctionTableStream));
  EXPECT_FALSE(mini.HasStream(MemoryInfoListStream));
  EXPECT_FALSE(full.HasStream(MemoryInfoListStream));
  EXPECT_FALSE(mini.HasStream(ThreadInfoListStream));
  EXPECT_FALSE(full.HasStream(ThreadInfoListStream));
  EXPECT_FALSE(mini.HasStream(HandleOperationListStream));
  EXPECT_FALSE(full.HasStream(HandleOperationListStream));
  EXPECT_FALSE(mini.HasStream(TokenStream));
  EXPECT_FALSE(full.HasStream(TokenStream));
}



TEST_F(ExceptionHandlerTest, PureVirtualCallMiniDumpTest) {
  ASSERT_TRUE(DoesPathExist(temp_path_));

  
  ASSERT_TRUE(DoesPathExist(temp_path_));
  std::wstring dump_path(temp_path_);
  google_breakpad::CrashGenerationServer server(
      kPipeName, NULL, NULL, NULL, ClientDumpCallback, NULL, NULL, NULL, true,
      &dump_path);

  ASSERT_TRUE(dump_file.empty() && full_dump_file.empty());

  
  
  
  EXPECT_TRUE(server.Start());
  EXPECT_EXIT(DoCrashPureVirtualCall(), ::testing::ExitedWithCode(0), "");
  ASSERT_TRUE(!dump_file.empty() && !full_dump_file.empty());
  ASSERT_TRUE(DoesPathExist(dump_file.c_str()));

  
  DumpAnalysis mini(dump_file);
  DumpAnalysis full(full_dump_file);

  
  EXPECT_TRUE(mini.HasStream(ThreadListStream));
  EXPECT_TRUE(full.HasStream(ThreadListStream));
  EXPECT_TRUE(mini.HasStream(ModuleListStream));
  EXPECT_TRUE(full.HasStream(ModuleListStream));
  EXPECT_TRUE(mini.HasStream(ExceptionStream));
  EXPECT_TRUE(full.HasStream(ExceptionStream));
  EXPECT_TRUE(mini.HasStream(SystemInfoStream));
  EXPECT_TRUE(full.HasStream(SystemInfoStream));
  EXPECT_TRUE(mini.HasStream(MiscInfoStream));
  EXPECT_TRUE(full.HasStream(MiscInfoStream));
  EXPECT_TRUE(mini.HasStream(HandleDataStream));
  EXPECT_TRUE(full.HasStream(HandleDataStream));

  
  EXPECT_TRUE(mini.HasTebs() || full.HasTebs());
  EXPECT_TRUE(mini.HasPeb() || full.HasPeb());

  
  EXPECT_TRUE(mini.HasStream(MemoryListStream));
  EXPECT_FALSE(mini.HasStream(Memory64ListStream));

  EXPECT_FALSE(full.HasStream(MemoryListStream));
  EXPECT_TRUE(full.HasStream(Memory64ListStream));

  
  
  EXPECT_FALSE(mini.HasStream(ThreadExListStream));
  EXPECT_FALSE(full.HasStream(ThreadExListStream));
  EXPECT_FALSE(mini.HasStream(CommentStreamA));
  EXPECT_FALSE(full.HasStream(CommentStreamA));
  EXPECT_FALSE(mini.HasStream(CommentStreamW));
  EXPECT_FALSE(full.HasStream(CommentStreamW));
  EXPECT_FALSE(mini.HasStream(FunctionTableStream));
  EXPECT_FALSE(full.HasStream(FunctionTableStream));
  EXPECT_FALSE(mini.HasStream(MemoryInfoListStream));
  EXPECT_FALSE(full.HasStream(MemoryInfoListStream));
  EXPECT_FALSE(mini.HasStream(ThreadInfoListStream));
  EXPECT_FALSE(full.HasStream(ThreadInfoListStream));
  EXPECT_FALSE(mini.HasStream(HandleOperationListStream));
  EXPECT_FALSE(full.HasStream(HandleOperationListStream));
  EXPECT_FALSE(mini.HasStream(TokenStream));
  EXPECT_FALSE(full.HasStream(TokenStream));
}
}
