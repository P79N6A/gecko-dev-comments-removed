




























#include <windows.h>
#include <dbghelp.h>
#include <strsafe.h>
#include <objbase.h>
#include <shellapi.h>

#include <string>

#include "breakpad_googletest_includes.h"
#include "client/windows/crash_generation/crash_generation_server.h"
#include "client/windows/handler/exception_handler.h"
#include "client/windows/unittests/exception_handler_test.h"
#include "common/windows/string_utils-inl.h"
#include "google_breakpad/processor/minidump.h"

namespace {

using std::wstring;
using namespace google_breakpad;

const wchar_t kPipeName[] = L"\\\\.\\pipe\\BreakpadCrashTest\\TestCaseServer";
const char kSuccessIndicator[] = "success";
const char kFailureIndicator[] = "failure";


BOOL DoesPathExist(const TCHAR *path_name);

class ExceptionHandlerDeathTest : public ::testing::Test {
 protected:
  
  
  TCHAR temp_path_[MAX_PATH];
  
  virtual void SetUp();
  
  void DoCrashAccessViolation();
  void DoCrashPureVirtualCall();
};

void ExceptionHandlerDeathTest::SetUp() {
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

  
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  int *i = NULL;
  ASSERT_DEATH((*i)++, kSuccessIndicator);
  delete exc;
}

static bool gDumpCallbackCalled = false;

void clientDumpCallback(void *dump_context,
                        const google_breakpad::ClientInfo *client_info,
                        const std::wstring *dump_path) {
  gDumpCallbackCalled = true;
}

void ExceptionHandlerDeathTest::DoCrashAccessViolation() {
  google_breakpad::ExceptionHandler *exc =
    new google_breakpad::ExceptionHandler(
    temp_path_, NULL, NULL, NULL,
    google_breakpad::ExceptionHandler::HANDLER_ALL, MiniDumpNormal, kPipeName,
    NULL);

  
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  
  
  
  ASSERT_TRUE(exc->IsOutOfProcess());
  int *i = NULL;
  printf("%d\n", (*i)++);
}

TEST_F(ExceptionHandlerDeathTest, OutOfProcTest) {
  
  
  
  
  
  
  

  ASSERT_TRUE(DoesPathExist(temp_path_));
  std::wstring dump_path(temp_path_);
  google_breakpad::CrashGenerationServer server(
    kPipeName, NULL, NULL, NULL, &clientDumpCallback, NULL, NULL, NULL, NULL,
    NULL, true, &dump_path);

  
  
  
  EXPECT_TRUE(server.Start());
  EXPECT_FALSE(gDumpCallbackCalled);
  ASSERT_DEATH(this->DoCrashAccessViolation(), "");
  EXPECT_TRUE(gDumpCallbackCalled);
}

TEST_F(ExceptionHandlerDeathTest, InvalidParameterTest) {
  using google_breakpad::ExceptionHandler;

  ASSERT_TRUE(DoesPathExist(temp_path_));
  ExceptionHandler handler(temp_path_, NULL, NULL, NULL,
                           ExceptionHandler::HANDLER_INVALID_PARAMETER);

  
  _CrtSetReportMode(_CRT_ASSERT, 0);

  
  
  ASSERT_EXIT(printf(NULL), ::testing::ExitedWithCode(0), "");
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

void ExceptionHandlerDeathTest::DoCrashPureVirtualCall() {
  PureVirtualCall instance;
}

TEST_F(ExceptionHandlerDeathTest, PureVirtualCallTest) {
  using google_breakpad::ExceptionHandler;

  ASSERT_TRUE(DoesPathExist(temp_path_));
  ExceptionHandler handler(temp_path_, NULL, NULL, NULL,
                           ExceptionHandler::HANDLER_PURECALL);

  
  _CrtSetReportMode(_CRT_ASSERT, 0);

  
  EXPECT_EXIT(DoCrashPureVirtualCall(), ::testing::ExitedWithCode(0), "");
}

wstring find_minidump_in_directory(const wstring &directory) {
  wstring search_path = directory + L"\\*";
  WIN32_FIND_DATA find_data;
  HANDLE find_handle = FindFirstFileW(search_path.c_str(), &find_data);
  if (find_handle == INVALID_HANDLE_VALUE)
    return wstring();

  wstring filename;
  do {
    const wchar_t extension[] = L".dmp";
    const int extension_length = sizeof(extension) / sizeof(extension[0]) - 1;
    const int filename_length = wcslen(find_data.cFileName);
    if (filename_length > extension_length &&
    wcsncmp(extension,
            find_data.cFileName + filename_length - extension_length,
            extension_length) == 0) {
      filename = directory + L"\\" + find_data.cFileName;
      break;
    }
  } while(FindNextFile(find_handle, &find_data));
  FindClose(find_handle);
  return filename;
}

TEST_F(ExceptionHandlerDeathTest, InstructionPointerMemory) {
  ASSERT_TRUE(DoesPathExist(temp_path_));
  google_breakpad::ExceptionHandler *exc =
      new google_breakpad::ExceptionHandler(
          temp_path_, NULL, NULL, NULL,
          google_breakpad::ExceptionHandler::HANDLER_ALL);

  
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  
  const uint32_t kMemorySize = 256;  
  const int kOffset = kMemorySize / 2;
  
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };
  char* memory = reinterpret_cast<char*>(VirtualAlloc(NULL,
                                                      kMemorySize,
                                                      MEM_COMMIT | MEM_RESERVE,
                                                      PAGE_EXECUTE_READWRITE));
  ASSERT_TRUE(memory);

  
  
  
  
  memcpy(memory + kOffset, instructions, sizeof(instructions));
  
  
  typedef void (*void_function)(void);
  void_function memory_function =
      reinterpret_cast<void_function>(memory + kOffset);
  ASSERT_DEATH(memory_function(), "");

  
  VirtualFree(memory, 0, MEM_RELEASE);

  
  wstring minidump_filename_wide = find_minidump_in_directory(temp_path_);
  ASSERT_FALSE(minidump_filename_wide.empty());
  string minidump_filename;
  ASSERT_TRUE(WindowsStringUtils::safe_wcstombs(minidump_filename_wide,
                                                &minidump_filename));

  
  
  
  
  {
    Minidump minidump(minidump_filename);
    ASSERT_TRUE(minidump.Read());

    MinidumpException* exception = minidump.GetException();
    MinidumpMemoryList* memory_list = minidump.GetMemoryList();
    ASSERT_TRUE(exception);
    ASSERT_TRUE(memory_list);
    ASSERT_LT((unsigned)0, memory_list->region_count());

    MinidumpContext* context = exception->GetContext();
    ASSERT_TRUE(context);

    uint64_t instruction_pointer;
    ASSERT_TRUE(context->GetInstructionPointer(&instruction_pointer));

    MinidumpMemoryRegion* region =
        memory_list->GetMemoryRegionForAddress(instruction_pointer);
    ASSERT_TRUE(region);

    EXPECT_EQ(kMemorySize, region->GetSize());
    const uint8_t* bytes = region->GetMemory();
    ASSERT_TRUE(bytes);

    uint8_t prefix_bytes[kOffset];
    uint8_t suffix_bytes[kMemorySize - kOffset - sizeof(instructions)];
    memset(prefix_bytes, 0, sizeof(prefix_bytes));
    memset(suffix_bytes, 0, sizeof(suffix_bytes));
    EXPECT_TRUE(memcmp(bytes, prefix_bytes, sizeof(prefix_bytes)) == 0);
    EXPECT_TRUE(memcmp(bytes + kOffset, instructions,
                       sizeof(instructions)) == 0);
    EXPECT_TRUE(memcmp(bytes + kOffset + sizeof(instructions),
                       suffix_bytes, sizeof(suffix_bytes)) == 0);
  }

  DeleteFileW(minidump_filename_wide.c_str());
}

TEST_F(ExceptionHandlerDeathTest, InstructionPointerMemoryMinBound) {
  ASSERT_TRUE(DoesPathExist(temp_path_));
  google_breakpad::ExceptionHandler *exc =
      new google_breakpad::ExceptionHandler(
          temp_path_, NULL, NULL, NULL,
          google_breakpad::ExceptionHandler::HANDLER_ALL);

  
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  SYSTEM_INFO sSysInfo;         
  GetSystemInfo(&sSysInfo);     

  const uint32_t kMemorySize = 256;  
  const DWORD kPageSize = sSysInfo.dwPageSize;
  const int kOffset = 0;
  
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };
  
  
  char* all_memory = reinterpret_cast<char*>(VirtualAlloc(NULL,
                                                          kPageSize * 2,
                                                          MEM_RESERVE,
                                                          PAGE_NOACCESS));
  ASSERT_TRUE(all_memory);
  char* memory = all_memory + kPageSize;
  ASSERT_TRUE(VirtualAlloc(memory, kPageSize,
                           MEM_COMMIT, PAGE_EXECUTE_READWRITE));

  
  
  
  
