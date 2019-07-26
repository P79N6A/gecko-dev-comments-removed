




























#include <windows.h>
#include <objbase.h>
#include <dbghelp.h>

#include "client/windows/crash_generation/minidump_generator.h"
#include "client/windows/unittests/dump_analysis.h"  

#include "gtest/gtest.h"

namespace {


const MINIDUMP_TYPE kSmallDumpType = static_cast<MINIDUMP_TYPE>(
    MiniDumpWithProcessThreadData |  
    MiniDumpWithUnloadedModules);  


const MINIDUMP_TYPE kLargerDumpType = static_cast<MINIDUMP_TYPE>(
    MiniDumpWithProcessThreadData |  
    MiniDumpWithUnloadedModules |  
    MiniDumpWithIndirectlyReferencedMemory);  


const MINIDUMP_TYPE kFullDumpType = static_cast<MINIDUMP_TYPE>(
    MiniDumpWithFullMemory |  
    MiniDumpWithProcessThreadData |  
    MiniDumpWithHandleData |  
    MiniDumpWithUnloadedModules);  

class MinidumpTest: public testing::Test {
 public:
  MinidumpTest() {
    wchar_t temp_dir_path[ MAX_PATH ] = {0};
    ::GetTempPath(MAX_PATH, temp_dir_path);
    dump_path_ = temp_dir_path;
  }

  virtual void SetUp() {
    
    ASSERT_EQ(NULL, ::GetModuleHandle(L"urlmon.dll"));

    
    
    HMODULE urlmon = ::LoadLibrary(L"urlmon.dll");
    ASSERT_TRUE(urlmon != NULL);
    ASSERT_TRUE(::FreeLibrary(urlmon));
  }

  virtual void TearDown() {
    if (!dump_file_.empty()) {
      ::DeleteFile(dump_file_.c_str());
      dump_file_ = L"";
    }
    if (!full_dump_file_.empty()) {
      ::DeleteFile(full_dump_file_.c_str());
      full_dump_file_ = L"";
    }
  }

  bool WriteDump(ULONG flags) {
    using google_breakpad::MinidumpGenerator;

    
    EXCEPTION_RECORD ex_record = {
        STATUS_ACCESS_VIOLATION,  
        0,  
        NULL,  
        reinterpret_cast<void*>(0xCAFEBABE),  
        2,  
        { EXCEPTION_WRITE_FAULT, reinterpret_cast<ULONG_PTR>(this) }
    };
    CONTEXT ctx_record = {};
    EXCEPTION_POINTERS ex_ptrs = {
      &ex_record,
      &ctx_record,
    };

    MinidumpGenerator generator(dump_path_);

    
    bool result = generator.WriteMinidump(::GetCurrentProcess(),
                                          ::GetCurrentProcessId(),
                                          ::GetCurrentThreadId(),
                                          ::GetCurrentThreadId(),
                                          &ex_ptrs,
                                          NULL,
                                          static_cast<MINIDUMP_TYPE>(flags),
                                          TRUE,
                                          &dump_file_,
                                          &full_dump_file_);
    return result == TRUE;
  }

 protected:
  std::wstring dump_file_;
  std::wstring full_dump_file_;