  memcpy(memory + kOffset, instructions, sizeof(instructions));
  
  
  typedef void (*void_function)(void);
  void_function memory_function =
      reinterpret_cast<void_function>(memory + kOffset);
  ASSERT_DEATH(memory_function(), "");

  
  VirtualFree(memory, 0, MEM_RELEASE);

  
  wstring minidump_filename_wide = find_minidump_in_directory(temp_path_);
  ASSERT_FALSE(minidump_filename_wide.empty());
  string minidump_filename;
  ASSERT_TRUE(WindowsStringUtils::safe_wcstombs(minidump_filename_wide,
                                                &minidump_filename));

  
  
  
  
  {
    Minidump minidump(minidump_filename);
    ASSERT_TRUE(minidump.Read());

    MinidumpException* exception = minidump.GetException();
    MinidumpMemoryList* memory_list = minidump.GetMemoryList();
    ASSERT_TRUE(exception);
    ASSERT_TRUE(memory_list);
    ASSERT_LT((unsigned)0, memory_list->region_count());

    MinidumpContext* context = exception->GetContext();
    ASSERT_TRUE(context);

    uint64_t instruction_pointer;
    ASSERT_TRUE(context->GetInstructionPointer(&instruction_pointer));

    MinidumpMemoryRegion* region =
        memory_list->GetMemoryRegionForAddress(instruction_pointer);
    ASSERT_TRUE(region);

    EXPECT_EQ(kMemorySize / 2, region->GetSize());
    const uint8_t* bytes = region->GetMemory();
    ASSERT_TRUE(bytes);

    uint8_t suffix_bytes[kMemorySize / 2 - sizeof(instructions)];
    memset(suffix_bytes, 0, sizeof(suffix_bytes));
    EXPECT_TRUE(memcmp(bytes + kOffset,
                       instructions, sizeof(instructions)) == 0);
    EXPECT_TRUE(memcmp(bytes + kOffset + sizeof(instructions),
                       suffix_bytes, sizeof(suffix_bytes)) == 0);
  }

  DeleteFileW(minidump_filename_wide.c_str());
}

TEST_F(ExceptionHandlerDeathTest, InstructionPointerMemoryMaxBound) {
  ASSERT_TRUE(DoesPathExist(temp_path_));
  google_breakpad::ExceptionHandler *exc =
      new google_breakpad::ExceptionHandler(
          temp_path_, NULL, NULL, NULL,
          google_breakpad::ExceptionHandler::HANDLER_ALL);

  
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  SYSTEM_INFO sSysInfo;         
  GetSystemInfo(&sSysInfo);     

  const DWORD kPageSize = sSysInfo.dwPageSize;
  
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };
  const int kOffset = kPageSize - sizeof(instructions);
  
  
  char* memory = reinterpret_cast<char*>(VirtualAlloc(NULL,
                                                      kPageSize * 2,
                                                      MEM_RESERVE,
                                                      PAGE_NOACCESS));
  ASSERT_TRUE(memory);
  ASSERT_TRUE(VirtualAlloc(memory, kPageSize,
                           MEM_COMMIT, PAGE_EXECUTE_READWRITE));

  
  memcpy(memory + kOffset, instructions, sizeof(instructions));
  
  
  typedef void (*void_function)(void);
  void_function memory_function =
      reinterpret_cast<void_function>(memory + kOffset);
  ASSERT_DEATH(memory_function(), "");

  
  VirtualFree(memory, 0, MEM_RELEASE);

  
  wstring minidump_filename_wide = find_minidump_in_directory(temp_path_);
  ASSERT_FALSE(minidump_filename_wide.empty());
  string minidump_filename;
  ASSERT_TRUE(WindowsStringUtils::safe_wcstombs(minidump_filename_wide,
                                                &minidump_filename));

  
  
  
  
  {
    Minidump minidump(minidump_filename);
    ASSERT_TRUE(minidump.Read());

    MinidumpException* exception = minidump.GetException();
    MinidumpMemoryList* memory_list = minidump.GetMemoryList();
    ASSERT_TRUE(exception);
    ASSERT_TRUE(memory_list);
    ASSERT_LT((unsigned)0, memory_list->region_count());

    MinidumpContext* context = exception->GetContext();
    ASSERT_TRUE(context);

    uint64_t instruction_pointer;
    ASSERT_TRUE(context->GetInstructionPointer(&instruction_pointer));

    MinidumpMemoryRegion* region =
        memory_list->GetMemoryRegionForAddress(instruction_pointer);
    ASSERT_TRUE(region);

    const size_t kPrefixSize = 128;  
    EXPECT_EQ(kPrefixSize + sizeof(instructions), region->GetSize());
    const uint8_t* bytes = region->GetMemory();
    ASSERT_TRUE(bytes);

    uint8_t prefix_bytes[kPrefixSize];
    memset(prefix_bytes, 0, sizeof(prefix_bytes));
    EXPECT_TRUE(memcmp(bytes, prefix_bytes, sizeof(prefix_bytes)) == 0);
    EXPECT_TRUE(memcmp(bytes + kPrefixSize,
                       instructions, sizeof(instructions)) == 0);
  }

  DeleteFileW(minidump_filename_wide.c_str());
}

}  