  std::wstring dump_path_;
};


bool HasFileInfo(const std::wstring& file_path) {
  DWORD dummy;
  const wchar_t* path = file_path.c_str();
  DWORD length = ::GetFileVersionInfoSize(path, &dummy);
  if (length == 0)
    return NULL;

  void* data = calloc(length, 1);
  if (!data)
    return false;

  if (!::GetFileVersionInfo(path, dummy, length, data)) {
    free(data);
    return false;
  }

  void* translate = NULL;
  UINT page_count;
  BOOL query_result = VerQueryValue(
      data,
      L"\\VarFileInfo\\Translation",
      static_cast<void**>(&translate),
      &page_count);

  free(data);
  if (query_result && translate) {
    return true;
  } else {
    return false;
  }
}

TEST_F(MinidumpTest, Version) {
  API_VERSION* version = ::ImagehlpApiVersion();

  HMODULE dbg_help = ::GetModuleHandle(L"dbghelp.dll");
  ASSERT_TRUE(dbg_help != NULL);

  wchar_t dbg_help_file[1024] = {};
  ASSERT_TRUE(::GetModuleFileName(dbg_help,
                                  dbg_help_file,
                                  sizeof(dbg_help_file) /
                                      sizeof(*dbg_help_file)));
  ASSERT_TRUE(HasFileInfo(std::wstring(dbg_help_file)) != NULL);


}

TEST_F(MinidumpTest, Normal) {
  EXPECT_TRUE(WriteDump(MiniDumpNormal));
  DumpAnalysis mini(dump_file_);

  
  EXPECT_TRUE(mini.HasStream(ThreadListStream));
  EXPECT_TRUE(mini.HasStream(ModuleListStream));
  EXPECT_TRUE(mini.HasStream(MemoryListStream));
  EXPECT_TRUE(mini.HasStream(ExceptionStream));
  EXPECT_TRUE(mini.HasStream(SystemInfoStream));
  EXPECT_TRUE(mini.HasStream(MiscInfoStream));

  EXPECT_FALSE(mini.HasStream(ThreadExListStream));
  EXPECT_FALSE(mini.HasStream(Memory64ListStream));
  EXPECT_FALSE(mini.HasStream(CommentStreamA));
  EXPECT_FALSE(mini.HasStream(CommentStreamW));
  EXPECT_FALSE(mini.HasStream(HandleDataStream));
  EXPECT_FALSE(mini.HasStream(FunctionTableStream));
  EXPECT_FALSE(mini.HasStream(UnloadedModuleListStream));
  EXPECT_FALSE(mini.HasStream(MemoryInfoListStream));
  EXPECT_FALSE(mini.HasStream(ThreadInfoListStream));
  EXPECT_FALSE(mini.HasStream(HandleOperationListStream));
  EXPECT_FALSE(mini.HasStream(TokenStream));

  
  EXPECT_FALSE(mini.HasTebs());
  EXPECT_FALSE(mini.HasPeb());

  
  EXPECT_FALSE(mini.HasMemory(this));
}

TEST_F(MinidumpTest, SmallDump) {
  ASSERT_TRUE(WriteDump(kSmallDumpType));
  DumpAnalysis mini(dump_file_);

  EXPECT_TRUE(mini.HasStream(ThreadListStream));
  EXPECT_TRUE(mini.HasStream(ModuleListStream));
  EXPECT_TRUE(mini.HasStream(MemoryListStream));
  EXPECT_TRUE(mini.HasStream(ExceptionStream));
  EXPECT_TRUE(mini.HasStream(SystemInfoStream));
  EXPECT_TRUE(mini.HasStream(UnloadedModuleListStream));
  EXPECT_TRUE(mini.HasStream(MiscInfoStream));

  
  EXPECT_TRUE(mini.HasTebs());
  EXPECT_TRUE(mini.HasPeb());

  EXPECT_FALSE(mini.HasStream(ThreadExListStream));
  EXPECT_FALSE(mini.HasStream(Memory64ListStream));
  EXPECT_FALSE(mini.HasStream(CommentStreamA));
  EXPECT_FALSE(mini.HasStream(CommentStreamW));
  EXPECT_FALSE(mini.HasStream(HandleDataStream));
  EXPECT_FALSE(mini.HasStream(FunctionTableStream));
  EXPECT_FALSE(mini.HasStream(MemoryInfoListStream));
  EXPECT_FALSE(mini.HasStream(ThreadInfoListStream));
  EXPECT_FALSE(mini.HasStream(HandleOperationListStream));
  EXPECT_FALSE(mini.HasStream(TokenStream));

  
  EXPECT_FALSE(mini.HasMemory(this));
}

TEST_F(MinidumpTest, LargerDump) {
  ASSERT_TRUE(WriteDump(kLargerDumpType));
  DumpAnalysis mini(dump_file_);

  
  EXPECT_TRUE(mini.HasStream(ThreadListStream));
  EXPECT_TRUE(mini.HasStream(ModuleListStream));
  EXPECT_TRUE(mini.HasStream(MemoryListStream));
  EXPECT_TRUE(mini.HasStream(ExceptionStream));
  EXPECT_TRUE(mini.HasStream(SystemInfoStream));
  EXPECT_TRUE(mini.HasStream(UnloadedModuleListStream));
  EXPECT_TRUE(mini.HasStream(MiscInfoStream));

  
  EXPECT_TRUE(mini.HasMemory(this));

  
  EXPECT_TRUE(mini.HasTebs());
  EXPECT_TRUE(mini.HasPeb());

  EXPECT_FALSE(mini.HasStream(ThreadExListStream));
  EXPECT_FALSE(mini.HasStream(Memory64ListStream));
  EXPECT_FALSE(mini.HasStream(CommentStreamA));
  EXPECT_FALSE(mini.HasStream(CommentStreamW));
  EXPECT_FALSE(mini.HasStream(HandleDataStream));
  EXPECT_FALSE(mini.HasStream(FunctionTableStream));
  EXPECT_FALSE(mini.HasStream(MemoryInfoListStream));
  EXPECT_FALSE(mini.HasStream(ThreadInfoListStream));
  EXPECT_FALSE(mini.HasStream(HandleOperationListStream));
  EXPECT_FALSE(mini.HasStream(TokenStream));
}

TEST_F(MinidumpTest, FullDump) {
  ASSERT_TRUE(WriteDump(kFullDumpType));
  ASSERT_TRUE(dump_file_ != L"");
  ASSERT_TRUE(full_dump_file_ != L"");
  DumpAnalysis mini(dump_file_);
  DumpAnalysis full(full_dump_file_);

  

  
  EXPECT_TRUE(mini.HasStream(ThreadListStream));
  EXPECT_TRUE(full.HasStream(ThreadListStream));
  EXPECT_TRUE(mini.HasStream(ModuleListStream));
  EXPECT_TRUE(full.HasStream(ModuleListStream));
  EXPECT_TRUE(mini.HasStream(ExceptionStream));
  EXPECT_TRUE(full.HasStream(ExceptionStream));
  EXPECT_TRUE(mini.HasStream(SystemInfoStream));
  EXPECT_TRUE(full.HasStream(SystemInfoStream));
  EXPECT_TRUE(mini.HasStream(UnloadedModuleListStream));
  EXPECT_TRUE(full.HasStream(UnloadedModuleListStream));
  EXPECT_TRUE(mini.HasStream(MiscInfoStream));
  EXPECT_TRUE(full.HasStream(MiscInfoStream));
  EXPECT_TRUE(mini.HasStream(HandleDataStream));
  EXPECT_TRUE(full.HasStream(HandleDataStream));

  
  EXPECT_FALSE(mini.HasMemory(this));
  EXPECT_TRUE(full.HasMemory(this));

  
  EXPECT_TRUE(mini.HasTebs() || full.HasTebs());
  EXPECT_TRUE(mini.HasPeb() || full.HasPeb());

  EXPECT_TRUE(mini.HasStream(MemoryListStream));
  EXPECT_TRUE(full.HasStream(Memory64ListStream));
  EXPECT_FALSE(mini.HasStream(Memory64ListStream));
  EXPECT_FALSE(full.HasStream(MemoryListStream));

  
  
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
